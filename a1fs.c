/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Alexey Khrabrov, Karen Reid
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 Karen Reid
 */

/**
 * CSC369 Assignment 1 - a1fs driver implementation.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <math.h>

// Using 2.9.x FUSE API
#define FUSE_USE_VERSION 29
#include <fuse.h>

#include "a1fs.h"
#include "fs_ctx.h"
#include "options.h"
#include "map.h"
#include "helper.h"

//NOTE: All path arguments are absolute paths within the a1fs file system and
// start with a '/' that corresponds to the a1fs root directory.
//
// For example, if a1fs is mounted at "~/my_csc369_repo/a1b/mnt/", the path to a
// file at "~/my_csc369_repo/a1b/mnt/dir/file" (as seen by the OS) will be
// passed to FUSE callbacks as "/dir/file".
//
// Paths to directories (except for the root directory - "/") do not end in a
// trailing '/'. For example, "~/my_csc369_repo/a1b/mnt/dir/" will be passed to
// FUSE callbacks as "/dir".

/**
 * Initialize the file system.
 *
 * Called when the file system is mounted. NOTE: we are not using the FUSE
 * init() callback since it doesn't support returning errors. This function must
 * be called explicitly before fuse_main().
 *
 * @param fs    file system context to initialize.
 * @param opts  command line options.
 * @return      true on success; false on failure.
 */
static bool a1fs_init(fs_ctx *fs, a1fs_opts *opts)
{
	// Nothing to initialize if only printing help
	if (opts->help)
		return true;

	size_t size;
	void *image = map_file(opts->img_path, A1FS_BLOCK_SIZE, &size);
	if (!image)
		return false;

	return fs_ctx_init(fs, image, size);
}

/**
 * Cleanup the file system.
 *
 * Called when the file system is unmounted. Must cleanup all the resources
 * created in a1fs_init().
 */
static void a1fs_destroy(void *ctx)
{
	fs_ctx *fs = (fs_ctx *)ctx;
	if (fs->image)
	{
		munmap(fs->image, fs->size);
		fs_ctx_destroy(fs);
	}
}

/** Get file system context. */
static fs_ctx *get_fs(void)
{
	return (fs_ctx *)fuse_get_context()->private_data;
}

/**
 * Get file system statistics.
 *
 * Implements the statvfs() system call. See "man 2 statvfs" for details.
 * The f_bfree and f_bavail fields should be set to the same value.
 * The f_ffree and f_favail fields should be set to the same value.
 * The following fields can be ignored: f_fsid, f_flag.
 * All remaining fields are required.
 *
 * Errors: none
 *
 * @param path  path to any file in the file system. Can be ignored.
 * @param st    pointer to the struct statvfs that receives the result.
 * @return      0 on success; -errno on error.
 */
static int a1fs_statfs(const char *path, struct statvfs *st)
{
	(void)path; // unused
	fs_ctx *fs = get_fs();

	memset(st, 0, sizeof(*st));
	st->f_bsize = A1FS_BLOCK_SIZE;
	st->f_frsize = A1FS_BLOCK_SIZE;
	//TODO: fill in the rest of required fields based on the information stored
	// in the superblock
	(void)fs;
	st->f_namemax = A1FS_NAME_MAX;
	struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;
	fsblkcnt_t f_blocks = sb->blocks_count;
	fsblkcnt_t f_bfree = sb->free_blocks_count;
	fsblkcnt_t f_bavail = sb->free_blocks_count;
	fsfilcnt_t f_files = sb->inodes_count;
	fsfilcnt_t f_ffree = sb->free_inodes_count;
	fsfilcnt_t f_favail = sb->free_inodes_count;
	st->f_blocks = f_blocks;
	st->f_bfree = f_bfree;
	st->f_bavail = f_bavail;
	st->f_files = f_files;
	st->f_ffree = f_ffree;
	st->f_favail = f_favail;

	return 0;
}

