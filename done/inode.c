/**
 * @file inode.c
 * @brief accessing the UNIX v6 filesystem -- inode/part
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */
#include <stdio.h>
#include "inode.h"
#include "unixv6fs.h"
#include "mount.h"
#include "error.h"
#include "bmblock.h"
#include "sector.h"

/**
 * @brief read all inodes from disk and print out their content to
 *        stdout according to the assignment
 * @param u the filesystem
 * @return 0 on success; < 0 on error.
 */
int inode_scan_print(const struct unix_filesystem *u)
{
    uint8_t data[SECTOR_SIZE];
    int err = 0;
    FILE * output = stdout;
    int count = 0;
    struct inode inode;
    for (uint32_t i = 0; i < u -> s.s_isize; ++i) {
        err = sector_read(u -> f, u -> s.s_inode_start + i, data);
        if (!err) {
            for (uint16_t k = 0; k < INODES_PER_SECTOR; ++k) {
                inode.i_mode = (uint16_t)((data[k * 32 + 1] << 8) + data[k * 32]);
                inode.i_size0 = data[k * 32 + 5];
                inode.i_size1 = (uint16_t)((data[k * 32 + 7] << 8) + data[k * 32 + 6]);
                if (inode.i_mode & IALLOC) {
                    ++count;
                    fprintf(output, "Inode %3d (", count);
                    if (inode.i_mode & IFDIR) {
                        fprintf(output, "%s", SHORT_DIR_NAME);
                    } else {
                        fprintf(output, "%s", SHORT_FIL_NAME);
                    }
                    fprintf(output, ") len %6d\n", inode_getsize(&inode));
                }
            }
        } else {
            i = u -> s.s_isize; // si il y a une erreur on sort de la boucle
        }
    }
    fprintf(output,"\n");
    return err;
}

/**
 * @brief prints the content of an inode structure
 * @param inode the inode structure to be displayed
 */
void inode_print(const struct inode* inode)
{
    FILE* output = stdout;
    fprintf(output,"**********FS INODE START**********\n");

    if (inode == NULL) {
        fprintf(output,"NULL ptr\n");
    } else {
        fprintf(output,"i_mode  : %d\n", inode -> i_mode);
        fprintf(output,"i_nlink : %d\n", inode -> i_nlink);
        fprintf(output,"i_uid   : %d\n", inode -> i_uid);
        fprintf(output,"i_gid   : %d\n", inode -> i_gid);
        fprintf(output,"i_size0 : %d\n", inode -> i_size0);
        fprintf(output,"i_size1 : %d\n", inode -> i_size1);
        fprintf(output,"size    : %d\n", inode_getsize(inode));

    }
    fprintf(output,"***********FS INODE END***********\n");
}

/**
 * @brief read the content of an inode from disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (OUT)
 * @return 0 on success; <0 on error
 */
