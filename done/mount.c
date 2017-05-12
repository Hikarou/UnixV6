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
    struct inode inode;
    uint8_t data[SECTOR_SIZE];
    uint64_t actu = 0;
    uint64_t i = 0;
    uint16_t k = 0;

    for (uint64_t j = u -> ibm -> min; j < u -> ibm -> max; ++j) { // tout effacer
        bm_clear(u -> ibm, j);
    }

    while (actu < u -> ibm -> max) { // pour chaque secteur
        int err = sector_read(u -> f, u -> s.s_inode_start + i, data);
        for (k = 0; k < INODES_PER_SECTOR; ++k) { // pour chaque inode
            if (!err) { // si pas d'erreur de lecture
                if ((actu >= u -> ibm -> min) && (actu <= u -> ibm -> max)) {
                    inode.i_mode = (uint16_t)((data[k * 32 + 1] << 8) + data[k * 32]);
                    if (inode.i_mode & IALLOC) {
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
        ++i;
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
    int32_t offset = 0;
    struct inode inode;

    // mettre tous les secteurs à libre
    for (uint64_t i = u -> fbm -> min; i < u -> fbm -> max; ++i) {
        bm_clear(u->fbm, i);
    }

    // pour chaque inode: appeler inode find sector
    for (uint64_t i = u -> ibm -> min - 1; i < u -> ibm -> max; ++i) {
        offset = 0;
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

    if(!fclose(u -> f)) {
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
	if (num_inodes <= 1){
		return ERR_NOT_ENOUGH_BLOCS;
	}
	
	int err = 0;
	uint8_t superblock[SECTOR_SIZE];
	uint8_t sector[SECTOR_SIZE];
	FILE* fichier = NULL;
	uint16_t nb_bin_petit = (1<<8)-1;
	uint16_t nb_bin_grand = -1 - nb_bin_petit;
	struct inode inode;
	
	
	memset(superblock,0,SECTOR_SIZE);
	memset(sector,0,SECTOR_SIZE);
	
	//Calcul des valeur de superblock et tests
	uint16_t s_fsize = num_blocks;	    /* size in sectors of entire volume */
	uint16_t s_isize = num_inodes/INODES_PER_SECTOR;    	/* size in sectors of the inodes */
	
	if (num_blocks <= 2 + s_isize + num_inodes + 1){
		return ERR_NOT_ENOUGH_BLOCS;
	}
	
	//créer superblock
	
	uint16_t s_fbmsize = 0;      /* size in sectors of the freelist bitmap */
    uint16_t s_ibmsize = 0;      /* size in sectors of the inode bitmap */
    uint16_t s_inode_start = SUPERBLOCK_SECTOR + 1;  /* first sector with inodes */
    uint16_t s_block_start = SUPERBLOCK_SECTOR + s_isize + 1;  /* first sector with data */
    uint16_t s_fbm_start = 0;    /* first sector with the freebitmap (==2) */
    uint16_t s_ibm_start = 0;    /* first sector with the inode bitmap */
    uint8_t s_flock = 0;	    /* lock during free list manipulation */
    uint8_t s_ilock = 0;	    /* lock during I list manipulation */
    uint8_t s_fmod = 0;		    /* super block modified flag */
    uint8_t s_ronly = 0;	    /* mounted read-only flag */ // faut-il mettre quelque chose de particulier ici?
    uint16_t s_time[2] = {0,0};	    /* current date of last update */
   
   
   	//s.s_isize = (superblock[1] << 8) + superblock[0];
   	superblock[0] = (uint8_t) (s_isize & nb_bin_petit);
   	superblock[1] = (uint8_t) ((s_isize & nb_bin_grand) >> 8);
    //u -> s.s_fsize = (superblock[3] << 8) + superblock[2];
    superblock[2] = (uint8_t) (s_fsize & nb_bin_petit);
    superblock[3] = (uint8_t) ((s_fsize & nb_bin_grand) >> 8); 
    //u -> s.s_fbmsize = (superblock[5] << 8) + superblock[4];
    superblock[4] = (uint8_t) (s_fbmsize & nb_bin_petit);
    superblock[5] = (uint8_t) ((s_fbmsize  & nb_bin_grand) >> 8);
    //u -> s.s_ibmsize = (superblock[7] << 8) + superblock[6];
    superblock[6] = (uint8_t) (s_ibmsize & nb_bin_petit);
    superblock[7] = (uint8_t) ((s_ibmsize & nb_bin_grand) >> 8);
    //u -> s.s_inode_start = (superblock[9] << 8) + superblock[8];
    superblock[8] = (uint8_t) (s_inode_start & nb_bin_petit);
    superblock[9] = (uint8_t) ((s_inode_start & nb_bin_grand) >> 8);
   // u -> s.s_block_start = (superblock[11] << 8) + superblock[10];
    superblock[10] = (uint8_t) (s_block_start & nb_bin_petit);
    superblock[11} = (uint8_t) ((s_block_start & nb_bin_grand) >> 8);
    //u -> s.s_fbm_start = (superblock[13] << 8) + superblock[12];
    superblock[12] = (uint8_t) (s_fbm_start & nb_bin_petit):
    superblock[13] = (uint8_t) ((s_fbm_start & nb_bin_grand) >> 8);
    //u -> s.s_ibm_start = (superblock[15] << 8) + superblock[14];
    superblock[14] = (uint8_t) (s_ibm_start & nb_bin_petit);
    superblock[15] = (uint8_t) ((s_ibm_start & nb_bin_grand) >> 8);
    superblock[16] = s.s_flock;
   	superblock[17] = s_ilock;
    superblock[18] = s_fmod;
    superblock[19] = s_ronly;
    //u -> s.s_time[0] = (superblock[21] << 8) + superblock[20];
	superblock[20] = (uint8_t) (s_time[0] & nb_bin_petit);
    superblock[21] = (uint8_t) ((s_time[0] & nb_bin_grand) >> 8);
    //u -> s.s_time[1] = (superblock[23] << 8) + superblock[22];
	superblock[22] = (uint8_t) (s_time[1] & nb_bin_petit);
	superblock[23] = (uint8_t) (s_time[1] & nb_bin_grand) >> 8);
	
	// créer un fichier binaire du bon nom et le remplr de zeros juqu'à la bonne taille
	fichier = fopen(filename, "wb");
	if (fichier == NULL){
		return ERR_IO;
	}
	
	// écrire le bootsector et le superblock 
	err = fseek(fichier,0,SEEK_SET);
	if (err){
		return err;
	}
	
	sector[BOOTBLOCK_MAGIC_NUM_OFFSET] = BOOTBLOCK_MAGIC_NUM; 
	err = sector_write(fichier, 0, sector);
	if (err){
		return err;
	}
	err = sector_write(fichier, 1, superblock);
	if (err){
		return err;
	}
	
	memset(sector,0,SECTOR_SIZE);
	
	// écrire inode_root 
	
	inode.i_mode = IALLOC | IFDIR;
    inode.i_nlink = 0;
    inode.i_uid = 0;
    inode.i_gid = 0;
    
    inode.i_size0 = 0;
    inode.i_size1 = 0;
    
    inode.i_addr[0] = s_block_start;
    for (size_t i = 1; i<ADDR_SMALL_LENGTH; ++i) {
    	inode.i_addr[i] = 0;
    }
    
    for (size_t i = 0; i<2; ++i) {
    	inode.atime[i] = 0;
        inode.mtime[i] = 0;
    } 	
 	
 	
	sector[ROOT_INUMBER*32] = (uint8_t) (inode -> i_mode & nb_bin_petit);
	sector[ROOT_INUMBER*32+1] = (uint8_t) ((inode -> i_mode & nb_bin_grand) >> 8);
    sector[ROOT_INUMBER*32+2] = (uint8_t) (inode -> i_nlink);
    sector[ROOT_INUMBER*32+3] = (uint8_t) (inode -> i_uid);
    sector[ROOT_INUMBER*32+4] = (uint8_t) (inode -> i_gid);
    sector[ROOT_INUMBER*32+5] = (uint8_t) (inode -> i_size0);
    sector[ROOT_INUMBER*32+6] = (uint8_t) (inode -> i_size1 & nb_bin_petit);
	sector[ROOT_INUMBER*32+7] = (uint8_t) ((inode -> i_size1 & nb_bin_grand) >> 8);
    
    for (size_t i = 0; i<ADDR_SMALL_LENGTH; ++i) {
        sector[ROOT_INUMBER*32+8+2*i] = (uint8_t) ((inode -> i_addr[i] & nb_bin_petit));
		sector[ROOT_INUMBER*32+9+2*i] = (uint8_t) ((inode -> i_addr[i] & nb_bin_grand) >> 8);
    }
    for (size_t i = 0; i<2; ++i) {
        sector[ROOT_INUMBER*32+24+2*i] = (uint8_t) (inode -> atime[i] & nb_bin_petit);
		sector[ROOT_INUMBER*32+25+2*i] = (uint8_t) ((inode -> atime[i] & nb_bin_grand) >> 8);
   		sector[ROOT_INUMBER*32+28+2*i] = (uint8_t) (inode -> mtime[i] & nb_bin_petit);
		sector[ROOT_INUMBER*32+29+2*i] = (uint8_t) ((inode -> mtime[i] & nb_bin_grand) >> 8);
    }
    
    err = sector_write(fichier, s_inode_start, sector);
	if (err){
		return err;
	}
	
	memset(sector,0,SECTOR_SIZE);
	// faire une boucle sur tous les secteurs des inodes 
	
	// faire une boucle sur tous les secteurs des data
	
	return err;
}