/**
 * Get file or directory attributes.
 *
 * Implements the lstat() system call. See "man 2 lstat" for details.
 * The following fields can be ignored: st_dev, st_ino, st_uid, st_gid, st_rdev,
 *                                      st_blksize, st_atim, st_ctim.
 * All remaining fields are required.
 *
 * NOTE: the st_blocks field is measured in 512-byte units (disk sectors);
 *       it should include any metadata blocks that are allocated to the 
 *       inode.
 *
 * NOTE2: the st_mode field must be set correctly for files and directories.
 *
 * Errors:
 *   ENAMETOOLONG  the path or one of its components is too long.
 *   ENOENT        a component of the path does not exist.
 *   ENOTDIR       a component of the path prefix is not a directory.
 *
 * @param path  path to a file or directory.
 * @param st    pointer to the struct stat that receives the result.
 * @return      0 on success; -errno on error;
 */
static int a1fs_getattr(const char *path, struct stat *st)
{
	if (strlen(path) >= A1FS_PATH_MAX)
		return -ENAMETOOLONG;
	fs_ctx *fs = get_fs();

	memset(st, 0, sizeof(*st));

	//NOTE: This is just a placeholder that allows the file system to be mounted
	// without errors. You should remove this from your implementation.
	// if (strcmp(path, "/") == 0)
	// {
	// 	//NOTE: all the fields set below are required and must be set according
	// 	// to the information stored in the corresponding inode
	// 	st->st_mode = S_IFDIR | 0777;
	// 	st->st_nlink = 2;
	// 	st->st_size = 0;
	// 	st->st_blocks = 0 * A1FS_BLOCK_SIZE / 512;
	// 	st->st_mtim = (struct timespec){0};
	// 	return 0;
	// }

	//TODO: lookup the inode for given path and, if it exists, fill in the
	// required fields based on the information stored in the inode
	struct a1fs_inode inode;
	int get_inode = get_inode_by_path(fs->image, fs, path, &inode);
	if (get_inode != 0)
	{
		return get_inode;
	}
	st->st_mode = inode.mode;
	st->st_nlink = inode.links;
	st->st_size = inode.size;
	st->st_blocks = ceil_divide(inode.size, 512);
	st->st_mtim = (struct timespec)inode.mtime;
	return 0;
}

/**
 * Read a directory.
 *
 * Implements the readdir() system call. Should call filler(buf, name, NULL, 0)
 * for each directory entry. See fuse.h in libfuse source code for details.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a filler() call failed).
 *
 * @param path    path to the directory.
 * @param buf     buffer that receives the result.
 * @param filler  function that needs to be called for each directory entry.
 *                Pass 0 as offset (4th argument). 3rd argument can be NULL.
 * @param offset  unused.
 * @param fi      unused.
 * @return        0 on success; -errno on error.
 */
static int a1fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						off_t offset, struct fuse_file_info *fi)
{
	(void)offset; // unused
	(void)fi;	  // unused
	fs_ctx *fs = get_fs();

	//NOTE: This is just a placeholder that allows the file system to be mounted
	// without errors. You should remove this from your implementation.
	// if (strcmp(path, "/") == 0)
	// {
	// 	filler(buf, ".", NULL, 0);
	// 	filler(buf, "..", NULL, 0);
	// 	return 0;
	// }

	//TODO: lookup the directory inode for given path and iterate through its
	// directory entries
	if (filler(buf, ".", NULL, 0) != 0)
		return -ENOMEM;
	if (filler(buf, "..", NULL, 0) != 0)
		return -ENOMEM;
	struct a1fs_inode curr_inode;
	void *image = fs->image;
	int get_inode = get_inode_by_path(fs->image, fs, path, &curr_inode);
	if (get_inode != 0)
	{
		return -errno;
	}

	for (unsigned int i = 0; i < curr_inode.num_extents; i++)
	{
		struct a1fs_extent *curr_extent = (struct a1fs_extent *)(image + curr_inode.extent_table * A1FS_BLOCK_SIZE + i * sizeof(struct a1fs_extent));
		for (int j = 0; j < curr_inode.entry_count; j++)
		{
			struct a1fs_dentry *entry = (struct a1fs_dentry *)(image + curr_extent->start * A1FS_BLOCK_SIZE + j * sizeof(struct a1fs_dentry));
			if (filler(buf, entry->name, NULL, 0) != 0)
			{
				return -ENOMEM;
			}
		}
	}

	return 0;
}

