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
#include <stdint.h>
#include <string.h>
#include "unixv6fs.h"
#include "bmblock.h"
#include "error.h"
#include "mount.h"
#include "sector.h"

/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u) {
   M_REQUIRE_NON_NULL(filename);
   M_REQUIRE_NON_NULL(u);
   memset(u, 0, sizeof(&u));
   u -> fbm = NULL;
   u -> ibm = NULL;
   u -> f = fopen(filename, "rd");
   if (u -> f == NULL) {
      return ERR_IO;
   }
   void* data = NULL;
   int returnSecRead = ERR_IO;
   returnSecRead = sector_read(u -> f, BOOTBLOCK_SECTOR, data);
   if (returnSecRead != 0) {
      return returnSecRead;
   }
   fseek(data,BOOTBLOCK_MAGIC_NUM_OFFSET,0);
   uint8_t toCheck;
   fread(&toCheck, sizeof(BOOTBLOCK_MAGIC_NUM), 1, data);
   if (toCheck !=BOOTBLOCK_MAGIC_NUM) {
      return ERR_BADBOOTSECTOR;
   }
   void* superblock=NULL;
   returnSecRead = sector_read(u -> f, SUPERBLOCK_SECTOR, superblock);
   if (returnSecRead != 0) {
      return returnSecRead;
   }
   fseek(superblock, 0, 0);
   fread(&(u -> s), SECTOR_SIZE, 1, superblock);
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
