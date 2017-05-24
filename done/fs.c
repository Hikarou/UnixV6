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
#include <stdlib.h>
#include "mount.h"
#include "sector.h"
#include "direntv6.h"
#include "error.h"
#include "inode.h"

struct unix_filesystem fs;

static int fs_getattr(const char *path, struct stat *stbuf)
{
    if (fs.f == NULL) {
        exit(1);
    }

    int err = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    if (err < 0) {
        return err;
    }

    uint16_t inode_nb = (uint16_t) err;
    struct inode i;
    err = inode_read(&fs, inode_nb, &i);
    if (err != 0) {
        return err;
    }

    memset(stbuf, 0, sizeof(struct stat));

    stbuf -> st_dev = 0;
    stbuf -> st_ino = inode_nb;
    stbuf -> st_mode = (i.i_mode & IFDIR) ? S_IFDIR : S_IFREG;
    stbuf -> st_mode = stbuf -> st_mode | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    stbuf -> st_nlink = i.i_nlink;
    stbuf -> st_uid = i.i_uid;
    stbuf -> st_gid = i.i_gid;
    stbuf -> st_rdev = 0;
    stbuf -> st_size = inode_getsize(&i);
    stbuf -> st_blksize = SECTOR_SIZE;
    stbuf -> st_blocks = (stbuf -> st_size)/SECTOR_SIZE;
    stbuf -> st_atim.tv_sec = i.atime[0];
    stbuf -> st_atim.tv_nsec = i.atime[1];
    stbuf -> st_mtim.tv_sec = i.mtime[0];
    stbuf -> st_mtim.tv_nsec = i.mtime[1];
    stbuf -> st_ctim.tv_sec = i.mtime[0];
    stbuf -> st_ctim.tv_nsec = i.mtime[1];

    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    int err = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    if (err < 0) {
        return err;
    }
    uint16_t inode_nb = (uint16_t) err;

    struct inode i;
    err = inode_read(&fs, inode_nb, &i);

    if (err != 0) {
        return err;
    }

    if (!(i.i_mode & IFDIR)) {
        return ERR_BAD_PARAMETER;
    }

    struct directory_reader d;
    err = direntv6_opendir(&fs, inode_nb, &d);

    if (err < 0) return err;

    char name[DIRENT_MAXLEN+1] = "";
    err = direntv6_readdir(&d, name, &inode_nb);

    while (err == 1) {
        filler(buf, name, NULL, 0);
        err = direntv6_readdir(&d, name, &inode_nb);
    };

    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi)
{
    (void) fi;
    char *ptr = buf;
    char *buf2 = NULL;

    // ouvrir le fichier
    int err = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    if (err < 0) {
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return 0;
    }

    uint16_t inode_nb = (uint16_t) err;
    struct filev6 file;
    err = inode_read(&fs, inode_nb, &(file.i_node));
    if (err != 0) {
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return 0;
    }

    err = filev6_open(&fs, inode_nb, &file);
    if (err != 0) {
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return 0;
    }

    // changer l'offset
    err = filev6_lseek(&file, (int32_t) offset);
    if (err != 0) {
        return 0;
    }

    size_t k = 0;
    //lire les secteurs nécessaires pour avoir 64 ko au max
    int nb_lu = 0;
    if (size < SECTOR_SIZE) {
        buf2 = calloc(1, SECTOR_SIZE);
        if (buf2 == NULL) {
            return 0;
        }
        // utilisation de memcpy
        k = file.offset % SECTOR_SIZE;
        err = filev6_readblock(&file, buf2);
        if (err < 0) {
            return 0;
        } else {
            size_t bytesRead = (size_t) err;
            if (size > bytesRead) {
                size = bytesRead;
            }
            if (k + size > SECTOR_SIZE) {
                size = (size_t) (SECTOR_SIZE - k);
            }
            memcpy(buf, buf2+k, size);
            nb_lu = size;
        }
        free(buf2);
    } else {
        do {
            err = filev6_readblock(&file, ptr);
            if (err == SECTOR_SIZE) {
                ptr += SECTOR_SIZE;
                ++k;
            } else if (err > 0 && err < SECTOR_SIZE) {
                err = 0;
            }
        } while (err > 0 && k*SECTOR_SIZE < size);

        if (err < 0) {
            nb_lu = 0;
        } else {
            nb_lu = file.offset - offset;
        }
    }

    return nb_lu;
}

static struct fuse_operations available_ops = {
    .getattr	= fs_getattr,
    .readdir	= fs_readdir,
    .read	= fs_read,
};

/* From https://github.com/libfuse/libfuse/wiki/Option-Parsing.
 * This will look up into the args to search for the name of the FS.
 */
static int arg_parse(void *data, const char *filename, int key, struct fuse_args *outargs)
{
    (void) data;
    (void) outargs;

    if (key == FUSE_OPT_KEY_NONOPT && fs.f == NULL && filename != NULL) {
        int err = mountv6(filename, &fs);
        if (err != 0) {
            puts(ERR_MESSAGES[err - ERR_FIRST]);
            exit(1);
        }
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int ret = fuse_opt_parse(&args, NULL, NULL, arg_parse);
    if (ret == 0) {
        ret = fuse_main(args.argc, args.argv, &available_ops, NULL);
        (void)umountv6(&fs);
    }
    return ret;
}
