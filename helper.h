#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "a1fs.h"
#include "fs_ctx.h"
#include "options.h"
#include "map.h"
#define check_bit(var, pos) ((var) & (1 << (pos)))

// delete
static inline void print_bitmap(unsigned char *b, int end)
{
    int num_of_byte;
    int in_use_bit;
    int bit, byte;
    num_of_byte = end / 8;
    fprintf(stderr, "\n---------------------------------------\n");
    for (byte = 0; byte < num_of_byte; byte++)
    {
        for (bit = 0; bit < 8; bit++)
        {
            in_use_bit = b[byte] & (1 << bit);
            if (in_use_bit >= 1)
            {
                fprintf(stderr, "1");
            }
            else
            {
                fprintf(stderr, "0");
            }
        }

        if (byte != (num_of_byte - 1))
        {
            fprintf(stderr, " ");
        }
    }
    fprintf(stderr, "\n---------------------------------------\n");
}

// just like ceil()
static inline int ceil_divide(int a, int b)
{
    int ans = a / b;
    if (a % b > 0)
    {
        ans++;
    }
    return ans;
}

// find the inode with given inode number
static inline int get_inode_by_inodenumber(fs_ctx *fs, unsigned int inode_num, struct a1fs_inode *inode)
{
    fprintf(stderr, "Inode check%d\n",check_bit((fs->inode_bitmap_pointer)[inode_num / 8], inode_num % 8));
    if (check_bit((fs->inode_bitmap_pointer)[inode_num / 8], inode_num % 8) > 0)
    {
        *inode = *(struct a1fs_inode *)((fs->inode_pointer) + sizeof(struct a1fs_inode) * inode_num);
        return 0;
    }
    return 1;
}

// find the inode with given path
static inline int get_inode_by_path(void *image, fs_ctx *fs, const char *path, struct a1fs_inode *inode)
{
    if (path[0] != '/')
    {
        fprintf(stderr, "\n--------------------0-------------------\n");
        return -ENOTDIR;
    }
    if (strlen(path) >= A1FS_PATH_MAX)
        return -ENAMETOOLONG;

    char copy[A1FS_PATH_MAX];
    strncpy(copy, path, strlen(path) + 1);
    char *p = strtok(copy, "/");
    if (p == NULL)
    {
        if (get_inode_by_inodenumber(fs, 0, inode) != 0)
        {
            fprintf(stderr, "\n--------------------1-------------------\n");
            return -ENOTDIR;
        }
        return 0;
    }
    struct a1fs_inode curr_inode;
    get_inode_by_inodenumber(fs, 0, &curr_inode);
    while (p != NULL)
    {
        bool notfound = true;
        if (strlen(p) > A1FS_NAME_MAX)
        {
            fprintf(stderr, "\n-----------------------6----------------\n");
            return -ENOENT;
        }
        for (unsigned int i = 0; i < curr_inode.num_extents; i++)
        {
            if (!(notfound))
            {
                break;
            }
            struct a1fs_extent *curr_extent = (struct a1fs_extent *)(image + curr_inode.extent_table * A1FS_BLOCK_SIZE + sizeof(struct a1fs_extent) * i);
            for (unsigned int j = 0; j < (curr_extent->count * A1FS_BLOCK_SIZE) / sizeof(struct a1fs_dentry); j++)
            {
                struct a1fs_dentry *curr_entry = (struct a1fs_dentry *)(image + curr_extent->start * A1FS_BLOCK_SIZE + sizeof(struct a1fs_dentry) * j);
                if (strcmp(p, curr_entry->name) == 0)
                {
                    p = strtok(NULL, "/");
                    notfound = false;
                    if (p == NULL)
                    {
                        if (get_inode_by_inodenumber(fs, curr_entry->ino, &curr_inode) != 0)
                        {   print_bitmap(fs->inode_bitmap_pointer, 16);
                            fprintf(stderr, "Inode %d\n",curr_entry->ino);
                            fprintf(stderr, "\n---------------------2------------------\n");
                            return -ENOTDIR;
                        }
                    }
                    else
                    {
                        if (get_inode_by_inodenumber(fs, curr_entry->ino, &curr_inode) != 0)
                        {
                            fprintf(stderr, "\n---------------------3------------------\n");
                            return -ENOTDIR;
                        }
                        if (!(curr_inode.mode & S_IFDIR))
                        {
                            fprintf(stderr, "\n-----------------------4----------------\n");
                            return -ENOTDIR;
                        }
                    }

                    break;
                }
            }
        }

        if (notfound)
        {
            fprintf(stderr, "\n-----------------------5----------------\n");
            return -ENOENT;
        }
    }
    *inode = curr_inode;
    return 0;
}

/** return free inode number, return -1 if not available */
static inline int get_free_ino(fs_ctx *fs)
{
    struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;
    int inodes_count = sb->inodes_count;
    for (int i = 0; i < inodes_count; i++)
    {
        if (check_bit((fs->inode_bitmap_pointer)[i / 8], i % 8) == 0)
        {
            return i;
        }
    }
    return -1;
}

/** return free block number, return -1 if not available */
static inline int get_free_blk(fs_ctx *fs)
{
    struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;
    int blocks_count = sb->blocks_count;
    for (int i = 0; i < blocks_count; i++)
    {
        if (check_bit((fs->block_bitmap_pointer)[i / 8], i % 8) == 0)
        {
            return i;
        }
    }
    return -1;
}

/** Update bitmap by given index and value 1 or 0 */
static inline void update_bitmap_by_index(unsigned char *bitmap_pointer, int index, int value)
{
    if (value == 1)
    {
        bitmap_pointer[index / 8] |= 1 << index % 8;
    }
    else
    {
        bitmap_pointer[index / 8] &= ~(1 << index % 8);
    }
}

/** 
 * Extract directory/file name from given path 
 * return directory/file name
 */
static inline char *get_name(const char *path)
{
    static char temp_path[A1FS_PATH_MAX];
    strncpy(temp_path, path, strlen(path) + 1);
    char *file_name = strrchr(temp_path, '/');
    if (file_name == NULL)
        file_name = temp_path;
    else
        file_name++;
    return file_name;
}

/** R
 * emove directory/file name from given path 
 * return modifeid path 
 */
static inline char *get_path(const char *path)
{
    static char parent_path[A1FS_PATH_MAX];
    strncpy(parent_path, path, strlen(path) + 1);
    char *p1 = strrchr(parent_path, '/');
    if (p1 != NULL)
        *p1 = '\0';
    if (strlen(parent_path) == 0)
    {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    }
    return parent_path;
}