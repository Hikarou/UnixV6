/**
 * @file filev6.c
 * @brief accessing the UNIX v6 filesystem -- file part of inode/file layer
 *
 * @date mars 2017
 */

#include <string.h>
#include "unixv6fs.h"
#include "mount.h"
#include "filev6.h"
#include "error.h"
#include "inode.h"
#include "sector.h"
#include "bmblock.h"

int write_small_file(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len);
int write_big_file(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len);
int write_change(struct unix_filesystem *u, struct filev6 *fv6);
int filev6_writesector(struct unix_filesystem *u, struct filev6 *fv6, const void* data, int len, uint32_t* sector_number);

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

    // correcteur : plus simple que toutes ces mises à 0, faire un memset(&inode, 0, sizeof(struct inode))
    if (!(mode & IALLOC)) {
        return ERR_BAD_PARAMETER;
    }

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
 * @brief write the len bytes of the given buffer on disk to the given filev6.
 * 		  this function choose between 3 functions if the file is big or small
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */

// correcteur : votre fonction est trop compliquée pour rien, vous n'êtes pas censés gérer tous ces cas là ici.
// c'était la fonction filev6_writesector qui devait gérer ce genre de cas
int filev6_writebytes(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(buf);

    int32_t taille_fichier_actu = inode_getsize(&(fv6 -> i_node));
    size_t taille_fichier_futur = (size_t) taille_fichier_actu + len;

    //vérification si l'on ne dépasse pas la taille du disque
    int nb_secteurs_demandes = taille_fichier_futur / SECTOR_SIZE + 1;
    int premier_secteur_libre = bm_find_next(u -> fbm);
    if (premier_secteur_libre < 0) return ERR_NOT_ENOUGH_BLOCS;
    for (uint64_t i = 0; i < nb_secteurs_demandes; ++i) {
        if (bm_get(u -> fbm, premier_secteur_libre + 1)) return ERR_NOT_ENOUGH_BLOCS;
    }
    const uint8_t *buf_new = buf;

    // test que le fichier à écrire n'est pas trop grand:
    if (taille_fichier_futur > 7*ADDRESSES_PER_SECTOR*SECTOR_SIZE) {
        return ERR_FILE_TOO_LARGE;
    }

    // si le fichier va rester petit
    int err = 0;
    if (taille_fichier_futur <= ADDR_SMALL_LENGTH*SECTOR_SIZE) {
        err = write_small_file(u, fv6, buf, len);

    } else if (taille_fichier_actu <= ADDR_SMALL_LENGTH*SECTOR_SIZE && taille_fichier_actu > 0) {
        // si le fichier est petit mais va devenir grand
        int len_small = ADDR_SMALL_LENGTH*SECTOR_SIZE - taille_fichier_actu;
        // Commencer par remplir les petits fichiers
        if (len_small > 0) {
            err = write_small_file(u, fv6, buf, len_small);
            if (err) {
                return err;
            }
            buf_new += len_small;

        }
        // transformer le système
        err = write_change(u, fv6);
        if (err) {
            return err;
        }
        // Complèter le grand fichier
        err = write_big_file(u, fv6, buf_new, len-len_small);

    } else { // Si le fichier est déjà grand
        err = write_big_file(u, fv6, buf, len);
    }

    return err;
}


