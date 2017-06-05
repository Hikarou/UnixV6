/**
 * @file mount.c
 * @brief accessing the UNIX v6 filesystem -- mounting part
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "unixv6fs.h"
#include "bmblock.h"
#include "error.h"
#include "sector.h"
#include "inode.h"
#include "mount.h"
#include "bmblock.h"
#include <stdlib.h>
#include <inttypes.h>


/**
 * @brief  fill the vector bitmap of the inodes
 * u - the mounted filesystem
 */
void fill_ibm(struct unix_filesystem * u)
{
    struct inode sect_inode[INODES_PER_SECTOR];
    uint64_t actu = 0;
    uint64_t sector_number = 0;


    for (uint64_t j = u -> ibm -> min; j < u -> ibm -> max; ++j) { // tout effacer
        bm_clear(u -> ibm, j);
    }

    while (actu < u -> ibm -> max) { // pour chaque secteur
        int err = sector_read(u -> f, u -> s.s_inode_start + sector_number, sect_inode);
        for (uint16_t k = 0; k < INODES_PER_SECTOR; ++k) { // pour chaque inode
            if (!err) { // si pas d'erreur de lecture
                if ((actu >= u -> ibm -> min) && (actu <= u -> ibm -> max)) {
                    if (sect_inode[k].i_mode & IALLOC) {
                        bm_set(u -> ibm, actu);
                    } else {
                        bm_clear(u -> ibm, actu);
                    }
                }
            } else { // si une erreur de lecture
                bm_set(u -> ibm, actu);
            }
            ++actu;
        }
        ++sector_number;
    }
}

/**
 * @brief  fill the vector bitmap of the sectors
 * u - the mounted filesystem
 */
void fill_fbm(struct unix_filesystem * u)
{
    int taille = 0;
    int taille_grand = 0;
    struct inode inode;

    // mettre tous les secteurs à libre
    for (uint64_t i = u -> fbm -> min; i < u -> fbm -> max; ++i) {
        bm_clear(u->fbm, i);
    }

    // pour chaque inode: appeler inode find sector
    for (uint64_t i = u -> ibm -> min - 1; i < u -> ibm -> max; ++i) {
        int32_t offset = 0;
        int err = bm_get(u -> ibm, i);

        if (err == 1 || i == u -> ibm -> min - 1) {
            err = inode_read(u, i, &inode);
            if (!err) {
                taille = inode_getsize(&inode)/SECTOR_SIZE;
                if (taille < 7*ADDRESSES_PER_SECTOR && taille > 7) {
                    taille_grand = taille/ADDRESSES_PER_SECTOR;
                    for (int k = 0; k <= taille_grand; ++k) {
                        bm_set(u -> fbm, inode.i_addr[k]);
                    }
                }

                err =  inode_findsector(u, &inode, offset);
                while (offset <= taille && err > 0) {
                    bm_set(u -> fbm, (uint64_t)err);
                    ++offset;
                    err =  inode_findsector(u, &inode, offset);
                }
                if (err < 0) {
                    printf("ERROR in inode_findsector\n");
                    puts(ERR_MESSAGES[err - ERR_FIRST]);
                }
            } else {
                printf("ERROR unable to read inode %lu\n", i);
                puts(ERR_MESSAGES[err - ERR_FIRST]);
            }
        }
    }
}

/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(u);
    memset(u, 0, sizeof(*u));
    u -> fbm = NULL;
    u -> ibm = NULL;
    u -> f = fopen(filename, "r+b");
    if (u -> f == NULL) {
        return ERR_IO;
    }

    uint8_t data[SECTOR_SIZE];
    int returnSecRead = ERR_IO;
    returnSecRead = sector_read(u -> f, BOOTBLOCK_SECTOR, data);

    if (returnSecRead != 0) {
        return returnSecRead;
    }

    uint8_t toCheck = data[BOOTBLOCK_MAGIC_NUM_OFFSET];

    if (toCheck !=BOOTBLOCK_MAGIC_NUM) {
        return ERR_BADBOOTSECTOR;
    }
    struct superblock superbck;
    returnSecRead = sector_read(u -> f, SUPERBLOCK_SECTOR, &superbck);
    if (returnSecRead != 0) {
        return returnSecRead;
    }

    u -> s = superbck;

    u -> fbm = NULL;
    u -> ibm = NULL;
    u -> fbm = bm_alloc((uint64_t) (u -> s.s_block_start + 1), (uint64_t) u -> s.s_fsize-1);
    u -> ibm = bm_alloc((uint64_t) (ROOT_INUMBER + 1), (uint64_t) (u -> s.s_isize)*INODES_PER_SECTOR-1);

    if (u -> ibm == NULL ||u -> fbm == NULL ) {
        return ERR_NOMEM;
    }

    fill_ibm(u);
    fill_fbm(u);

    return 0;
}

