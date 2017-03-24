/**
 * @file test-file.c
 * @brief program to perform tests on files
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include <stdlib.h>
#include <stdio.h>
#include "mount.h"
#include "error.h"
#include "test-inodes.h"
#include "filev6.h"

#define MIN_ARGS 1
#define MAX_ARGS 1
#define USAGE    "test <diskname>"

void error(const char* message)
{
    fputs(message, stderr);
    putc('\n', stderr);
    fputs("Usage: " USAGE, stderr);
    putc('\n', stderr);
    exit(1);
}

void check_args(int argc)
{
    if (argc < MIN_ARGS) {
        error("too few arguments:");
    }
    if (argc > MAX_ARGS) {
        error("too many arguments:");
    }
}

int main(int argc, char *argv[])
{
    // Check the number of args but remove program's name
    check_args(argc - 1);

    //TODO TEST AS ASKED
    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
        mountv6_print_superblock(&u);
        error = test(&u);
        struct filev6 fichier;
        int inodeNum = 0;
        
        printf("\nEntrez le numero de l'inode: ");
    	scanf("%d", &inodeNum);
        
        error = filev6_open(&u,inodeNum,&fichier);
        
        if (error >= 0){
        	// test si c'st un répertoire
        	uint8_t table[SECTOR_SIZE+1];
        	printf("****CONTENU DU FICHIER****\n\n");
        	error = filev6_readblock(&fichier, table);
        	while (error > 0) {
		    			    	

	    		table[SECTOR_SIZE] = '\0';
	    		printf("%s",table);
		    	error = filev6_readblock(&fichier, table);
        	}
        	
        	printf("\n %d caractères trouvés dans le fichier.\n", fichier.offset);
        	error = 0;
        }	
    }
    
    if (error) {
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
    umountv6(&u); /* shall umount even if mount failed,
                   * for instance fopen could have succeeded
                   * in mount (thus fclose required).
                   */

    return error;
}