/**
 * @brief write the len bytes of the given buffer on disk to the given filev6 for big files
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */
int write_big_file(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(buf);

    const uint8_t* ptr = buf;
    uint8_t address_sector[SECTOR_SIZE];
    int k = 0;

    // on crée un petit fichier qui contiendra les adresses
    struct filev6 address_file;
    memset(&address_file, 0, sizeof(struct filev6));
    memset(address_sector, 0, SECTOR_SIZE);

    int32_t taille_data = inode_getsize(&(fv6 -> i_node));
    int nb_sector_used = 0;
    uint32_t data_sector_number = 0;

    if (taille_data % SECTOR_SIZE) {
        nb_sector_used = taille_data/SECTOR_SIZE +1;
    } else {
        nb_sector_used = taille_data/SECTOR_SIZE;
    }

    // créer un inode pour un faux fichier contenant des addresses
    int err = inode_alloc(u);
    if (err < 0) {
        return err;
    }

    address_file.i_number = err;
    address_file.i_node = fv6 -> i_node;

    int32_t taille_addr = nb_sector_used*sizeof(uint8_t);
    // changer la taille
    err = inode_setsize(&address_file.i_node, 2*taille_addr);
    if (err < 0) {
        return err;
    }

    // écrire l'inode
    err = inode_write(u, address_file.i_number, &address_file.i_node);
    if (err < 0) {
        return err;
    }

    // ouvrir le fichier correctement
    err = filev6_open(u, address_file.i_number, &address_file);
    if (err < 0) {
        return err;
    }

    // lire le dernier secteur d'adresses
    if (taille_data % SECTOR_SIZE > 0) {
        err = filev6_lseek(&address_file, 2*(taille_addr-1));
        if (err < 0) {
            return err;
        }

        err = filev6_readblock(&address_file, address_sector);
        if (err < 0) {
            return err;
        }

        // lire le dernier secteur d'addresses
        k = (2*(taille_addr - 1) % SECTOR_SIZE);
        data_sector_number = (address_sector[k+1] << 8) + address_sector[k];
        err = 0;
    }
    if (data_sector_number >= u -> s.s_fsize) return ERR_NOT_ENOUGH_BLOCS;
    while (err == 0 && len > 0) {
        if (taille_data > 7*ADDRESSES_PER_SECTOR*SECTOR_SIZE) {
            err = ERR_BAD_PARAMETER;
            return err;
        }

        // write_sector à ce secteur avec les data
        err = filev6_writesector(u, fv6, ptr, len, &data_sector_number);

        if (err > 0) {

            ptr += err;
            len -= err;
            taille_data += err;

            if (data_sector_number != 0) {
                // si on a écrit les data dans un nouveau secteur
                // il faut écrire son numéro dans le secteur d'adresses
                err = write_small_file(u, &address_file, (uint16_t*) &data_sector_number, sizeof(uint16_t));
                if (err < 0) {
                    return err;
                }
                // il faut aller relire l'inode écrit par la fonction write_small_file
                err = inode_read(u, address_file.i_number, &(address_file.i_node));
                if (err < 0) {
                    return err;
                }
            }
            // car de toute façon on en a pas besoin de data_sector_number
            data_sector_number = 0;

            // écrire la nouvelle taille des datas
            err = inode_setsize(&(fv6 -> i_node), (int) taille_data);
            if (err < 0) {
                return err;
            }

            err = inode_write(u, fv6 -> i_number, &(fv6 -> i_node));
            if (err < 0) {
                return err;
            }
        }
    }

    // mettre à jour les addresse dans le bon fv6
    for (int i = 0; i<ADDR_SMALL_LENGTH; ++i) {
        fv6 -> i_node.i_addr[i] = address_file.i_node.i_addr[i];
    }

    err = inode_write(u, fv6 -> i_number, &(fv6 -> i_node));
    if (err < 0) {
        return err;
    }

    //supprimer l'inode fake
    address_file.i_node.i_mode = 0;
    err = inode_write(u, address_file.i_number, &(address_file.i_node));
    bm_clear(u ->ibm, address_file.i_number);

    return err;
}


/**
 * @brief Change small into big files
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @return 0 on success; <0 on errror
 */