/**
 * Create a directory.
 *
 * Implements the mkdir() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the directory to create.
 * @param mode  file mode bits.
 * @return      0 on success; -errno on error.
 */
static int a1fs_mkdir(const char *path, mode_t mode)
{
	mode = mode | S_IFDIR;
	fs_ctx *fs = get_fs();

	//TODO: create a directory at given path with given mode
	(void)path;
	(void)mode;
	(void)fs;
	return -ENOSYS;
	// struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;

	// if (sb->free_inodes_count <= 0 || sb->free_blocks_count < 2)
	// {
	// 	return -ENOSPC;
	// }

	// unsigned char *inode_bitmap = fs->inode_bitmap_pointer;
	// unsigned char *block_bitmap = fs->block_bitmap_pointer;
	// unsigned char *inode_table = fs->inode_pointer;

	// int new_blk = get_free_blk(fs);
	// update_bitmap_by_index(block_bitmap, new_blk, 1);
	// sb->free_blocks_count--;

	// int new_ino = get_free_ino(fs);
	// update_bitmap_by_index(inode_bitmap, new_ino, 1);
	// sb->free_inodes_count--;
	// struct a1fs_inode *new_inode = (struct a1fs_inode *)(inode_table + sizeof(struct a1fs_inode) * (new_ino));
	// new_inode->mode = mode;
	// new_inode->links = 2;
	// new_inode->size = 0;
	// clock_gettime(CLOCK_REALTIME, &(new_inode->mtime));
	// new_inode->inode = new_ino;
	// new_inode->entry_count = 0;
	// new_inode->num_extents = 0;
	// new_inode->extent_table = new_blk;

	// // extract parant path
	// char *parent_path = get_path(path);

	// // extract directory name
	// char *dir_name = get_name(path);
	// int found_dentry = 0;
	// struct a1fs_inode parent_inode;
	// get_inode_by_path(fs->image, fs, parent_path, &parent_inode);
	// for (unsigned int i = 0; i < parent_inode.num_extents; i++)
	// {
	// 	struct a1fs_extent *extent = (struct a1fs_extent *)(fs->image + parent_inode.extent_table * A1FS_BLOCK_SIZE + sizeof(struct a1fs_extent) * i);
	// 	for (unsigned int j = 0; j < extent->count * A1FS_BLOCK_SIZE / sizeof(struct a1fs_dentry); j++)
	// 	{
	// 		struct a1fs_dentry *new_dentry = (struct a1fs_dentry *)(fs->image + extent->start * A1FS_BLOCK_SIZE + sizeof(struct a1fs_dentry) * j);
	// 		if (new_dentry->ino == 0 && strlen(new_dentry->name) == 0)
	// 		{
	// 			new_dentry->ino = new_ino;
	// 			strncpy(new_dentry->name, dir_name, strlen(dir_name) + 1);
	// 			found_dentry = 1;
	// 			break;
	// 		}
	// 	}
	// 	if (found_dentry == 1)
	// 		break;
	// }

	// //Not block available in parent inode creat new blocks for dentry
	// if (found_dentry == 0)
	// {
	// 	int new_dentry_blk = get_free_blk(fs);
	// 	if (new_dentry_blk == -1)
	// 	{
	// 		return -ENOSPC;
	// 	}
	// 	update_bitmap_by_index(block_bitmap, new_dentry_blk, 1);
	// 	sb->free_blocks_count--;

	// 	struct a1fs_extent *new_extent = (struct a1fs_extent *)(fs->image + parent_inode.extent_table * A1FS_BLOCK_SIZE + sizeof(struct a1fs_extent) * parent_inode.num_extents);
	// 	new_extent->start = new_dentry_blk;
	// 	new_extent->count = 1;
	// 	parent_inode.num_extents++;

	// 	struct a1fs_dentry *new_entry = (struct a1fs_dentry *)(fs->image + new_dentry_blk * A1FS_BLOCK_SIZE);
	// 	new_entry->ino = new_ino;
	// 	strncpy(new_entry->name, dir_name, strlen(dir_name) + 1);

	// 	found_dentry = 1;
	// }

	// // Update parant inode attribute
	// parent_inode.links++;
	// parent_inode.size += sizeof(struct a1fs_dentry);
	// clock_gettime(CLOCK_REALTIME, &(parent_inode.mtime));
	// parent_inode.entry_count++;
	// memcpy(fs->inode_pointer + sizeof(struct a1fs_inode) * parent_inode.inode, &parent_inode, sizeof(a1fs_inode));

	// return 0;
}

