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
#include <string.h>
#include "mount.h"
#include "error.h"
#include "test-inodes.h"
#include "filev6.h"
#include "inode.h"
#include "sha.h"

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

//Helper to print the inode
void printTheInode(const struct unix_filesystem *u, uint16_t inr, struct filev6 *f)
{
    int error = filev6_open(u, inr, f);

    if (error >= 0) {
        printf("Printing inode #%u:\n", inr);
        struct inode i;
	error = inode_read(u, inr, &i);
	inode_print(&i, inr);
	if (i.i_mode & IFDIR) {
            printf("which is a directory.\n");
	} else {
            uint8_t table[SECTOR_SIZE+1];
     	    printf("The first sector of data of which contains:\n");
	    error = filev6_readblock(f, table);
	    table[SECTOR_SIZE] = '\0';
	    printf("%s\n", table);
	}
    } else {
        printf("filve6_open failed for inode #%u", inr);
    }
}

int main(int argc, char *argv[])
{
    // Check the number of args but remove program's name
    check_args(argc - 1);

    //TODO TEST AS ASKED
    /*
    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
        mountv6_print_superblock(&u);
        error = test(&u);
        struct filev6 fichier;
        int inodeNum = 3;

        //printf("\nEntrez le numero de l'inode: ");
        //scanf("%d", &inodeNum);

        error = filev6_open(&u,inodeNum,&fichier);

        if (error >= 0) {
            // test si c'st un répertoire
            uint8_t table[SECTOR_SIZE+1];
            printf("****CONTENU DU FICHIER****\n\n");
            error = filev6_readblock(&fichier, table);
           // while (error > 0) {
                table[SECTOR_SIZE] = '\0';
                printf("%s",table);
                error = filev6_readblock(&fichier, table);
            //}
            printf("\n %d caractères trouvés dans le fichier.\n", fichier.offset);
            error = 0;
        } else {
           printf("filev6_open failed %lu");
	}
    }

    if (error) {
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
    umountv6(&u); * shall umount even if mount failed,
                   * for instance fopen could have succeeded
                   * in mount (thus fclose required).
                   *

    return error;
    // */
    struct filev6 f;
    memset(&f, 255, sizeof(f));
    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
	mountv6_print_superblock(&u);
	test(&u);
        printTheInode(&u, 3, &f);
	printf("----\n");
	printTheInode(&u, 5, &f);
	printf("Listing inodes SHA:\n");
	//TODO
    }

    if (error) {
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
    umountv6(&u);

    return error;
}
