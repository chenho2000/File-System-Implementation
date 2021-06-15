#!/bin/bash
root="/tmp/gongmin9"
# Modify your path
echo "Setup"
echo ""
make
truncate -s 409600 image
./mkfs.a1fs -i 16 image
./a1fs image "$root"
echo "--------------------------------------------------------------------------"
echo "System information"
echo ""
stat -f "$root"
ls -la "$root"
echo "--------------------------------------------------------------------------"
echo "mkdir/rmdir"
echo ""
mkdir "$root"/dir1
mkdir "$root"/dir2
mkdir "$root"/dir3
ls -la "$root"
stat -f "$root"
rmdir "$root"/dir1
rmdir "$root"/dir2
rmdir "$root"/dir3
ls -la "$root"
stat -f "$root"
echo "--------------------------------------------------------------------------"
echo "create/unlink"
echo ""
touch "$root"/file1.txt
touch "$root"/file2.txt
touch "$root"/file3.txt
ls -la "$root"
stat -f "$root"
rm "$root"/file1.txt
rm "$root"/file2.txt
rm "$root"/file3.txt
ls -la "$root"
stat -f "$root"
echo "--------------------------------------------------------------------------"
echo "truncate"
echo ""
touch "$root"/file4.txt
ls -la "$root"
stat -f "$root"
truncate -s 10000 "$root"/file4.txt
ls -la "$root"
stat -f "$root"
truncate -s 1000 "$root"/file4.txt
ls -la "$root"
stat -f "$root"
echo "--------------------------------------------------------------------------"
echo "read/write"
echo ""
touch "$root"/file5.txt
ls -la "$root"
echo "This is a message from Ming & Hongyu" >> "$root"/file5.txt
ls -la "$root"
xxd "$root"/file5.txt
truncate -s 128 "$root"/file5.txt
ls -la "$root"
xxd "$root"/file5.txt
truncate -s 12 "$root"/file5.txt
ls -la "$root"
xxd "$root"/file5.txt
echo "--------------------------------------------------------------------------"
echo ""
echo "unmount/remount"
mkdir "$root"/dirbeforeunmount	
fusermount -u "$root"
./a1fs image "$root"
stat -f "$root"
ls -la "$root"
echo "--------------------------------------------------------------------------"
echo "clean"
echo ""
rm image
fusermount -u "$root"
make clean