/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 */
void mountv6_print_superblock(const struct unix_filesystem *u)
{
    FILE* output = stdout;

    fprintf(output, "**********FS SUPERBLOCK START**********\n");
    fprintf(output, "s_isize       : %" PRIu16 "\n", u -> s.s_isize);
    fprintf(output, "s_fsize       : %" PRIu16 "\n", u -> s.s_fsize);
    fprintf(output, "s_fbmsize     : %" PRIu16 "\n", u -> s.s_fbmsize);
    fprintf(output, "s_ibmsize     : %" PRIu16 "\n", u -> s.s_ibmsize);
    fprintf(output, "s_inode_start : %" PRIu16 "\n", u -> s.s_inode_start);
    fprintf(output, "s_block_start : %" PRIu16 "\n", u -> s.s_block_start);
    fprintf(output, "s_fbm_start   : %" PRIu16 "\n", u -> s.s_fbm_start);
    fprintf(output, "s_ibm_start   : %" PRIu16 "\n", u -> s.s_ibm_start);
    fprintf(output, "s_flock       : %" PRIu8 "\n", u -> s.s_flock);
    fprintf(output, "s_ilock       : %" PRIu8 "\n", u -> s.s_ilock);
    fprintf(output, "s_fmod        : %" PRIu8 "\n", u -> s.s_fmod);
    fprintf(output, "s_ronly       : %" PRIu8 "\n", u -> s.s_ronly);
    fprintf(output, "s_time        : [%" PRIu16 "] %" PRIu16 "\n", u -> s.s_time[0], u -> s.s_time[1]);
    fprintf(output, "**********FS SUPERBLOCK END**********\n");

}

/**
 * @brief umount the given filesystem
 * @param u - the mounted filesystem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(u);

    free(u -> ibm);
    free(u -> fbm);

    if(fclose(u -> f) != 0) {
        return ERR_IO;
    }

    return 0;
}


/**
 * @brief create a new filesystem
 * @param num_blocks the total number of blocks (= max size of disk), in sectors
 * @param num_inodes the total number of inodes
 */
int mountv6_mkfs(const char *filename, uint16_t num_blocks, uint16_t num_inodes)
{
    M_REQUIRE_NON_NULL(filename);
    if (num_inodes <= 1) {
        return ERR_NOT_ENOUGH_BLOCS;
    }

    //Calcul des valeur de superblock et tests
    uint16_t s_fsize = num_blocks;	    /* size in sectors of entire volume */
    uint16_t s_isize = num_inodes/INODES_PER_SECTOR;    	/* size in sectors of the inodes */

    if (num_blocks <= 2 + s_isize + num_inodes + 1) {
        return ERR_NOT_ENOUGH_BLOCS;
    }

    //créer superblock
    struct superblock s;
    memset(&s, 0, SECTOR_SIZE);
    s.s_isize = s_isize;
    s.s_fsize = s_fsize;
    s.s_inode_start = SUPERBLOCK_SECTOR + 1;
    s.s_block_start = s.s_inode_start + s_isize + 1;

    // créer un fichier binaire du bon nom et le remplr de zeros juqu'à la bonne taille
    FILE* fichier = NULL;
    fichier = fopen(filename, "wb");
    if (fichier == NULL) {
        return ERR_IO;
    }

    // écrire le bootsector et le superblock
    int err = fseek(fichier, 0, SEEK_SET);
    if (err) {
        fclose(fichier);
        return err;
    }

    uint8_t sector[SECTOR_SIZE];
    memset(sector, 0, SECTOR_SIZE);
    sector[BOOTBLOCK_MAGIC_NUM_OFFSET] = BOOTBLOCK_MAGIC_NUM;
    err = sector_write(fichier, 0, sector);
    if (err) {
        fclose(fichier);
        return err;
    }
    err = sector_write(fichier, 1, &s);
    if (err) {
        fclose(fichier);
        return err;
    }

    // écrire inode_root
    struct inode sect_inode[INODES_PER_SECTOR];
    memset(sect_inode, 0, INODES_PER_SECTOR*sizeof(struct inode));

    struct inode inode;
    memset(&inode, 0, sizeof(struct inode));
    inode.i_mode = IALLOC | IFDIR;
    inode.i_addr[0] = s.s_block_start;

    sect_inode[ROOT_INUMBER] = inode;


    err = sector_write(fichier, s.s_inode_start, sect_inode);
    if (err) {
        fclose(fichier);
        return err;
    }

    memset(sector,0,SECTOR_SIZE);
    // faire une boucle sur tous les secteurs des inodes
    for (uint16_t i = s.s_inode_start+1; i < s.s_block_start; ++i) {
        err = sector_write(fichier, i, sector);
        if (err) {
            fclose(fichier);
            return err;
        }
    }

    // faire une boucle sur tous les secteurs des data
    for (uint16_t i = s.s_block_start; i <= s_fsize; ++i) {
        err = sector_write(fichier, i, sector);
        if (err) {
            fclose(fichier);
            return err;
        }
    }

    fclose(fichier);

    return 0;
}
