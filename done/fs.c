/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const char *fs_str = "Hello World!\n";
static const char *fs_path = "/hello";

static int fs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, fs_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(fs_str);
    } else
        res = -ENOENT;

    return res;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, fs_path + 1, NULL, 0);

    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    if(strcmp(path, fs_path) != 0)
        return -ENOENT;

    len = strlen(fs_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, fs_str + offset, size);
    } else
        size = 0;

    return size;
}

static struct fuse_operations available_ops = {
    .getattr	= fs_getattr,
    .readdir	= fs_readdir,
    .read	= fs_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &available_ops, NULL);
}
