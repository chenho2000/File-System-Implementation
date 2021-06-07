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
 * CSC369 Assignment 1 - a1fs formatting tool.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include "helper.h"

#include "a1fs.h"
#include "map.h"

/** Command line options. */
typedef struct mkfs_opts
{
	/** File system image file path. */
	const char *img_path;
	/** Number of inodes. */
	size_t n_inodes;

	/** Print help and exit. */
	bool help;
	/** Overwrite existing file system. */
	bool force;
	/** Zero out image contents. */
	bool zero;

} mkfs_opts;

static const char *help_str = "\
Usage: %s options image\n\
\n\
Format the image file into a1fs file system. The file must exist and\n\
its size must be a multiple of a1fs block size - %zu bytes.\n\
\n\
Options:\n\
    -i num  number of inodes; required argument\n\
    -h      print help and exit\n\
    -f      force format - overwrite existing a1fs file system\n\
    -z      zero out image contents\n\
";

static void print_help(FILE *f, const char *progname)
{
	fprintf(f, help_str, progname, A1FS_BLOCK_SIZE);
}

static bool parse_args(int argc, char *argv[], mkfs_opts *opts)
{
	char o;
	while ((o = getopt(argc, argv, "i:hfvz")) != -1)
	{
		switch (o)
		{
		case 'i':
			opts->n_inodes = strtoul(optarg, NULL, 10);
			break;

		case 'h':
			opts->help = true;
			return true; // skip other arguments
		case 'f':
			opts->force = true;
			break;
		case 'z':
			opts->zero = true;
			break;

		case '?':
			return false;
		default:
			assert(false);
		}
	}

	if (optind >= argc)
	{
		fprintf(stderr, "Missing image path\n");
		return false;
	}
	opts->img_path = argv[optind];

	if (opts->n_inodes == 0)
	{
		fprintf(stderr, "Missing or invalid number of inodes\n");
		return false;
	}
	return true;
}

/** Determine if the image has already been formatted into a1fs. */
static bool a1fs_is_present(void *image)
{
	//TODO: check if the image already contains a valid a1fs superblock
	a1fs_superblock *sb = (a1fs_superblock *)image;
	if (image == NULL || sb->magic != A1FS_MAGIC)
	{
		return false;
	}
	return true;
}

/**
 * Format the image into a1fs.
 *
 * NOTE: Must update mtime of the root directory.
 *
 * @param image  pointer to the start of the image.
 * @param size   image size in bytes.
 * @param opts   command line options.
 * @return       true on success;
 *               false on error, e.g. options are invalid for given image size.
 */
