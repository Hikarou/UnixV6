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
            printf("which is a directory.\n\n");
        } else {
            uint8_t table[SECTOR_SIZE+1];
            printf("The first sector of data of which contains:\n");
            error = filev6_readblock(f, table);
            table[SECTOR_SIZE] = '\0';
            printf("%s\n", table);
        }
    } else {
        printf("filve6_open failed for inode #%u\n", inr);
    }
}

int main(int argc, char *argv[])
{
    // Check the number of args but remove program's name
    check_args(argc - 1);

    struct filev6 f;
    memset(&f, 255, sizeof(f));
    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u);
    if (error == 0) {
        mountv6_print_superblock(&u);
        printf("\n");
        printTheInode(&u, 3, &f);
        printTheInode(&u, 5, &f);
        printf("----\n\nListing inodes SHA:\n");

        uint16_t count = 1;

        struct inode i;
        error = inode_read(&u, count, &i);

        while (error == 0 || (u.s.s_isize)*INODES_PER_SECTOR < count) {
            print_sha_inode(&u, i, count);
            ++count;
            error = inode_read(&u, count, &i);
        }
        if (error == ERR_UNALLOCATED_INODE && count >1) {
           /*  puisque le signal d'arrêt est
            une erreur, on remet à zero l'errueur car dans ce cas ce n'en est pas une */
            error = 0;
        }
    }

    if (error) {
        fprintf(stderr, "Error : ");
        puts(ERR_MESSAGES[error - ERR_FIRST]);
    }
    umountv6(&u);

    return error;
}
