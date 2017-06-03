/**
 * @file inode.c
 * @brief accessing the UNIX v6 filesystem -- inode/part
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
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
    int err = 0;
    FILE * output = stdout;
    int count = 0;
    struct inode inode_data[INODES_PER_SECTOR];
    for (uint32_t i = 0; i < u -> s.s_isize; ++i) {
        err = sector_read(u -> f, u -> s.s_inode_start + i, inode_data);
        if (!err) {
            for (uint16_t k = 0; k < INODES_PER_SECTOR; ++k) {
                if (inode_data[k].i_mode & IALLOC) {
                    ++count;
                    fprintf(output, "Inode %3d (", count);
                    if (inode_data[k].i_mode & IFDIR) {
                        fprintf(output, "%s", SHORT_DIR_NAME);
                    } else {
                        fprintf(output, "%s", SHORT_FIL_NAME);
                    }
                    fprintf(output, ") len %6d\n", inode_getsize(inode_data + k));
                }
            }
        } else {
            fprintf(output,"\n");
    		return err;
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
        fprintf(output,"i_mode  : %" PRIu16 "\n", inode -> i_mode);
        fprintf(output,"i_nlink : %" PRIu8 "\n", inode -> i_nlink);
        fprintf(output,"i_uid   : %" PRIu8 "\n", inode -> i_uid);
        fprintf(output,"i_gid   : %" PRIu8 "\n", inode -> i_gid);
        fprintf(output,"i_size0 : %" PRIu8 "\n", inode -> i_size0);
        fprintf(output,"i_size1 : %" PRIu16 "\n", inode -> i_size1);
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
    struct inode data[INODES_PER_SECTOR];
    size_t nbrInodeSec = 0;

    memset(data, 0, INODES_PER_SECTOR);

    // regarde de ou à ou commencent les inodes
    if ((u -> s.s_isize)*INODES_PER_SECTOR < inr || inr < ROOT_INUMBER) {
        err = ERR_INODE_OUTOF_RANGE;
        return err;
    }
    // Lire le secteur
    err = sector_read(u -> f, (uint32_t) (u -> s.s_inode_start + inr / INODES_PER_SECTOR), data);
    if (!err) {
        nbrInodeSec = inr%INODES_PER_SECTOR;
        *inode = data[nbrInodeSec];
        if (!(inode -> i_mode & IALLOC)) {
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
        if (size <= ADDR_SMALL_LENGTH*SECTOR_SIZE) {
            nbSector = file_sec_off;
            if (nbSector > ADDR_SMALL_LENGTH) {
                return ERR_OFFSET_OUT_OF_RANGE;
            } else {
                return (i -> i_addr[nbSector]);
            }
        } else if (size <= (ADDR_SMALL_LENGTH - 1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE ) {
            adNbSector =  file_sec_off/ADDRESSES_PER_SECTOR;
            nbSector = file_sec_off%ADDRESSES_PER_SECTOR;
            if (adNbSector > (ADDR_SMALL_LENGTH - 1)) {
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
    struct inode data[INODES_PER_SECTOR];
    size_t nbrInodeSec = 0;


    if ((u -> s.s_isize)*INODES_PER_SECTOR < inr || inr < ROOT_INUMBER) {
        return ERR_INODE_OUTOF_RANGE;
    }
    // Lire le secteur
    err = sector_read(u -> f, (uint32_t) (u -> s.s_inode_start + inr / INODES_PER_SECTOR), data);
    nbrInodeSec = inr%INODES_PER_SECTOR;

    if (!err) {
        data[nbrInodeSec] = *inode;
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

    int err = bm_find_next(u -> ibm);
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
    uint16_t nb_bin_grand = -1;

    inode -> i_size1 = (uint16_t) (nb_bin_grand & new_size);
    inode -> i_size0 = (uint8_t) (new_size >> 16) & nb_bin_petit;

    return 0;
}
