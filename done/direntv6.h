#pragma once

/**
 * @file direntv6.h
 * @brief accessing the UNIX v6 filesystem -- directory layer
 *
 * @author Edouard Bugnion
 * @date summer 2016
 */

#include <stdint.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"

#ifdef __cplusplus
extern "C" {
#endif

struct directory_reader {
    struct filev6 fv6;
    struct direntv6 dirs[DIRENTRIES_PER_SECTOR];
    int cur ;
    int last;
    /* 
     * TODO : Demander s'il faut enlever le champ unused
     */
    int unused; // so that it can compile before WEEK 6
};

/**
 * @brief opens a directory reader for the specified inode 'inr'
 * @param u the mounted filesystem
 * @param inr the inode -- which must point to an allocated directory
 * @param d the directory reader (OUT)
 * @return 0 on success; <0 on errror
 */
#include <stdio.h>
#include "inode.h"
#include "unixv6fs.h"
#include "mount.h"
#include "error.h"
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
    //fprintf(output,"\n******INODE SCAN PRINT******\n");
    for (int i = 0; i < u -> s.s_isize; ++i) {
        err = sector_read(u -> f, u -> s.s_inode_start + i, data);
        if (!err) {
            for (int k = 0; k < INODES_PER_SECTOR; ++k) {
                inode.i_mode = (data[k*32+1] << 8) + data[k*32];
                inode.i_size0 = data[k*32+5];
                inode.i_size1 = (data[k*32+7] << 8) + data[k*32+6];
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
 * @param inode number
 */
void inode_print(const struct inode* inode, uint16_t inr)
{
    FILE* output = stdout;
    fprintf(output,"**********FS INODE START**********\n");

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
    fprintf(output,"***********FS INODE END***********\n");
}

/**
 * @brief read the content of an inode from disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inodePourTestinode structure, read from disk (OUT)
 * @return 0 on success; <0 on error
 */
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
 */
int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d);

/**
 * @brief return the next directory entry.
 * @param d the dierctory reader
 * @param name pointer to at least DIRENTMAX_LEN+1 bytes.  Filled in with the NULL-terminated string of the entry (OUT)
 * @param child_inr pointer to the inode number in the entry (OUT)
 * @return 1 on success;  0 if there are no more entries to read; <0 on error
 */
int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr);

/**
 * @brief debugging routine; print the a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix);

/**
 * @brief get the inode number for the given path
 * @param u a mounted filesystem
 * @param inr the current of the subtree
 * @param entry the prefix to the subtree
 * @return inr on success; <0 on error
 */
int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry);

/**
 * @brief create a new direntv6 with the given name and given mode
 * @param u a mounted filesystem
 * @param entry the path of the new entry
 * @param mode the mode of the new inode
 * @return inr on success; <0 on error
 */
int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode);

#ifdef __cplusplus
}
#endif
