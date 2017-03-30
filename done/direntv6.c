#pragma once

/**
 * @file direntv6.c
 * @brief accessing the UNIX v6 filesystem -- directory layer
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date mars 2017
 */

#include <stdint.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"

#define MAXPATHLEN_UV6 ((uint16_t)1024)

/**
 * @brief opens a directory reader for the specified inode 'inr'
 * @param u the mounted filesystem
 * @param inr the inode -- which must point to an allocated directory
 * @param d the directory reader (OUT)
 * @return 0 on success; <0 on errror
 */
int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(d);
	
    struct filev6 fv6;
    int err = 0;

    err = filev6_open(u, inr, &fv6);
    if (err!=0) {
        return err;
    }

    //C'est pas plutôt un && au lieu du || ?
    //Du coup, j'ai remplacé
    //if ((fv6.i_node.imode & IALLOC) || !(fv6.i_node.imode & IFDIR)) {
    //par
    if(!(fv6.i_node.imode & IFDIR))
    {
        return ERR_INVALID_DIRECTORY_INODE;
    }

    d -> fv6 = fv6;
    d -> cur = 0;
    d -> last = 0;

    return 0;

}

/**
 * @brief return the next directory entry.
 * @param d the directory reader
 * @param name pointer to at least DIRENTMAX_LEN+1 bytes. 
          Filled in with the NULL-terminated string of the entry (OUT)
 * @param child_inr pointer to the inode number in the entry (OUT)
 * @return 1 on success; 0 if there are no more entries to read; <0 on error
 */
int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr)
{
    //Dans l'énoncé, il est dit de name que ce nom devrait être correct donc toujours terminer par 
    //le charactère nul. Est-ce que ça veut dire qu'il faut partir du principe que ce sera toujours
    //le cas où pas ?

    M_REQUIRE_NON_NULL(d);
    M_REQUIRE_NON_NULL(name);

    if (strlen(name) <= DIRENT_MAXLEN)
    {
        return ERR_BAD_PARAMETER;
    }

    M_REQUIRE_NON_NULL(child_inr);

    int err = 0;

    // lire le secteur _: directory_reader -> filev6 -> inode: rad block
    // slocker le readblock dans dirs
    // last combien de sous fichier dans le secteur
    // cur doit valoir 0.


    // tant que readblock ne renvoie pas zero il faut encore lire des secteurs


    // lire le dir qui  à la position cur, prendre son nom et son nom dans la structure direntv6

    if (d -> cur == d -> last) {
	uint8_t data[SECTOR_SIZE];
        err = filev6_readblock(&(d -> fv6), data);
	if (err <= 0) {
            return err;
	}
        d -> cur = 0;
	d -> last = err/sizeof(struct direntv6);
	for (int i = 0; i < d -> last; ++i) {
            //TODO A finir
	    //Je ne sais pas du tout si c'est correct cette partie
	    
            for (int j = 0; j < DIRENT_MAXLEN; ++j) {
                f -> dirs[i] -> d_name[j] = data[i * DIRENT_MAXLEN + j];
	    }
	    //Pour tester et voir ce qu'il se trouve ici
	    printf("%s", *(f -> dirs[i] -> d_name));
	}
    }


    return 1;
}

/**
 * @brief debugging routine; print the a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix) {
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(u);

    struct directory_reader d;

    int err = 0;

    err = direntv6_opendir(u, inr, &d);
    if (err < 0) {
        return err;
    }
    char name [DIRENT_MAXLEN + 1];
    memset(name, '\0', v, DIRENT_MAXLEN);
    uint16_t child_inr = 0;
    err = direntv6_readdir(&d, &name, &child_inr);
}
