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
    int res = 0;

    //Je ne suis pas sûr si cette vérification est obligatoire
    // OUI je pense que c'est bien
    if (fs.f == NULL) {
        exit(1);
    }

    int inode_nb = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    if (inode_nb < 0) {
		return inode_nb;
    }

    struct inode i;
    int err = inode_read(&fs, inode_nb, &i);
    if (err != 0) {
        return err;
    }

    //For debug purpose
    inode_print(&i);

    memset(stbuf, 0, sizeof(struct stat));

    
    //stbuf -> st_dev = ????; //Je ne sais pas quoi mettre là dedans:
    stbuf -> st_ino = inode_nb;
    stbuf -> st_mode = (i.i_mode & IFDIR) ? S_IFDIR : S_IFREG;
    //Je ne suis pas sûr que l'on puisse séparer ça en deux, mais c'est plus joli, je testerai plus tard
    stbuf -> st_mode = stbuf -> st_mode | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    stbuf -> st_nlink = i.i_nlink;
    stbuf -> st_uid = i.i_uid;
    stbuf -> st_gid = i.i_gid;
    //stbuf -> st_rdev = ????; 
    stbuf -> st_size = inode_getsize(&i);
    //stbuf -> st_blksize = ????;   The st_blksize field gives the "preferred" blocksize for efficient filesystem I/O.   (Writing to a file in smaller chunks may cause an inefficient read-modify-rewrite.)

    stbuf -> st_blocks = (stbuf -> st_size)/SECTOR_SIZE;
    //stbuf -> st_atim = i.atime; //ces deux lignes plantent car elle demandent des types timespec
    //stbuf -> st_mtim = i.mtime;
    //stbuf -> st_ctim = ????;

    return res;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{	
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    int inode_nb = direntv6_dirlookup(&fs, ROOT_INUMBER, path);
    if (inode_nb < 0) {
		return inode_nb;
    }
    
    struct inode i;
    int err = inode_read(&fs, inode_nb, &i);
    if (err != 0) {
        return err;
    }
    
    if (!(i.i_mode & IFDIR)){
    	printf("Problem: required inode on directory\n");
    	return ERR_BAD_PARAMETER;
    } 

	struct directory_reader d; 
	err = direntv6_opendir(&fs, inode_nb, &d);
	
	while (err == 1) {
		for (int i = 0; i < d.last; ++i){
			filler(buf, ((d.dirs)[i]).d_name, NULL, 0);
		}
		
		err = direntv6_opendir(&fs, inode_nb, &d);
	}

    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    (void) fi;
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