/**
 * Remove a directory.
 *
 * Implements the rmdir() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOTEMPTY  the directory is not empty.
 *
 * @param path  path to the directory to remove.
 * @return      0 on success; -errno on error.
 */
static int a1fs_rmdir(const char *path)
{
	fs_ctx *fs = get_fs();

	//TODO: remove the directory at given path (only if it's empty)
	(void)path;
	(void)fs;
	return -ENOSYS;
}

/**
 * Create a file.
 *
 * Implements the open()/creat() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to create.
 * @param mode  file mode bits.
 * @param fi    unused.
 * @return      0 on success; -errno on error.
 */
static int a1fs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	(void)fi; // unused
	assert(S_ISREG(mode));
	fs_ctx *fs = get_fs();

	//TODO: create a file at given path with given mode
	(void)path;
	(void)mode;
	(void)fs;
	return -ENOSYS;
	// struct a1fs_superblock *sb = (struct a1fs_superblock *)fs->image;
	// if (sb->free_inodes_count == 0 || sb->free_blocks_count == 1)
	// {
	// 	return -ENOSPC;
	// }
	// unsigned char *inode_bitmap = fs->inode_bitmap_pointer;
	// unsigned char *block_bitmap = fs->block_bitmap_pointer;
	// unsigned char *inode_table = fs->inode_pointer;

	// int new_blk = get_free_blk(fs);
	// int new_ino = get_free_ino(fs);
	// print_bitmap(fs->inode_bitmap_pointer, sb->inodes_count);
	// print_bitmap(fs->block_bitmap_pointer, sb->blocks_count);
	// if (new_blk == -1 || new_ino == -1)
	// {
	// 	return -ENOSPC;
	// }
	// update_bitmap_by_index(block_bitmap, new_blk, 1);
	// sb->free_blocks_count--;
	// update_bitmap_by_index(inode_bitmap, new_ino, 1);
	// sb->free_inodes_count--;

	// struct a1fs_inode *new_inode = (struct a1fs_inode *)(inode_table + sizeof(struct a1fs_inode) * (new_ino));
	// new_inode->mode = mode;
	// new_inode->links = 1;
	// new_inode->size = 0;
	// clock_gettime(CLOCK_REALTIME, &(new_inode->mtime));
	// new_inode->inode = new_ino;
	// new_inode->num_extents = 0;
	// new_inode->extent_table = new_blk;
	// // extract parant path
	// char *parent_path = get_path(path);
	// // extract directory name
	// char *file_name = get_name(path);
	// struct a1fs_inode parent_inode;
	// get_inode_by_path(fs->image, fs, parent_path, &parent_inode);
	// if (parent_inode.num_extents >= 512)
	// {
	// 	return -ENOSPC;
	// }
	// for (long unsigned int i = 0; i < parent_inode.num_extents; i++)
	// {
	// 	struct a1fs_extent *curr_extent = (struct a1fs_extent *)(fs->image + parent_inode.extent_table * A1FS_BLOCK_SIZE + sizeof(struct a1fs_extent) * i);
	// 	for (long unsigned int j = 0; j < (curr_extent->count * A1FS_BLOCK_SIZE / sizeof(struct a1fs_dentry)); j++)
	// 	{
	// 		struct a1fs_dentry *curr_dentry = (struct a1fs_dentry *)(fs->image + curr_extent->start * A1FS_BLOCK_SIZE + sizeof(struct a1fs_dentry) * j);
	// 		if (curr_dentry->ino == 0)
	// 		{
	// 			curr_dentry->ino = new_ino;
	// 			parent_inode.size += sizeof(struct a1fs_dentry);
	// 			parent_inode.entry_count++;
	// 			clock_gettime(CLOCK_REALTIME, &(parent_inode.mtime));
	// 			strncpy(curr_dentry->name, file_name, strlen(file_name) + 1);
	// 			memcpy(fs->inode_pointer + sizeof(struct a1fs_inode) * parent_inode.inode, &parent_inode, sizeof(a1fs_inode));
	// 			return 0;
	// 		}
	// 	}
	// }

	// return -ENOSPC;
}

