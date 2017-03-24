#include <stdio.h>
#include "inode.h"
#include "unixv6fs.h"
#include "mount.h"
#include "error.h"
#include "sector.h"


int inode_scan_print(const struct unix_filesystem *u)
{
    /**
     * @brief read the content of an inode from disk
     * @param u the filesystem (IN)
     * @param inr the inode number of the inode to read (IN)
     * @param inode the inodePourTestinode structure, read from disk (OUT)
     * @return 0 on success; <0 on error
     */
    uint8_t data[SECTOR_SIZE];
    int err = 0;
    FILE * output = stdout;
    int count = 0;
    struct inode inode;
    fprintf(output,"\n******INODE SCAN PRINT******\n");
    for (int i = 0; i < u -> s.s_isize; ++i) {
        err = sector_read(u -> f, u -> s.s_inode_start + i, data);
        if (!err) {
            for (int k = 0; k < INODES_PER_SECTOR; ++k) {
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
    fprintf(output,"\n");
    return err;
}


void inode_print(const struct inode* inode, uint16_t inr)
{
    FILE* output = stdout;
    fprintf(output,"**********FS INODE START**********\n",inr);

    if (inode == NULL) {
        fprintf(output,"NULL ptr\n");
    } else {
        fprintf(output,"i_mode  : %d\n", inode -> i_mode);
        fprintf(output,"i_nlink : %d\n", inode -> i_nlink);
        fprintf(output,"i_i_uid : %d\n", inode -> i_uid);
        fprintf(output,"i_gid   : %d\n", inode -> i_gid);
        fprintf(output,"i_size0 : %d\n", inode -> i_size0);
        fprintf(output,"i_size1 : %d\n", inode -> i_size1);
        fprintf(output,"size    : %d\n", inode_getsize(inode));
    }
    fprintf(output,"***********FS INODE END***********\n\n ");
}


int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode)
{
    int err = 0;
    uint8_t data[SECTOR_SIZE];
    size_t nbrInodeSec = 0;

    // regarde de ou à ou commencent les inode
    if ((u -> s.s_isize)*INODES_PER_SECTOR < inr) {
        err = ERR_INODE_OUTOF_RANGE;
        return err;
    }
    // Lire le secteur
    err = sector_read(u -> f, u -> s.s_inode_start + inr/INODES_PER_SECTOR, data);
    if (!err) {
        nbrInodeSec = inr%INODES_PER_SECTOR;
        inode -> i_mode = (data[nbrInodeSec*32+1] << 8) + data[nbrInodeSec*32];
        if (inode -> i_mode & IALLOC) {
            inode -> i_nlink = data[nbrInodeSec*32+2];
            inode -> i_uid = data[nbrInodeSec*32+3];
            inode -> i_gid = data[nbrInodeSec*32+4];
            inode -> i_size0 = data[nbrInodeSec*32+5];
            inode -> i_size1 = (data[nbrInodeSec*32+7] << 8) + data[nbrInodeSec*32+6];
            for (int i = 0; i<ADDR_SMALL_LENGTH; ++i) {
                inode -> i_addr[i] = (data[nbrInodeSec*32+9+2*i] << 8) + data[nbrInodeSec*32+8+2*i];
            }
            for (int i = 0; i<2; ++i) {
                inode -> atime[i] = (data[nbrInodeSec*32+25+2*i] << 8) + data[nbrInodeSec*32+24 +2*i];
                inode -> mtime[i] = (data[nbrInodeSec*32+29+2*i] << 8) + data[nbrInodeSec*32+28 +2*i];
            }
        } else {
            inode = NULL;
            return ERR_UNALLOCATED_INODE;
        }
    } else {
        return err;
    }

    return 0;
}


int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off)
{
    int nbSector = 0;
    int32_t size = 0;
    int err = 0;
    int adNbSector = 0;
    uint8_t data[SECTOR_SIZE];

    if (i -> i_mode & IALLOC) {
        size = inode_getsize(i);
        if (size < 8*SECTOR_SIZE) {
            nbSector = file_sec_off;
            if (nbSector > 7) {
                return ERR_OFFSET_OUT_OF_RANGE;
            } else {
                return (i -> i_addr[nbSector]);
            }
        } else if (size < 7 * ADDRESSES_PER_SECTOR * SECTOR_SIZE ) {
            adNbSector =  file_sec_off/ADDRESSES_PER_SECTOR;
            nbSector = file_sec_off%ADDRESSES_PER_SECTOR;
            if (adNbSector > 7) {
                return ERR_OFFSET_OUT_OF_RANGE;
            } else {
                err = sector_read(u->f, i->i_addr[adNbSector], data);
                if (!err) {
                    return (data[2*nbSector+1]<<8)+data[2*nbSector];
                } else {
                    return err;
                }
            }
        } else {
            return ERR_FILE_TOO_LARGE;
        }
    } else {
        return ERR_UNALLOCATED_INODE;
    }


}
