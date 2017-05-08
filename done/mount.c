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
	int err = 0;
	struct inode inode;
	uint8_t data[SECTOR_SIZE];
	uint64_t actu = 0; 
	uint64_t i = 0;
	uint16_t k = 0;
	
	for (uint64_t j = u -> ibm -> min; j < u -> ibm -> max; ++j){ // tout effacer
		bm_clear(u -> ibm, j);
	}
	
	while (actu < u -> ibm -> max) { // pour chaque secteur
        err = sector_read(u -> f, u -> s.s_inode_start + i, data);
        for (k = 0; k < INODES_PER_SECTOR; ++k) { // pour chaque inode
            if (!err){ // si pas d'erreur de lecture
            	if ((actu >= u -> ibm -> min) && (actu <= u -> ibm -> max)){ 
				    inode.i_mode = (uint16_t)((data[k * 32 + 1] << 8) + data[k * 32]);
				    if (inode.i_mode & IALLOC) {
						bm_set(u -> ibm, actu);
				    }
				    else{
				    	bm_clear(u -> ibm, actu);
				    }
		        }
            }
            else{// si une erreur de lecture
            	bm_set(u -> ibm, actu);
            }
            ++actu;
        }
        ++i;
    }
}

/**
 * @brief  fill the vector bitmap of the sectors 
 * u - the mounted filesystem
 */
void fill_fbm(struct unix_filesystem * u)
{
	int err = 0;
	int taille = 0;
	int taille_grand = 0;
	int32_t offset = 0;
	struct inode inode;
	
	// mettre tous les secteurs à libre
	for (uint64_t i = u -> fbm -> min; i < u -> fbm -> max; ++i){
		bm_clear(u->fbm, i);
	}
	
	// en supposant que le premier secteur est toujours plein (par la root)
	/*err = inode_read(u, ROOT_INUMBER, &inode);
	taille = inode_getsize(&inode)/SECTOR_SIZE;
	if (!err){
		err =  inode_findsector(u, &inode, offset);
		while (offset <= taille && err > 0) {
			printf("    secteur: %d : rempli. offset = %d, taille = %d\n", err, offset, taille);		
			bm_set(u->fbm, err);
			++offset;
			err =  inode_findsector(u, &inode, offset);
		}
		if (err < 0){
			  puts(ERR_MESSAGES[err - ERR_FIRST]);
		}
	}
	else{
		printf("ERROR reading ROOT SECTOR\n");
		puts(ERR_MESSAGES[err - ERR_FIRST]);
	}*/
	
	// pour chaque inode: appeler inode find sector
	for (uint64_t i = u -> ibm -> min; i < u -> ibm -> max; ++i){
		offset = 0;
		err = bm_get(u -> ibm, i);
		
		if (err == 1){
			printf("i = %d, utilisé: %d\n",i, err);
			err = inode_read(u, i, &inode);
			if (!err){
				taille = inode_getsize(&inode)/SECTOR_SIZE;	
				if (taille < 7*ADDRESSES_PER_SECTOR && taille > 7) {
					printf("Long fichier, secteurs: ");
					taille_grand = taille/ADDRESSES_PER_SECTOR;
					for (int k = 0; k <= taille_grand; ++k){
						printf("%d ", inode.i_addr[k]);
						bm_set(u -> fbm, inode.i_addr[k]);
					}
					printf("\n");
				}
				
				err =  inode_findsector(u, &inode, offset);
				while (offset <= taille && err > 0) {
					printf("    secteur: %d : rempli. offset = %d, taille = %d\n", err, offset, taille);		
					bm_set(u -> fbm, err);
					++offset;
					err =  inode_findsector(u, &inode, offset);
				}
				if (err < 0){
					printf("ERROR in inode_findsector\n"); 
					puts(ERR_MESSAGES[err - ERR_FIRST]);
				}	
			}
			else{
				printf("ERROR unable to read inode %d\n", i);
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

	u -> fbm = NULL;
	u -> ibm = NULL;
	// plus 0 pour se souvenir TODO
	u -> fbm = bm_alloc((uint64_t) (u -> s.s_block_start + 0), (uint64_t) u -> s.s_fsize);
	u -> ibm = bm_alloc((uint64_t) (ROOT_INUMBER + 0), (uint64_t) (u -> s.s_isize)*INODES_PER_SECTOR);
	
	if (u -> ibm == NULL ||u -> fbm == NULL ){
		return ERR_NOMEM;
	}
	
	fill_ibm(u);
	fill_fbm(u);	
	
	bm_print(u -> fbm);
	bm_print(u -> ibm);	
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

    if(!fclose(u -> f)) {
        return ERR_IO;
    }

    return 0;
}
