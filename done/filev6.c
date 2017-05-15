/**
 * @file filev6.c
 * @brief accessing the UNIX v6 filesystem -- file part of inode/file layer
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include <string.h>
#include "unixv6fs.h"
#include "mount.h"
#include "filev6.h"
#include "error.h"
#include "inode.h"
#include "sector.h"


/**
 * @brief open the file corresponding to a given inode; set offset to zero
 * @param u the filesystem (IN)
 * @param inr the inode number (IN)
 * @param fv6 the complete filve6 data structure (OUT)
 * @return 0 on success; the appropriate error code (<0) on error
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

    if (offset >= inode_getsize(&(fv6 -> i_node))) {
        return ERR_OFFSET_OUT_OF_RANGE;
    }

    fv6 -> offset = offset;

    return 0;
}

/**
 * @brief read at most SECTOR_SIZE from the file at the current cursor
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param buf points to SECTOR_SIZE bytes of available memory (OUT)
 * @return >0: the number of bytes of the file read; 0: end of file;
 *             the appropriate error code (<0) on error
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
int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);

    struct inode inode;

    if (!(mode & IALLOC)) {
        return ERR_BAD_PARAMETER;
    }

    // Intialiser l'inode TODO: vérfier que ces champs soient les bons
    inode.i_mode = mode;
    inode.i_nlink = 0;
    inode.i_uid = 0;
    inode.i_gid = 0;
    inode.i_size0 = 0;
    inode.i_size1 = 0;
    for (size_t i = 0; i<ADDR_SMALL_LENGTH; ++i) {
        inode.i_addr[i] = 0;
    }

    for (size_t i = 0; i<2; ++i) {
        inode.atime[i] = 0;
        inode.mtime[i] = 0;
    }

    // recopier l'inode dans fv6
    fv6 -> i_node = inode;
    fv6 -> u = u;
    fv6 -> offset = 0;


    // écrire l'inode sur le disk
    return inode_write(u, fv6 -> i_number, &inode);
}

/**
 * @brief write the len bytes of the given buffer on disk to the given filev6
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */
int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(buf);

    int err = 0;

    return err;

}


/**
 * Retourne le nombre de caractère écrit
 *
 */
int filev6_writesector(struct unix_filesystem *u, struct filev6 *fv6, void* data, int len)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(data);


    int err = 0;
    size_t taille_actu = 0;
    int nb_bytes = 0;
    int taille_last_sector = 0;
    int nb_sector_used = 0;
    uint32_t sector_number = 0;
    uint8_t sector[SECTOR_SIZE];
    uint8_t read[SECTOR_SIZE];

    // mesure de la taille du fichier actuel
    taille_actu = inode_getsize(&fv6 -> i_node);
    if (taille_actu >  7 * ADDRESSES_PER_SECTOR * SECTOR_SIZE) {
        return ERR_FILE_TOO_LARGE;
    }

    nb_sector_used = taille_actu / SECTOR_SIZE;
    taille_last_sector = taille_actu % SECTOR_SIZE;
    // si la taille actuelle ne remplit pas complètement tous les secteurs
    if (taille_last_sector) {
        if (len < SECTOR_SIZE-taille_last_sector) {
            nb_bytes = len;
        } else {
            nb_bytes = SECTOR_SIZE-taille_last_sector;
        }
        memset(sector, 0, SECTOR_SIZE);

        // Lire le secteur actuel
        if (nb_sector_used > 7) {
            return ERR_BAD_PARAMETER;
        }

        sector_number = fv6 -> i_node.i_addr[nb_sector_used-1];
        err = sector_read(u -> f, sector_number, read);

        // rajouter à la fin la suite
        memcpy(sector, read, taille_last_sector);
        memcpy(sector + taille_last_sector, data, nb_bytes);

        // Ecrire le nouveau secteur et mettre à jour l'offset
        err =  sector_write(u -> f, sector_number, sector);
        if (err) {
            return err;
        }
    } else {
        // remplir le secteur
        memset(sector, 0, SECTOR_SIZE);
        if (len < SECTOR_SIZE) {
            nb_bytes = len;
        } else {
            nb_bytes = SECTOR_SIZE;
        }

        memcpy(sector, data, nb_bytes);

        // trouver le prochain secteur libre
        err = bm_find_next(u -> fbm);
        if (err < 0) {
            return err;
        }
        sector_number = (uint32_t) err;

        // ecrire dans le secteur
        err =  sector_write(u -> f, sector_number, sector);
        if (err) {
            return err;
        }

        /* a mon avis, cette fonction ferait mieux d'aller ailleurs
         * de même que celle qui met à jour la taille d'un inode
         */

        // mettre à jour les adresses de l'inode
        if (nb_sector_used >7) {
            return ERR_BAD_PARAMETER;
        }
        fv6 -> i_node.i_addr[nb_sector_used] = sector_number;

    }


    // mise à jour de l'offset
    fv6 -> offset += nb_bytes;
    return nb_bytes;
}