/**
 * Remove a file.
 *
 * Implements the unlink() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path  path to the file to remove.
 * @return      0 on success; -errno on error.
 */
static int a1fs_unlink(const char *path)
{
	fs_ctx *fs = get_fs();

	//TODO: remove the file at given path
	(void)path;
	(void)fs;
	return -ENOSYS;
}

/**
 * Change the modification time of a file or directory.
 *
 * Implements the utimensat() system call. See "man 2 utimensat" for details.
 *
 * NOTE: You only need to implement the setting of modification time (mtime).
 *       Timestamp modifications are not recursive. 
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists.
 *
 * Errors: none
 *
 * @param path   path to the file or directory.
 * @param times  timestamps array. See "man 2 utimensat" for details.
 * @return       0 on success; -errno on failure.
 */
static int a1fs_utimens(const char *path, const struct timespec times[2])
{
	fs_ctx *fs = get_fs();

	//TODO: update the modification timestamp (mtime) in the inode for given
	// path with either the time passed as argument or the current time,
	// according to the utimensat man page
	struct a1fs_inode inode;
	int get_inode = get_inode_by_path(fs->image, fs, path, &inode);
	if (get_inode != 0)
	{
		return -errno;
	}
	if (times)
	{
		inode.mtime.tv_sec = times[1].tv_sec;
		inode.mtime.tv_nsec = times[1].tv_nsec;
		unsigned int ino = inode.inode;
		unsigned char *inode_pointer = fs->inode_pointer;
		memcpy(sizeof(struct a1fs_inode) * ino + inode_pointer, &inode, sizeof(a1fs_inode));
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &(inode.mtime));
		unsigned int ino = inode.inode;
		unsigned char *inode_pointer = fs->inode_pointer;
		memcpy(sizeof(struct a1fs_inode) * ino + inode_pointer, &inode, sizeof(a1fs_inode));
	}
	return 0;
}

/**
 * Change the size of a file.
 *
 * Implements the truncate() system call. Supports both extending and shrinking.
 * If the file is extended, the new uninitialized range at the end must be
 * filled with zeros.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to set the size.
 * @param size  new file size in bytes.
 * @return      0 on success; -errno on error.
 */
static int a1fs_truncate(const char *path, off_t size)
{
	fs_ctx *fs = get_fs();

	//TODO: set new file size, possibly "zeroing out" the uninitialized range
	(void)path;
	(void)size;
	(void)fs;
	return -ENOSYS;
}

