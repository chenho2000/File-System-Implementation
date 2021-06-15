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
	./a1fs image /tmp/gongmin9


info:
	stat -f /tmp/gongmin9
	ls -la /tmp/gongmin9
debug:
	truncate -s 409600 image
	./mkfs.a1fs -i 16 image
	gdb --args ./a1fs image /tmp/userid -d

case1: 
	mkdir /tmp/gongmin9/dir1
	mkdir /tmp/gongmin9/dir2
	mkdir /tmp/gongmin9/dir3
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9
	rmdir /tmp/gongmin9/dir1
	rmdir /tmp/gongmin9/dir2
	rmdir /tmp/gongmin9/dir3
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9

case2:
	touch /tmp/gongmin9/file1.txt
	touch /tmp/gongmin9/file2.txt
	touch /tmp/gongmin9/file3.txt
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9
	rm /tmp/gongmin9/file1.txt
	rm /tmp/gongmin9/file2.txt
	rm /tmp/gongmin9/file3.txt
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9

case3:
	touch /tmp/gongmin9/file4.txt
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9
	truncate -s 10000 /tmp/gongmin9/file4.txt
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9
	truncate -s 1000 /tmp/gongmin9/file4.txt
	ls -la /tmp/gongmin9
	stat -f /tmp/gongmin9

case4:
	touch /tmp/gongmin9/file5.txt
	ls -la /tmp/gongmin9
	echo "This is a message from Ming & Hongyu" >> /tmp/gongmin9/file5.txt
	ls -la /tmp/gongmin9
	xxd /tmp/gongmin9/file5.txt
	truncate -s 128 /tmp/gongmin9/file5.txt
	ls -la /tmp/gongmin9
	xxd /tmp/gongmin9/file5.txt
	truncate -s 12 /tmp/gongmin9/file5.txt
	ls -la /tmp/gongmin9
	xxd /tmp/gongmin9/file5.tx
	
case5:	
	mkdir /tmp/gongmin9/dirbeforeunmount
	# touch /tmp/gongmin9/filebeforeunmount
	fusermount -u /tmp/gongmin9
	./a1fs image /tmp/gongmin9
	stat -f /tmp/gongmin9
	ls -la /tmp/gongmin9

c:
	rm image
	fusermount -u /tmp/gongmin9