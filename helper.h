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

static inline int ceil_divide(int a, int b)
{
    int ans = a / b;
    if (a % b > 0)
    {
        ans++;
    }
    return ans;
}

static inline int get_inode_by_path(void *image, struct a1fs_superblock *sb, const char *path, struct a1fs_inode *inode)
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
        if ((((sb->inode_bitmap_pointer)[0]) & (1)) == 0)
        {
            return -ENOTDIR;
        }
        *inode = *(struct a1fs_inode *)sb->inode_pointer;
        return 0;
    }
    struct a1fs_inode curr_inode = *(struct a1fs_inode *)sb->inode_pointer;
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
                    int byte = curr_entry->ino / 8;
                    int bit = curr_entry->ino % 8;
                    if (((sb->inode_bitmap_pointer)[byte] & (1 << bit)) > 0)
                    {
                        if (p == NULL)
                        {
                            *inode = *(struct a1fs_inode *)(sb->inode_pointer + curr_entry->ino * sizeof(struct a1fs_inode));
                        }
                        else
                        {
                            curr_inode = *(struct a1fs_inode *)(sb->inode_pointer + curr_entry->ino * sizeof(struct a1fs_inode));
                            if (!(curr_inode.mode & S_IFDIR))
                            {
                                return -ENOTDIR;
                            }
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