/**
 * Read data from a file.
 *
 * Implements the pread() system call. Must return exactly the number of bytes
 * requested except on EOF (end of file). Reads from file ranges that have not
 * been written to must return ranges filled with zeros. You can assume that the
 * byte range from offset to offset + size is contained within a single block.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path    path to the file to read from.
 * @param buf     pointer to the buffer that receives the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to read from.
 * @param fi      unused.
 * @return        number of bytes read on success; 0 if offset is beyond EOF;
 *                -errno on error.
 */
static int a1fs_read(const char *path, char *buf, size_t size, off_t offset,
					 struct fuse_file_info *fi)
{
	(void)fi; // unused
	fs_ctx *fs = get_fs();

	//TODO: read data from the file at given offset into the buffer
	a1fs_superblock *sb = (a1fs_superblock *)fs->image;
	struct a1fs_inode inode;
	int get_inode = get_inode_by_path(fs->image, fs, path, &inode);
	if (get_inode != 0)
	{
		return -errno;
	}
	else if (offset >= (int)sb->size || offset >= (int)inode.size)
	{
		return 0;
	}
	char ans[size];
	char *ans_pointer = ans;
	memset(ans, 0, size);
	int curr = size;
	for (int i = 0; i < (int)(inode.num_extents); i++)
	{
		struct a1fs_extent *curr_extent = (struct a1fs_extent *)(fs->image + inode.extent_table * A1FS_BLOCK_SIZE + sizeof(struct a1fs_extent) * i);
		int total_size = (curr_extent->count) * A1FS_BLOCK_SIZE;
		unsigned char *data_pointer = fs->image + (curr_extent->start) * A1FS_BLOCK_SIZE;
		if (curr > total_size)
		{
			memcpy(ans_pointer, data_pointer, total_size);
		}
		else
		{
			memcpy(ans_pointer, data_pointer, curr);
			break;
		}
		ans_pointer += total_size;
		curr -= total_size;
	}
	memcpy(buf, ans + offset, size);

	return 0;
}

/**
 * Write data to a file.
 *
 * Implements the pwrite() system call. Must return exactly the number of bytes
 * requested except on error. If the offset is beyond EOF (end of file), the
 * file must be extended. If the write creates a "hole" of uninitialized data,
 * the new uninitialized range must filled with zeros. You can assume that the
 * byte range from offset to offset + size is contained within a single block.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   ENOSPC  too many extents (a1fs only needs to support 512 extents per file)
 *
 * @param path    path to the file to write to.
 * @param buf     pointer to the buffer containing the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to write to.
 * @param fi      unused.
 * @return        number of bytes written on success; -errno on error.
 */
static int a1fs_write(const char *path, const char *buf, size_t size,
					  off_t offset, struct fuse_file_info *fi)
{
	(void)fi; // unused
	fs_ctx *fs = get_fs();

	//TODO: write data from the buffer into the file at given offset, possibly
	// "zeroing out" the uninitialized range
	(void)path;
	(void)buf;
	(void)size;
	(void)offset;
	(void)fs;
	return -ENOSYS;
}

static struct fuse_operations a1fs_ops = {
	.destroy = a1fs_destroy,
	.statfs = a1fs_statfs,
	.getattr = a1fs_getattr,
	.readdir = a1fs_readdir,
	.mkdir = a1fs_mkdir,
	.rmdir = a1fs_rmdir,
	.create = a1fs_create,
	.unlink = a1fs_unlink,
	.utimens = a1fs_utimens,
	.truncate = a1fs_truncate,
	.read = a1fs_read,
	.write = a1fs_write,
};

int main(int argc, char *argv[])
{
	a1fs_opts opts = {0}; // defaults are all 0
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	if (!a1fs_opt_parse(&args, &opts))
		return 1;

	fs_ctx fs = {0};
	if (!a1fs_init(&fs, &opts))
	{
		fprintf(stderr, "Failed to mount the file system\n");
		return 1;
	}

	return fuse_main(args.argc, args.argv, &a1fs_ops, &fs);
}
