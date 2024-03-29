/**
 * @file direntv6.c
 * @brief accessing the UNIX v6 filesystem -- directory layer
 *
 * @date mars 2017
 */
#include <stdint.h>
#include <stdio.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"
#include <stdlib.h>
#include "error.h"
#include <string.h>
#include "direntv6.h"

//#define MAXPATHLEN_UV6 10

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
        d -> last = err / (int)sizeof(struct direntv6);
        for (d -> cur = 0; (d -> cur) < (d -> last); ++(d -> cur)) {
            // remplir les deux champs de direntv6
            int cur = d -> cur;

            // correcteur : il aurait été bien plus simple de faire vos filev6_readblock dans un tableau de struct direntv6
            struct direntv6 * curDir = &(d -> dirs[cur]);
            int curEntry = cur * (int)sizeof(struct direntv6);
            curDir -> d_inumber = (data[curEntry +1] << 8) + data[curEntry];
            
            // correcteur : [WRONG_EXECUTION] après l'appel à strncpy vous devriez mettre curDir->d-name[DIRENT_MAXLEN - 1] à \0, car si vous copiez le nombre maximum de caractère, le \0 sera écrasé
            /* exemple: 
            a[5] = abcd\0
            b[10] = efghijklm\0
            strncpy(a, b, 5)
            => a vaudra 'efghi' (non terminé par un \0)

            cf le cours
            */
            strncpy(curDir -> d_name, (char*)(data + 2 + curEntry), DIRENT_MAXLEN);
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
    char name[DIRENT_MAXLEN+1] = "";
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
    fprintf(output, "%s %s/\n", SHORT_DIR_NAME, prefix);
    do {
        err = direntv6_readdir(&d, name, &nextInode);

        if (err > 0) {

            errFake = direntv6_opendir(u, nextInode, &dTest);
            if (errFake == 0) {
                // écrire le nom de plus
                // correcteur : [USELESS_DYNALLOC_2] vous n'avez pas besoin d'un malloc ici ! Un tableau statique aurait parfaitement convenu ici et vous aurait évité des erreurs (voir plus bas)
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
                    // correcteur : vous devez free(autre) ici
                    return err;
                }
                free(autre);

                nextInode = 0;
            } else if (errFake == ERR_INVALID_DIRECTORY_INODE) {
                fprintf(output, "%s %s/%s\n", SHORT_FIL_NAME, prefix, name);

            } else {
                err = errFake;
            }
        }


    } while (err>0);

    return err;
}


// correcteur : [S] cette fonction n'a aucun commentaire expliquant comment vous l'avez implémenté, ce qui la rend dur à comprendre pour une personne extérieure comme moi
int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);
    int err = 0;
    size_t tailleTot = strlen(entry);
    size_t taille = 0;
    size_t shiftTaille = 0;
    size_t k = 0;
    uint16_t inr_next = 0;
    char* name_ref;
    char name_read[DIRENT_MAXLEN+1] = "";

    // Nom du futur dossier:
    do {
        if (entry[k] == '/') {
            if (taille > 0) {
                shiftTaille = k-taille;
                k = tailleTot;
                ++taille; // taille contient déjà le \0
            }
        } else {
            if (k == tailleTot-1) {
                shiftTaille = k-taille;
            }
            ++taille;
        }
        ++k;
    } while (k<tailleTot);

    if (taille == 0) { // il n'y a que des /
        return 0;
    }

    if (taille + shiftTaille == tailleTot) {
        ++taille;
    }

    // correcteur : [USELESS_DYNALLOC] pas besoin d'allocation dynamique ici, un tableau char name_ref[taille] aurait suffi et aurait été plus simple
    name_ref = malloc((taille)*sizeof(char));
    if (name_ref == NULL) {
        return ERR_NOMEM;
    }


    for (size_t i = 0; i< taille-1; ++i) {
        name_ref[i] = entry[shiftTaille + i];
    }
    name_ref[taille-1] = '\0';

    // Recherche du futur dossier ou fichier
    struct directory_reader d;
    err = direntv6_opendir(u, inr, &d);
    if (err<0) {
        free(name_ref);
        return err;
    }

    int comp = 0;
    do {
        err = direntv6_readdir(&d, name_read, &inr_next);
        comp = strncmp(name_ref, name_read, taille-1);
    } while (err > 0 && comp);

    if (err < 0) {
        free(name_ref);
        return err;
    }

    if (comp != 0) {
        free(name_ref);
        return ERR_IO;
    }

    // Ouvrir le prochain dossier ou retourner l'inode number
    if (tailleTot > shiftTaille + taille) { // il faut encore lire un dossier
        err = direntv6_dirlookup(u, inr_next, entry+taille+shiftTaille);

        if (err == 0) {
            err = (int) inr_next;
        }
    } else { // on a le bon fichier
        err = (int) inr_next;
    }

    free(name_ref);

    return err;
}