int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode)
{
    int err = 0;
    uint8_t data[SECTOR_SIZE];
    size_t nbrInodeSec = 0;

    // regarde de ou à ou commencent les inodes
    if ((u -> s.s_isize)*INODES_PER_SECTOR < inr || inr < ROOT_INUMBER) {
        err = ERR_INODE_OUTOF_RANGE;
        return err;
    }
    // Lire le secteur
    err = sector_read(u -> f, (uint32_t) (u -> s.s_inode_start + inr / INODES_PER_SECTOR), data);
    if (!err) {
        nbrInodeSec = inr%INODES_PER_SECTOR;
        inode -> i_mode = (uint16_t)((data[nbrInodeSec*32+1] << 8) + data[nbrInodeSec*32]);
        if (inode -> i_mode & IALLOC) {
            inode -> i_nlink = data[nbrInodeSec*32+2];
            inode -> i_uid = data[nbrInodeSec*32+3];
            inode -> i_gid = data[nbrInodeSec*32+4];
            inode -> i_size0 = data[nbrInodeSec*32+5];
            inode -> i_size1 = (uint16_t)((data[nbrInodeSec*32+7] << 8) + data[nbrInodeSec*32+6]);
            for (size_t i = 0; i<ADDR_SMALL_LENGTH; ++i) {
                inode -> i_addr[i] = (uint16_t)((data[nbrInodeSec*32+9+2*i] << 8) +
                                                data[nbrInodeSec*32+8+2*i]);
            }
            for (size_t i = 0; i<2; ++i) {
                inode -> atime[i] = (uint16_t)((data[nbrInodeSec*32+25+2*i] << 8) +
                                               data[nbrInodeSec*32+24 +2*i]);
                inode -> mtime[i] = (uint16_t)((data[nbrInodeSec*32+29+2*i] << 8) +
                                               data[nbrInodeSec*32+28 +2*i]);
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

/**
 * @brief identify the sector that corresponds to a given portion of a file
 * @param u the filesystem (IN)
 * @param inode the inode (IN)
 * @param file_sec_off the offset within the file (in sector-size units)
 * @return >0: the sector on disk; <0 error
 */
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

/**
 * @brief write the content of an inode to disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (IN)
 * @return 0 on success; <0 on error
 */
int inode_write(struct unix_filesystem *u, uint16_t inr, const struct inode *inode)
{

    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(inode);

    int err = 0;
    uint8_t data[SECTOR_SIZE];
    size_t nbrInodeSec = 0;
    uint16_t nb_bin_petit = (1<<8)-1;
    uint16_t nb_bin_grand = -1 - nb_bin_petit;


    if ((u -> s.s_isize)*INODES_PER_SECTOR < inr || inr < ROOT_INUMBER) {
        return ERR_INODE_OUTOF_RANGE;
    }
    // Lire le secteur
    err = sector_read(u -> f, (uint32_t) (u -> s.s_inode_start + inr / INODES_PER_SECTOR), data);
    nbrInodeSec = inr%INODES_PER_SECTOR;

    if (!err) {
        data[nbrInodeSec*32] = (uint8_t) (inode -> i_mode & nb_bin_petit);
        data[nbrInodeSec*32+1] = (uint8_t) ((inode -> i_mode & nb_bin_grand) >> 8);
        data[nbrInodeSec*32+2] = (uint8_t) (inode -> i_nlink);
        data[nbrInodeSec*32+3] = (uint8_t) (inode -> i_uid);
        data[nbrInodeSec*32+4] = (uint8_t) (inode -> i_gid);
        data[nbrInodeSec*32+5] = (uint8_t) (inode -> i_size0);
        data[nbrInodeSec*32+6] = (uint8_t) (inode -> i_size1 & nb_bin_petit);
        data[nbrInodeSec*32+7] = (uint8_t) ((inode -> i_size1 & nb_bin_grand) >> 8);

        for (size_t i = 0; i<ADDR_SMALL_LENGTH; ++i) {
            //inode -> i_addr[i] = (uint16_t)((data[nbrInodeSec*32+9+2*i] << 8) +
            //                                data[nbrInodeSec*32+8+2*i]); //TODO A enlever?
            data[nbrInodeSec*32+8+2*i] = (uint8_t) ((inode -> i_addr[i] & nb_bin_petit));
            data[nbrInodeSec*32+9+2*i] = (uint8_t) ((inode -> i_addr[i] & nb_bin_grand) >> 8);
        }
        for (size_t i = 0; i<2; ++i) {
            //inode -> atime[i] = (uint16_t)((data[nbrInodeSec*32+25+2*i] << 8) +
            //                               data[nbrInodeSec*32+24 +2*i]);
            data[nbrInodeSec*32+24+2*i] = (uint8_t) (inode -> atime[i] & nb_bin_petit);
            data[nbrInodeSec*32+25+2*i] = (uint8_t) ((inode -> atime[i] & nb_bin_grand) >> 8);
            // inode -> mtime[i] = (uint16_t)((data[nbrInodeSec*32+29+2*i] << 8) +
            //                                data[nbrInodeSec*32+28 +2*i]);
            data[nbrInodeSec*32+28+2*i] = (uint8_t) (inode -> mtime[i] & nb_bin_petit);
            data[nbrInodeSec*32+29+2*i] = (uint8_t) ((inode -> mtime[i] & nb_bin_grand) >> 8);
        }


        return sector_write(u -> f, (uint32_t) (u -> s.s_inode_start + inr / INODES_PER_SECTOR), data);
    } else {
        return err;
    }
}

/**
 * @brief alloc a new inode (returns its inr if possible)
 * @param u the filesystem (IN)
 * @return the inode number of the new inode or error code on error
 */
int inode_alloc(struct unix_filesystem *u)
{
	M_REQUIRE_NON_NULL(u);

    int err = 0;

    err = bm_find_next(u -> ibm);
    if (err < 0) {
        return ERR_NOMEM;
    }

    bm_set(u -> ibm, (uint64_t) err);

    return err;
}

/**
 * @brief set the size of a given inode to the given size
 * @param inode the inode
 * @param new_size the new size
 * @return 0 on success; <0 on error
 */
int inode_setsize(struct inode *inode, int new_size)
{
    M_REQUIRE_NON_NULL(inode);
	
	uint16_t nb_bin_petit = (1<<8)-1;
	
	inode -> i_size0 = (uint8_t) (nb_bin_petit & new_size);
	inode -> i_size1 = (uint16_t) (new_size - inode -> i_size0) >> 8;
	
	return 0;
}
