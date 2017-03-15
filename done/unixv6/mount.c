#pragma once

/**
 * @file mount.h
 * @brief accessing the UNIX v6 filesystem -- core of the first set of assignments
 *
 * @author José Ferro Pinto
 * @author Marc Favrod
 * @date mars 2017
 */

#include <stdio.h>
#include <string.h>
#include "unixv6fs.h"
#include "bmblock.h"
#include "error.h"

/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u) {
   M_REQUIRE_NON_NULL(filename);
   M_REQUIRE_NON_NULL(unix_filesystem);
   memset(u, 0, sizeof(&u));
   u -> fbm = NULL;
   u -> ibm = NULL;
   u -> f = fopen(filename, "rd");
   if (u -> f == NULL) {
      return ERR_IO;
   }
   void* data;
   int returnSecRead = sector_read(u -> f, BOOTBLOCK_SECTOR, data);
   if (returnSecRead != 0) {
      return returnSecRead;
   }
   //travailler avec le data... Besoin de plus d'infos pour la manipulation de void*
   //fseek(data,BOOTBLOCK_MAGIC_NUM_OFFSET,0)
   returnSecRead = sector_read(u -> f, SUPERBLOCK_SECTOR, u -> s);
   if (returnSecRead != 0) {
      return returnSecRead;
   }

   return 0;
}

/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 */
void mountv6_print_superblock(const struct unix_filesystem *u);

/**
 * @brief umount the given filesystem
 * @param u - the mounted filesytem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u);
