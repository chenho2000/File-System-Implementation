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

debug:
	truncate -s 409600 image
	./mkfs.a1fs -i 16 image
	gdb --args ./a1fs image /tmp/userid -d

test:
	truncate -s 409600 image
	./mkfs.a1fs -i 16 image
	./a1fs image /tmp/userid
	stat -f /tmp/userid
	ls -la /tmp/userid
	touch /tmp/userid/test1
	touch /tmp/userid/test2
	touch /tmp/userid/test3
	rm /tmp/userid/test1

touch:
	touch /tmp/userid/test
testdir:
	truncate -s 409600 image
	./mkfs.a1fs -i 16 image
	./a1fs image /tmp/userid
	stat -f /tmp/userid
	ls -la /tmp/userid
	mkdir /tmp/userid/csc369

c:
	rm image
	fusermount -u /tmp/userid