static bool mkfs(void *image, size_t size, mkfs_opts *opts)
{
	//TODO: initialize the superblock and create an empty root directory
	//NOTE: the mode of the root directory inode should be set to S_IFDIR | 0777
	if (image == NULL || size == 0 || opts == NULL)
	{
		return false;
	}
	memset(image, 0, size);
	uint64_t magic = A1FS_MAGIC;
	size = (uint64_t)size;
	unsigned int blocks_count = size / A1FS_BLOCK_SIZE;
	unsigned int inode_bitmap_count = ceil_divide(opts->n_inodes, A1FS_BLOCK_SIZE * 8);
	unsigned int block_bitmap_count = ceil_divide(blocks_count, A1FS_BLOCK_SIZE * 8);
	unsigned int inodes_count = opts->n_inodes;
	unsigned int inode_table_count = ceil_divide((sizeof(struct a1fs_inode) * inodes_count), A1FS_BLOCK_SIZE);

	unsigned int first_ino_bitmap = 1;
	unsigned int first_blo_bitmap = inode_bitmap_count + 1;
	unsigned int first_ino = first_blo_bitmap + block_bitmap_count;
	unsigned int first_data_block = first_ino + inode_table_count;

	unsigned int free_blocks_count = blocks_count - inode_bitmap_count - block_bitmap_count - inode_table_count - 2;
	unsigned int free_inodes_count = opts->n_inodes - 1;
	unsigned char *inode_bitmap_pointer = (unsigned char *)(image + first_ino_bitmap * A1FS_BLOCK_SIZE);
	unsigned char *block_bitmap_pointer = (unsigned char *)(image + first_blo_bitmap * A1FS_BLOCK_SIZE);
	unsigned char *inode_pointer = (unsigned char *)(image + first_ino * A1FS_BLOCK_SIZE);
	unsigned char *data_block_pointer = (unsigned char *)(image + first_data_block * A1FS_BLOCK_SIZE);

	if (blocks_count < inode_bitmap_count + inode_table_count + block_bitmap_count + 2)
		return false;
	a1fs_superblock sb = {magic, size, first_blo_bitmap, first_blo_bitmap, first_ino, first_data_block, inode_bitmap_count, block_bitmap_count, inode_table_count, inodes_count, blocks_count, free_blocks_count, free_inodes_count, inode_bitmap_pointer, block_bitmap_pointer, inode_pointer, data_block_pointer};
	memcpy(image, &sb, sizeof(sb));
	memset(image + A1FS_BLOCK_SIZE, 0, (inode_bitmap_count + block_bitmap_count) * A1FS_BLOCK_SIZE);
	int total = inode_bitmap_count + block_bitmap_count + inode_table_count + 1;
	unsigned int total_byte = ceil_divide(total, 8);
	char *block_bitmap = (char *)(image + first_blo_bitmap * A1FS_BLOCK_SIZE);
	char *inode_bitmap = (char *)(image + first_ino_bitmap * A1FS_BLOCK_SIZE);
	for (unsigned int i = 0; i < total_byte; i++)
	{
		int bit = 0;
		while (total >= 0 && bit < 8)
		{
			block_bitmap[i] |= 1 << bit;
			bit++;
			total--;
		}
	}
	for (unsigned int i = 0; i < inodes_count; i++)
	{
		a1fs_inode init_inode = {0};
		memcpy(sizeof(struct a1fs_inode) * i + image + A1FS_BLOCK_SIZE * (1 + inode_bitmap_count + block_bitmap_count), &init_inode, sizeof(struct a1fs_inode));
	}

	struct a1fs_inode *root = (struct a1fs_inode *)(image + first_ino * A1FS_BLOCK_SIZE);
	root->mode = S_IFDIR | 0777;
	root->links = 2;
	root->size = 0;
	clock_gettime(CLOCK_REALTIME, &(root->mtime));
	root->inode = 0;
	root->entry_count = 0;
	root->num_extents = 0;
	root->extent_table = first_ino + inode_table_count;
	root->num_extents = 0;
	inode_bitmap[0] |= 1 << 0;
	block_bitmap[root->extent_table / 8] |= 1 << root->extent_table % 8;
	return true;
}

int main(int argc, char *argv[])
{
	mkfs_opts opts = {0}; // defaults are all 0
	if (!parse_args(argc, argv, &opts))
	{
		// Invalid arguments, print help to stderr
		print_help(stderr, argv[0]);
		return 1;
	}
	if (opts.help)
	{
		// Help requested, print it to stdout
		print_help(stdout, argv[0]);
		return 0;
	}

	// Map image file into memory
	size_t size;
	void *image = map_file(opts.img_path, A1FS_BLOCK_SIZE, &size);
	if (image == NULL)
		return 1;

	// Check if overwriting existing file system
	int ret = 1;
	if (!opts.force && a1fs_is_present(image))
	{
		fprintf(stderr, "Image already contains a1fs; use -f to overwrite\n");
		goto end;
	}

	if (opts.zero)
		memset(image, 0, size);
	if (!mkfs(image, size, &opts))
	{
		fprintf(stderr, "Failed to format the image\n");
		goto end;
	}

	ret = 0;
end:
	munmap(image, size);
	return ret;
}
