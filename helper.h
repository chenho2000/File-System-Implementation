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
                        {   
                            return -ENOTDIR;
                        }
                    }
                    else
                    {
                        if (get_inode_by_inodenumber(fs, curr_entry->ino, &curr_inode) != 0)
                        {
                            return -ENOTDIR;
                        }
                        if (!(curr_inode.mode & S_IFDIR))
                        {
                            return -ENOTDIR;
                        }
                    }
                    break;
                }
            }
        }

        if (notfound)
        {
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

/** 
 * Remove directory/file name from given path 
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

/**
 * Get the consecutive free blocks number by given length(extend_blocks)
 * Priority 1: there are enough free blocks after the last used block
 * Priority 2: find the first appeared enough free blocks
 * Return -1 if there are no enough free blocks
 */
 
static inline int get_blk_by_length(fs_ctx* fs, unsigned int extend_blocks){
    struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;
    unsigned int blocks_count = sb->blocks_count;
    int found  = -1;
    unsigned int last_block = -1;
    for (unsigned int i = 0; i < blocks_count; i++)
    {   
        
        if (check_bit((fs->block_bitmap_pointer)[i / 8], i % 8) == 0 && found == -1)
        {   
            if (i + extend_blocks > blocks_count) return -1;
                
            found = i;
            for (unsigned int j = i + 1; j < i + extend_blocks - 1; j++){

                if (check_bit((fs->block_bitmap_pointer)[j / 8], j % 8) > 0){
                    found = -1;
                    break;
                }
            }

        }
        if (check_bit((fs->block_bitmap_pointer)[i / 8], i % 8) > 0){
            last_block = i;
        }
    }
    if (last_block + extend_blocks > blocks_count) return found;
    for (unsigned int k = last_block + 1; k < last_block + extend_blocks - 1; k++){
        if (check_bit((fs->block_bitmap_pointer)[k / 8], k % 8) > 0){
            return found;
            }
    }
    return last_block;
    
}

/**
 * Get the longest consecutive free blocks number 
 * Update longest_blocks_count
 * Return -1 if there are no free blocks
 */
static inline int get_consecutive_blk(fs_ctx* fs, unsigned int* longest_blocks_count)
{
    struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;
    unsigned int blocks_count = sb->blocks_count;
    int max = 0;
    unsigned int i = 0;
    int start = -1;
    while(i < blocks_count){
        int count = 0;
        if (check_bit((fs->block_bitmap_pointer)[i / 8], i % 8) == 0){
            for (unsigned int j = i; j < blocks_count; j++){
                if (check_bit((fs->block_bitmap_pointer)[j / 8], i % 8) == 0){
                    count++;
                }
                else{
                    break;
                }
            }
            if (count > max){
                start = i;
                max = count;
            }
            i += count;
        }
        else count++;
    }
    *longest_blocks_count = max;
    return start;
}