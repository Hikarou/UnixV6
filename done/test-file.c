/**
 * @file test-file.c
 * @brief program to perform tests on files
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include <string.h>
#include "mount.h"
#include "error.h"
#include "filev6.h"
#include "inode.h"
#include "sector.h"
#include "sha.h"

//Helper to print the inode
void printTheInode(const struct unix_filesystem *u, uint16_t inr, struct filev6 *f)
{
    int error = filev6_open(u, inr, f);

    if (error >= 0) {
        printf("Printing inode #%u:\n", inr);
        inode_print(&(f -> i_node));
        if (f -> i_node.i_mode & IFDIR) {
            printf("which is a directory.\n\n");
        } else {
            uint8_t table[SECTOR_SIZE+1];
            printf("The first sector of data of which contains:\n");
            error = filev6_readblock(f, table);
            if (error < 0) return;
            table[SECTOR_SIZE] = '\0';
            printf("%s\n", table);
        }
    } else {
        printf("filve6_open failed for inode #%u\n", inr);
    }
}

int test(struct unix_filesystem *u)
{
    int err = 0;
    struct filev6 f;
    memset(&f, 255, sizeof(f));
    printf("\n");
    printTheInode(u, 3, &f);
    printTheInode(u, 5, &f);
    printf("----\n\nListing inodes SHA:\n");

    uint16_t count = 0;

    /*
    struct inode i;
    err = inode_read(u, count, &i);

    while(err == 0 || (u -> s.s_isize) * INODES_PER_SECTOR < count) {
        print_sha_inode(u, i, count);
        ++count;
        err = inode_read(u, count, &i);
    }
    // */
    struct inode inode_data[INODES_PER_SECTOR];
    for (uint32_t i = 0; i < u -> s.s_isize; ++i) {
        err = sector_read(u -> f, u -> s.s_inode_start + i, inode_data);
        if (!err) {
            for (uint16_t k = 0; k < INODES_PER_SECTOR; ++k) {
                print_sha_inode(u, inode_data[k], count);
                ++count;
            }
        }
    }
    if (err == ERR_UNALLOCATED_INODE && count > 1) {
        /* puisque le signal d'arrêt est une errur, on remet à zéro si il a bien
         * fait le travail au moins une fois
         */
        err = 0;
    }

    return err;
}
