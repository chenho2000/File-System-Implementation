# File System Implementation
*This repository stores work done by Hongyu Chen and Ming Gong in CSC369 Operating Systems.*
SB: Super Block, IB: Inode Bitmap, DB: Data Bitmap, I: Inode Table, D: Data Block

![image](https://github.com/chenho2000/File-System-Implementation/assets/60719036/339c4727-9eb3-40bf-8ade-04ede7bc02aa)


# File System Implementation README

Our file system implementation follows a structure similar to the Very Simple File System (VSFS), comprising five main components:

1. **Super Block (SB):** 
   - Holds metadata about the overall file system, including the total number of inodes and blocks, pointers to critical structures like inode bitmaps and tables, and counts of free blocks and inodes.

2. **Inode Bitmap (IB) and Data Bitmap (DB):** 
   - Used to track used and free inodes and blocks respectively.

3. **Inode Table (I):** 
   - Reserves blocks to store information about each file, such as file mode, size, and pointers to data blocks.

4. **Data Block (D):** 
   - Contains the actual data of the file system.

### Allocation Strategy

We employ an extent-based allocation strategy, using ordered arrays of extents to manage file data. Extents represent contiguous blocks of data and are tracked within the inode table.

1. **Allocation:** 
   - We attempt to allocate new data blocks sequentially, extending existing extents if possible to minimize fragmentation.
   
2. **Deallocation:** 
   - When freeing space, we release data blocks and inodes in a systematic order, updating the corresponding bitmaps and superblock metadata.

### Fragmentation Management

Our allocation strategy aims to minimize fragmentation by:
- Allocating file data to sequential blocks to optimize read performance.
- Prioritizing allocation of small extents before larger ones to reduce external fragmentation.

### Directory Management

Directories are treated similarly to files but utilize a specific structure called "a1fs_dir_entry" to store directory information. When freeing a directory, all files within it are recursively freed before releasing the directory itself.

### Implementation Details and Limits

- The system supports a maximum of 512 extents per file.
- Minimum tested configurations include a 64KiB image size with 4 inodes.
- The maximum path component and full path lengths are defined constants.
- There are no explicit limits on file size, number of directory entries, or total blocks in the file system.
- Read and write operations are block-based.
- Out of space conditions are handled gracefully to prevent metadata leaks.

### Notes

- "." and ".." directory entries are not physically stored.
- Only the modification time (mtime) is stored for files and directories.
- Data and metadata blocks are allocated on demand.
- Efficient block-level I/O operations are performed using `memcpy()`.
- The implementation avoids floating-point arithmetic, using integer arithmetic for division.

For further implementation details and constraints, refer to the provided documentation and header files.
