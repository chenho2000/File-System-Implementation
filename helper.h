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

static inline int ceil_divide(int a, int b)
{
    int ans = a / b;
    if (a % b > 0)
    {
        ans++;
    }
    return ans;
}

static inline int get_inode_by_inodenumber(fs_ctx *fs, unsigned int inode_num, struct a1fs_inode *inode)
{
    if (check_bit((fs->inode_bitmap_pointer)[inode_num / 8], inode_num % 8) == 1)
    {
        *inode = *(struct a1fs_inode *)((fs->inode_pointer) + sizeof(struct a1fs_inode) * inode_num);
        return 0;
    }
    return 1;
}

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
                        if (get_inode_by_inodenumber(fs, curr_entry->ino, inode) != 0)
                        {
                            return -ENOTDIR;
                        }
                    }
                    else
                    {
                        if (get_inode_by_inodenumber(fs, curr_entry->ino, inode) != 0)
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
    return 0;
}

static inline void read_and_store(void *image, char *ans, struct a1fs_inode inode)
{
    for (int i = 0; i < (int)(inode.num_extents); i++)
    {
        struct a1fs_extent *curr_extent = (struct a1fs_extent *)(image + inode.extent_table * A1FS_BLOCK_SIZE + sizeof(struct a1fs_extent) * i);
        int total_size = (curr_extent->count) * A1FS_BLOCK_SIZE;
        unsigned char* data_pointer = image + (curr_extent->start) * A1FS_BLOCK_SIZE;
        memcpy(ans, data_pointer, total_size);
        ans += total_size;
    }
}