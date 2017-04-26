/**
 * @file filev6.c
 * @brief accessing the UNIX v6 filesystem -- file part of inode/file layer
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include "unixv6fs.h"
#include "mount.h"
#include "filev6.h"
#include "error.h"
#include "inode.h"
#include "sector.h"


/**
 * @brief open up a file corresponding to a given inode; set offset to zero
 * @param u the filesystem (IN)
 * @param inr he inode number (IN)
 * @param fv6 the complete filve6 data structure (OUT)
 * @return 0 on success; <0 on errror
 */
int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6)
{
    //required_arguments
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    int readReturn = inode_read(u, inr, &fv6 -> i_node);

    if(readReturn < 0) {
        return readReturn;
    }

    fv6->u = u;
    fv6->i_number = inr;
    fv6->offset = 0;

    return 0;
}

/**
 * @brief change the current offset of the given file to the one specified
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param off the new offset of the file
 * @return 0 on success; <0 on errror
 */
int filev6_lseek(struct filev6 *fv6, int32_t offset)
{
    if (offset < 0) {
        return ERR_BAD_PARAMETER;
    }

    if (offset > inode_getsize(&(fv6 -> i_node))) {
        return ERR_OFFSET_OUT_OF_RANGE;
    }

    fv6 -> offset = offset;

    return 0;
}

/**
 * @brief read at most SECTOR_SIZE from the file at the current cursor
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param buf points to SECTOR_SIZE bytes of available memory (OUT)
 * @return >0: the number of bytes of the file read; 0: end of file; <0 error
 */
int filev6_readblock(struct filev6 *fv6, void *buf)
{
    //required arguments
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);

    //Already been fully read
    int inodeSize = inode_getsize(&(fv6 -> i_node));

    if (fv6 -> offset >= inodeSize) {
        fv6 -> offset = inodeSize;
        return 0;
    }

    int findSector = inode_findsector(fv6 -> u, &(fv6 -> i_node), (fv6 -> offset)/SECTOR_SIZE);
    if (findSector < 0) {
        return findSector;
    }

    int sectorRead = sector_read(fv6 -> u -> f, (uint32_t)findSector, buf);

    if (sectorRead <0) {
        return sectorRead;
    }

    int diff = inodeSize - fv6 -> offset;
    if (diff <= SECTOR_SIZE) {
        fv6 -> offset = inodeSize;
        return diff;
    } else {
        fv6 -> offset += SECTOR_SIZE;
        return SECTOR_SIZE;
    }
}

/**
 * @brief create a new filev6
 * @param u the filesystem (IN)
 * @param mode the new offset of the file
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @return 0 on success; <0 on errror
 */
int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6);

/**
 * @brief write the len bytes of the given buffer on disk to the given filev6
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */
int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, void *buf, int len);
