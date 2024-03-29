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
#include "inode.h"

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
            struct direntv6 * curDir = &(d -> dirs[cur]);
            int curEntry = cur * (int)sizeof(struct direntv6);
            curDir -> d_inumber = (data[curEntry +1] << 8) + data[curEntry];
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
                fprintf(output, "%s %s/%s\n", SHORT_FIL_NAME, prefix, name);

            } else {
                err = errFake;
            }
        }


    } while (err>0);

    return err;
}

/**
 * @brief get the inode number for the given path
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param entry the pathname relative to the subtree
 * @return inr on success; <0 on error
 */
int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry)
{
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);
    int err = 0;
    size_t tailleTot = strlen(entry);
    if (tailleTot == 1 && entry [0] == '/') return ROOT_INUMBER;
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
        comp = strncmp(name_ref, name_read, taille);
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

/**
 * @brief create a new direntv6 with the given name and given mode
 * @param u a mounted filesystem
 * @param entry the path of the new entry
 * @param mode the mode of the new inode
 * @return inr on success; <0 on error
 */
int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode)
{
	M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);
    
	int err = 0;
	size_t taille = strlen(entry);
	size_t taille_nom = 0;
	int k = (int) taille;
	uint16_t child = 0;
	char* name = NULL;
	char nom_cmp[DIRENT_MAXLEN+1];
	char* path = entry;
	struct directory_reader d_parent;
	struct direntv6 dir_new;
	struct filev6 file_new;
	
	// diviser le chemin
	do {
		--k;
	} while (path[k] == '/');
	
	path[k+1] = '\0';
	
	do {
		--k;
	} while (path[k] != '/' && k >= 0);
	
	name = entry + k + 1;
	path = NULL;
	
	if (k < 1){
		path = malloc(sizeof(char)*2);
		if (path == NULL){
			return ERR_NOMEM;
		}
		path[0] = '/';
		path[1] = '\0';
	} else {
		path = malloc(sizeof(char)*(k+1));
		if (path == NULL){
			return ERR_NOMEM;
		}
		strncpy(path,entry,k);
		path[k] = '\0';
	}
	
	taille_nom = strlen(name);
	if (strlen(name) > DIRENT_MAXLEN){
		return ERR_FILENAME_TOO_LONG;
	}
	
	// vérifier que le parent existe
	err =  direntv6_dirlookup(u, ROOT_INUMBER, path);
	if (err <= 0){
		return err;
	}
	
	// on a plus besoin du path.
	free(path);
		
	// création du directory_reader
	err = direntv6_opendir(u, (uint16_t) err, &d_parent);
	if (err){
		return err;
	}
	
	// vérifier que le fils n'existe pas
	do {
		err =  direntv6_readdir(&d_parent, nom_cmp, &child);
		if (!strncmp(name, nom_cmp,taille_nom)){
			return ERR_FILENAME_ALREADY_EXISTS;
		}
	} while (err > 0);

	// Création de l'inode: (si on a le droit de le faire)
	err = inode_alloc(u);
	if (err < 0){
		return err;
	}
	
	// intialiser la structure direntv6 et filev6
	file_new.i_number = err;
	dir_new.d_inumber = err;
	strncpy(name, dir_new.d_name, DIRENT_MAXLEN);
	err = filev6_create(u, mode, &file_new);
	
	// lire le parent: il se trouve déjà dans directory_Reader: 
	return err;
}

