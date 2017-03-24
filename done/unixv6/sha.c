#pragma once

/**
 * @file sha.c
 * @brief helpers for the project -- SHA-related part
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include <stdio.h>
#include <openssl/sha.h>
#include "mount.h"
#include "unixv6fs.h"
#include "sha.h"
#include "inode.h"
#include "filev6.h"

/**
 * @brief transforms an array of chars into something readable given by the exercise
 * @param SHA the sha unreadable (IN)
 * @param sha_string the sha readable (OUT)
 */
static void sha_to_string(const unsigned char *SHA, char *sha_string)
{
    if ((SHA == NULL) || (sha_string == NULL)) {
        return;
    }

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i * 2], "%02x", SHA[i]);
    }

    sha_string[2 * SHA256_DIGEST_LENGTH] = '\0';
}

/**
 * @brief print the sha of the content
 * @param content the content of which we want to print the sha
 * @param length the length of the content
 */
void print_sha_from_content(const unsigned char *content, size_t length)
{
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(content, length, sha);
    char sha_string[2*SHA256_DIGEST_LENGTH+1];
    sha_to_string(sha, sha_string);
    printf(sha_string);
}

/**
 * @brief print the sha of the content of an inode
 * @param u the filesystem
 * @param inode the inocde of which we want to print the content
 * @param inr the inode number
 */
void print_sha_inode(struct unix_filesystem *u, struct inode inode, int inr)
{
    //Need to check if inode is valid
    if (inode.i_mode & IALLOC) {
        return;
    }
    printf("SHA inode %d: ", inr);
    if (inode.i_mode & IFDIR) {
        printf("no SHA for directories.");
    } else {
        uint8_t content[inode_getsize(&inode)*+1];
        uint8_t subcontent[SECTOR_SIZE+1];
        int length = 0;
        struct filev6 f;
        int error = filev6_open(u, inr, &f);
        //getting all the content from inode
        do {
            error = filev6_readblock(&f, subcontent);
            for(int i = 0; i < error; ++i) {
                content[length + i] = subcontent[i];
            }
            length += error;
        } while(error>= 0);
        print_sha_from_content(content, length);
    }
}
