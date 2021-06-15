# This code is provided solely for the personal and private use of students
# taking the CSC369H course at the University of Toronto. Copying for purposes
# other than this use is expressly prohibited. All forms of distribution of
# this code, including but not limited to public repositories on GitHub,
# GitLab, Bitbucket, or any other online platform, whether as given or with
# any changes, are expressly prohibited.
#
# Authors: Alexey Khrabrov, Karen Reid
#
# All of the files in this directory and all subdirectories are:
# Copyright (c) 2019 Karen Reid

CC = gcc
CFLAGS  := $(shell pkg-config fuse --cflags) -g3 -Wall -Wextra -Werror $(CFLAGS)
LDFLAGS := $(shell pkg-config fuse --libs) $(LDFLAGS)

.PHONY: all clean

all: a1fs mkfs.a1fs

a1fs: a1fs.o fs_ctx.o map.o options.o
	$(CC) $^ -o $@ $(LDFLAGS)

mkfs.a1fs: map.o mkfs.o
	$(CC) $^ -o $@ $(LDFLAGS)

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:.c=.o)

-include $(OBJ_FILES:.o=.d)

%.o: %.c
	$(CC) $< -o $@ -c -MMD $(CFLAGS)

clean:
	rm -f $(OBJ_FILES) $(OBJ_FILES:.o=.d) a1fs mkfs.a1fs

# test code
setup:
	truncate -s 409600 image
	./mkfs.a1fs -i 16 image
	./a1fs image /tmp/chenho92


info:
	stat -f /tmp/chenho92
	ls -la /tmp/chenho92
debug:
	truncate -s 409600 image
	./mkfs.a1fs -i 16 image
	gdb --args ./a1fs image /tmp/chenho92 -d

case1: 
	mkdir /tmp/chenho92/dir1
	mkdir /tmp/chenho92/dir2
	mkdir /tmp/chenho92/dir3
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92
	rmdir /tmp/chenho92/dir1
	rmdir /tmp/chenho92/dir2
	rmdir /tmp/chenho92/dir3
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92

case2:
	touch /tmp/chenho92/file1.txt
	touch /tmp/chenho92/file2.txt
	touch /tmp/chenho92/file3.txt
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92
	rm /tmp/chenho92/file1.txt
	rm /tmp/chenho92/file2.txt
	rm /tmp/chenho92/file3.txt
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92

case3:
	touch /tmp/chenho92/file4.txt
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92
	truncate -s 10000 /tmp/chenho92/file4.txt
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92
	truncate -s 1000 /tmp/chenho92/file4.txt
	ls -la /tmp/chenho92
	stat -f /tmp/chenho92

case4:
	touch /tmp/chenho92/file5.txt
	ls -la /tmp/chenho92
	echo "This is a message from Ming & Hongyu" >> /tmp/chenho92/file5.txt
	ls -la /tmp/chenho92
	xxd /tmp/chenho92/file5.txt
	truncate -s 128 /tmp/chenho92/file5.txt
	ls -la /tmp/chenho92
	xxd /tmp/chenho92/file5.txt
	truncate -s 12 /tmp/chenho92/file5.txt
	ls -la /tmp/chenho92
	xxd /tmp/chenho92/file5.txt
	
case5:	
	mkdir /tmp/chenho92/dirbeforeunmount
	# touch /tmp/chenho92/filebeforeunmount
	fusermount -u /tmp/chenho92
	./a1fs image /tmp/chenho92
	stat -f /tmp/chenho92
	ls -la /tmp/chenho92

c:
	rm image
	fusermount -u /tmp/chenho92