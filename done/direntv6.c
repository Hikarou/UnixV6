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
#include <stdlib.h>
#include "error.h"
#include <string.h>
#include "direntv6.h"

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

    struct filev6 fiv6;
    int err = 0;

    err = filev6_open(u, inr, &fiv6);
    if (err!=0) {
        return err;
    }

    if(!(fiv6.i_node.i_mode & IALLOC) || !(fiv6.i_node.i_mode & IFDIR)) {
        return ERR_INVALID_DIRECTORY_INODE;
    }

    d -> fv6 = fiv6;
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

    M_REQUIRE_NON_NULL(d);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(child_inr);

    int err = 0;
    // si on est à la fin du block, on essaie de lire la suite
    if (d -> cur >= d -> last) {
        uint8_t data[SECTOR_SIZE];
        err = filev6_readblock(&(d -> fv6), data);
        if (err <= 0) {
            return err; // s'il n'y a pas de suite, => C'est la fin du fichier donc on renvoie 0
        }
        // s'il y a une suite, on regarde combien de fichiers il y a dans le dossier, et on remplit dirs
        d -> last = err/sizeof(struct direntv6);
        for (d -> cur = 0; (d -> cur) < (d -> last); ++(d -> cur)) {
            // remplir les deux champs de direntv6
            int cur = d -> cur;
            //struct direntv6 curDir = d -> dirs[cur]; Cette simplification ne marche pas... Pourquoi? TODO
            int curEntry = cur * sizeof(struct direntv6);
            (d -> dirs[cur]).d_inumber = (data[curEntry +1] << 8) + data[curEntry];
            strncpy((d -> dirs[cur]).d_name, (char*)(data + 2 + curEntry), DIRENT_MAXLEN);
        }
        d -> cur = 0;
    }

    // si on est pas à la fin du block, on lit juste le répertoire suivant

    *child_inr = (d -> dirs[d -> cur]).d_inumber;
    strncpy(name, (d -> dirs[d -> cur]).d_name, DIRENT_MAXLEN);
    name[DIRENT_MAXLEN+1] = '\0';

    ++(d -> cur);

    return 1;
}

/**
 * @brief debugging routine; print the a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix)
{

    int err = 0;
    char name[DIRENT_MAXLEN+1];
    uint16_t nextInode = 0;
    int errFake = 0;
    struct directory_reader d;
    struct directory_reader dTest;

    FILE* output = stdout;
    char* autre = NULL;

    err = direntv6_opendir(u, inr, &d);


    if (err != 0) {
        return err;
    }
    fprintf(output, "\n%s %s/", SHORT_DIR_NAME, prefix);
    do {
        err = direntv6_readdir(&d, name, &nextInode);

        if (err > 0) {

            errFake = direntv6_opendir(u, nextInode, &dTest);
            if (errFake == 0) {
                // écrire le nom de plus
                autre = malloc(strlen(prefix) + 2 + strlen(name));
                if(autre == NULL) {
                    return ERR_NOMEM;
                }
                strcpy(autre, prefix);
                strcat(strcat(autre, "/"), name);

                err = direntv6_print_tree(u, nextInode, autre);

                if (err == 0) {
                    err = 1;
                } else {
                    return err;
                }
                free(autre);

                nextInode = 0;
            } else if (errFake == ERR_INVALID_DIRECTORY_INODE) {
                fprintf(output, "\n%s %s/%s", SHORT_FIL_NAME, prefix, name);

            } else {
                err = errFake;
            }
        }


    } while (err>0);

    return err;
}

