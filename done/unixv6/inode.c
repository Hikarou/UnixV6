#include <stdio.h>
#include "inode.h"
#include "unixv6fs.h"
#include "mount.h"
#include "sector.h"


int inode_scan_print(const struct unix_filesystem *u)
{
    /**
     * @brief read the content of an inode from disk
     * @param u the filesystem (IN)
     * @param inr the inode number of the inode to read (IN)
     * @param inode the inode structure, read from disk (OUT)
     * @return 0 on success; <0 on error
     */
    uint8_t data[SECTOR_SIZE];
    int err = 0;
    FILE * output = stdout;
    int count = 0;
    struct inode inode;

    for (int i = 0; i < u -> s.s_isize; ++i) {
        err = sector_read(u -> f, u -> s.s_inode_start + i, data);
        if (!err) {
            for (int k = 0; k < 16; ++k) {
                inode.i_mode = (data[k*32+1] << 8) + data[k*32];
                inode.i_size0 = data[k*32+5];
                inode.i_size1 = (data[k*32+7] << 8) + data[k*32+6];
                if (inode.i_mode & IALLOC) {
                    ++count;
                    if (inode.i_mode & IFDIR) {
                        fprintf(output, "Inode %3d (%s) len %6d\n", count, SHORT_DIR_NAME, inode_getsize(&inode));
                    } else {
                        fprintf(output, "Inode %3d (%s) len %6d\n", count, SHORT_FIL_NAME, inode_getsize(&inode));
                    }
                }
            }
        } else {
            i = u -> s.s_isize; // si il y a une erreur on sort de la boucle
        }

    }

    return err;
}


void inode_print(const struct inode* pInode)
{
	FILE* output = stdout;
	fprintf(output,"**********FS INODE START**********\n");
	
	if (pInode == NULL){
		fprintf(output,"NULL ptr\n");
	}
	else{
		fprintf(output,"i_mode : %d\n", pInode -> i_mode);
		fprintf(output,"i_nlink : %d\n", pInode -> i_nlink);
		fprintf(output,"i_i_uid : %d\n", pInode -> i_uid);
		fprintf(output,"i_gid : %d\n", pInode -> i_gid);
		fprintf(output,"i_size0 : %d\n", pInode -> i_size0);
		fprintf(output,"i_size1 : %d\n", pInode -> i_size1);
		fprintf(output,"size : %d\n", inode_getsize(pInode));
	}
	fprintf(output,"***********FS INODE END***********\n");
}