int write_change(struct unix_filesystem *u, struct filev6 *fv6)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);

    int nb_addr_used = inode_getsize(&(fv6 -> i_node))/SECTOR_SIZE;

    struct inode inode_new = fv6 -> i_node;

    // initialisation du secteur
    uint16_t data_addr[SECTOR_SIZE/2];
    memset(data_addr, 0, SECTOR_SIZE);

    // copier les données dans le nouveau secteurs
    for (int i = 0; i < nb_addr_used; ++i) {
        data_addr[i] = inode_new.i_addr[i] ;
    }

    // pour "tromper" la fonction filev6_writesector on lui dit que la taille est nulle.
    int err = inode_setsize(&(fv6 -> i_node), 0);
    if (err) {
        return err;
    }

    // écrire les adresses dans le nouveau secteur
    uint32_t sector_number = 0;
    err = filev6_writesector(u, fv6, data_addr, 2*nb_addr_used, &sector_number);
    if (err<0) {
        return err;
    }

    // mise à jour des vraies adresses
    fv6 -> i_node.i_addr[0] = sector_number;
    for (int i = 1; i<ADDR_SMALL_LENGTH; ++i) {
        fv6 -> i_node.i_addr[i] = 0;
    }

    // remettre la vraie taille du fichier
    fv6 -> i_node.i_size0 = inode_new.i_size0;
    fv6 -> i_node.i_size1 = inode_new.i_size1;

    err = inode_write(u,  fv6 -> i_number,  &(fv6 -> i_node));
    if (err<0) {
        return err;
    }

    return 0;
}

/**
 * @brief write small files
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on errror
 */
int write_small_file(struct unix_filesystem *u, struct filev6 *fv6, const void *buf, int len)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(buf);

    const uint8_t* ptr = buf;
    int32_t taille = inode_getsize(&(fv6 -> i_node));
    int nb_sector_used = taille/SECTOR_SIZE+1;
    uint32_t sector_number = 0;
    struct inode inode_new = fv6 -> i_node;
    int err = 0;

    while (err == 0 && len > 0) {
        if (nb_sector_used > ADDR_SMALL_LENGTH) {
            err = ERR_BAD_PARAMETER;
        }

        sector_number = inode_new.i_addr[nb_sector_used-1];

        err = filev6_writesector(u, fv6, ptr, len, &sector_number);

        if (err > 0) {

            if (sector_number != 0) {
                nb_sector_used = taille/SECTOR_SIZE+1;
                inode_new.i_addr[nb_sector_used-1] = sector_number;
            }

            ptr += err;
            len -= err;
            taille += err;

            err = inode_setsize(&inode_new, (int) taille);
            if (err < 0) {
                return err;
            }

            err = inode_write(u, fv6 -> i_number, &inode_new);
        }
    }

    return err;
}

/**
 * @brief write a most on sector given by sector_number. If it writes a new sector,
 * the sector_number changes. If not, the sector number is set to zero.
 * @param u the filesystem (IN)
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @param sector_number nomber of sector to be written (IN-OUT)
 * @return number of bytes written on success; <0 on errror
 */
int filev6_writesector(struct unix_filesystem *u, struct filev6 *fv6, const void* data, int len, uint32_t* sector_number)
{
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(data);

    int err = 0;
    int nb_bytes = 0;
    uint8_t sector[SECTOR_SIZE];
    uint8_t read[SECTOR_SIZE];

    // mesure de la taille du fichier actuel
    int32_t taille_actu = inode_getsize(&fv6 -> i_node);
    if (taille_actu >  7 * ADDRESSES_PER_SECTOR * SECTOR_SIZE) {
        return ERR_FILE_TOO_LARGE;
    }

    int taille_last_sector = taille_actu % SECTOR_SIZE;

    // si la taille actuelle ne remplit pas complètement tous les secteurs
    if (taille_last_sector && *sector_number > 0) {
        if (len < SECTOR_SIZE-taille_last_sector) {
            nb_bytes = len;
        } else {
            nb_bytes = SECTOR_SIZE-taille_last_sector;
        }
        memset(sector, 0, SECTOR_SIZE);

        err = sector_read(u -> f, *sector_number, read);

        // rajouter à la fin la suite
        memcpy(sector, read, taille_last_sector);
        memcpy(sector + taille_last_sector, data, nb_bytes);

        // Ecrire le nouveau secteur et mettre à jour l'offset
        err =  sector_write(u -> f, *sector_number, sector);
        if (err) {
            return err;
        }

        // si on a écrit dans le même secteur qu'indiqué, on le met à 0
        *sector_number = 0;
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

        *sector_number = (uint32_t) err;

        // ecrire dans le secteur
        err =  sector_write(u -> f, *sector_number, sector);
        if (err) {
            return err;
        }

        bm_set(u -> fbm, (uint64_t) *sector_number);

    }

    return nb_bytes;
}
