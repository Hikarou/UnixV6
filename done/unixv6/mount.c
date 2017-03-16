#pragma once

/**
 * @file mount.h
 * @brief accessing the UNIX v6 filesystem -- core of the first set of assignments
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
#include "mount.h"
#include "sector.h"
#include <inttypes.h>

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
    u -> f = fopen(filename, "rd");
    if (u -> f == NULL) {
        return ERR_IO;
    }

    uint8_t data[SECTOR_SIZE];
    int returnSecRead = ERR_IO;
    returnSecRead = sector_read(u -> f, BOOTBLOCK_SECTOR, data);

    if (returnSecRead != 0) {
        return returnSecRead;
    }

    uint8_t toCheck;
    toCheck = data[BOOTBLOCK_MAGIC_NUM_OFFSET];


    if (toCheck !=BOOTBLOCK_MAGIC_NUM) {
        return ERR_BADBOOTSECTOR;
    }
    uint8_t superblock[SECTOR_SIZE];
    returnSecRead = sector_read(u -> f, SUPERBLOCK_SECTOR, superblock);
    if (returnSecRead != 0) {
        return returnSecRead;
    }

    u -> s.s_isize = (superblock[1] << 8) + superblock[0];
    u -> s.s_fsize = (superblock[3] << 8) + superblock[2];
    u -> s.s_fbmsize = (superblock[5] << 8) + superblock[4];
    u -> s.s_ibmsize = (superblock[7] << 8) + superblock[6];
    u -> s.s_inode_start = (superblock[9] << 8) + superblock[8];
    u -> s.s_block_start = (superblock[11] << 8) + superblock[10];
    u -> s.s_fbm_start = (superblock[13] << 8) + superblock[12];
    u -> s.s_ibm_start = (superblock[15] << 8) + superblock[14];
    u -> s.s_flock = superblock[16];
    u -> s.s_ilock = superblock[17];
    u -> s.s_fmod = superblock[18];
    u -> s.s_ronly = superblock[19];
    u -> s.s_time[0] = (superblock[21] << 8) + superblock[20];
    u -> s.s_time[1] = (superblock[23] << 8) + superblock[22];


    return 0;
}

/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 */
void mountv6_print_superblock(const struct unix_filesystem *u)
{
    FILE* output = stdout;

    fprintf(output, "*********FS SUPERBLOCK START**********\n");
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
    /* pour la dernière ligne, je ne suis pas sûr s'il veut qu'on affiche entre [] la taille du
     tableau ou simplement les deux cases du tableau (dont la première entre crochet)*/
    //Il faudra faire la vérif dès qu'on a des valeurs qui sont sencées être différentes de 0
    //ou demander mardi prochain aux assistants
    fprintf(output, "*********FS SUPERBLOCK END**********\n");

}

/**
 * @brief umount the given filesystem
 * @param u - the mounted filesystem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(u);

    if(!fclose(u -> f)) {
        return ERR_IO;
    }

    return 0;
}
