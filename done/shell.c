/**
 * @file shell.c
 * @brief Small shell to use the function implemented
 *
 * @author José Ferro Pinto
 * @author Marc Favrod-Coune
 * @date avril 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mount.h"
#include "sector.h"
#include "direntv6.h"
#include "error.h"
#include "inode.h"
#include "sha.h"

#define MAX_READ 255
#define NB_CMDS 13
#define ERR_OK 0
#define EXIT 1
#define ERR_ARGS 2
#define ERR_NOT_MOUNTED 3
#define ERR_FS 4
#define NOT_IMPLEMENTED 5
#define NB_ARGS 1

struct unix_filesystem u;

typedef int (*shell_fct)(char** fct);

struct shell_map {
    const char* name;    // nom de la commande
    shell_fct fct;       // fonction réalisant la commande
    const char* help;    // description de la commande
    size_t argc;         // nombre d'arguments de la commande
    const char* args;    // description des arguments de la commande
};

int do_exit();

int do_help();

int do_mount(char**);

int do_lsall();

int do_psb();

int do_cat(char**);

int do_sha(char**);

int do_inode (char**);

int do_istat(char**);

int do_mkfs(char**);

int do_mkdir(char**);

int do_add(char**);

int tokenize_input (char*, char***, int*);

struct shell_map shell_cmds[] = {
    {"help", do_help, "display this help.", 0, NULL},
    {"exit", do_exit, "exit shell.", 0, NULL},
    {"quit", do_exit, "exit shell.", 0, NULL},
    {"mkfs", do_mkfs, "create a new filesystem.", 3, "<diskname> <#inodes> <#blocks>"},
    {"mount", do_mount, "mount the provided filesystem.", 1, "<diskname>"},
    {"mkdir", do_mkdir, "create a new directory.", 1, "<dirname>"},
    {"lsall", do_lsall, "list all directories and files contained in the currently mounted filesystem.", 0, ""},
    {"add", do_add, "add a new file.", 2, "<src-fullpath> <dst>"},
    {"cat", do_cat, "display the content of a file.", 1, "<pathname>"},
    {"istat", do_istat, "display information about the provided inode.", 1, "<inode_nr>"},
    {"inode", do_inode, "display the inode number of a file.", 1, "<pathname>"},
    {"sha", do_sha, "display the SHA of a file.", 1, "<pathname>"},
    {"psb", do_psb, "Print SuperBlock of the currently mounted filesystem.", 0, NULL}
};

int main()
{
    char input[MAX_READ+1] = "";
    char** parsed = NULL;
    int err = 0;
    int size_parsed = 0;
    int k = 0;
    shell_fct function = NULL;

    while ((!feof(stdin) && !ferror(stdin)) && err != EXIT) {
        size_parsed = 0;
        k = 0;
        err = 0;

        /* ici on crée un tableau de chaine de caractères: il est dynamique car sa taille
         * peut changer dans la fonction tokenize_input. Il est vrai qu'on aurait pu faire
         * un tableau statique de 3 éléments mais pour plusieurs fonctions, ce tableau ne
         * serait que partiellement rempli.
         */
        parsed = calloc(NB_ARGS, sizeof(char*));
        if (parsed != NULL) {
            fgets(input, MAX_READ, stdin);
            int position_last_char = strlen(input) - 1;

            if (input[position_last_char] == '\n') {
                input[position_last_char] = '\0';
            }
            input[MAX_READ] = '\0';
            // take care if end of file or error in file
            if (feof(stdin) || ferror(stdin)) {
                err = EXIT;
            } else {
                err = tokenize_input(input, &parsed, &size_parsed);
                if (err == ERR_OK) {
                    //finding the function asked
                    do {
                        err = strcmp(parsed[0], shell_cmds[k].name);
                        ++k;
                    } while (k < NB_CMDS && err != 0);
                    --k;
                    if (err == 0) {
                        function = shell_cmds[k].fct;

                        if ((shell_cmds[k]).argc == (size_t)size_parsed-1) {
                            err = function(parsed);
                            if (err < 0) {
                                printf("ERROR FS: ");
                                puts(ERR_MESSAGES[err - ERR_FIRST]);
                                err = ERR_FS;
                            }
                        } else {
                            printf("ERROR SHELL: wrong number of arguements\n");
                            err = ERR_ARGS;
                        }
                    } else {
                        printf("ERROR SHELL: Invalid command\n");
                        err = ERR_ARGS;
                    }
                }
            }
            free(parsed);
        } else {
            err = ERR_NOMEM;
            puts(ERR_MESSAGES[err - ERR_FIRST]);
            err =  EXIT;
        }
    }

    if (u.f != NULL) {
        umountv6(&u);
    }

    return 0;
}

/**
 * @brief tokenize the input given
 * @param input the input to tonkenize (IN)
 * @param parsed the input tokenized (OUT)
 * @param size_parsed the length of parsed
 * @return 0 on success; >0 on error
 */
int tokenize_input (char* input, char*** parsed, int* size_parsed)
{
    char* ptr = NULL;
    int size = NB_ARGS;

    int k = 1;
    int i = 0;
    int l = strlen(input);

    *(size_parsed) = 0;

    if (input == NULL || parsed == NULL) {
        return ERR_ARGS;
    }

    if (l <= 0) {
        return ERR_ARGS;
    }

    ptr = input;
    do {
        if (k == 0) { // si on est dans du texte
            if (input[i] == ' ') {
                k = 1;
                if (i > 0) {
                    input[i] = '\0';
                    if (*size_parsed > size-1) {
                        ++size;
                        *parsed = realloc(*parsed, sizeof(char*) * size);
                        if (*parsed == NULL) return EXIT;
                    }
                    (*parsed)[*(size_parsed)] = ptr;
                    *(size_parsed) = *(size_parsed) + 1;
                }
            }
        } else if (k == 1) { // si on est dans des espaces
            if (input[i] == ' ') {
                k = 1;
            } else {
                ptr = input + i;
                k = 0;
            }
        }
        ++i;
    } while (i < l);


    if (k == 0) {
        if (*size_parsed > size-1) {
            ++size;
            *parsed = realloc(*parsed, sizeof(char) * size * MAX_READ);
            if (*parsed == NULL) return EXIT;
        }
        (*parsed)[*(size_parsed)] = ptr;
        *(size_parsed) = *(size_parsed) + 1;
    }

    return 0;
}


int do_exit()
{
    return EXIT;
}

int do_help()
{
    for (int i = 0; i < NB_CMDS; ++i) {
        printf("- %s", shell_cmds[i].name);
        if (shell_cmds[i].argc > 0) {
            printf(" %s", shell_cmds[i].args);
        }
        printf(": %s\n", shell_cmds[i].help);
    }
    return ERR_OK;
}

int do_mount(char** args)
{

    int err = 0;
    if (u.f != NULL) {
        err = umountv6(&u);
    }

    if (err != 0) return err;

    u.f = NULL;

    err = mountv6(args[1], &u);

    if (err < 0) {
        return err;
    }

    return ERR_OK;
}

int do_lsall()
{
    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    int err = direntv6_print_tree(&u, ROOT_INUMBER, "");

    if (err < 0) {
        return err;
    }

    return ERR_OK;
}

int do_psb()
{
    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    mountv6_print_superblock(&u);
    return ERR_OK;
}

int do_cat(char** args)
{
    int inode_nb = 0;
    int err = 0;
    struct filev6 file;
    char content[SECTOR_SIZE+1] = "";

    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    inode_nb = direntv6_dirlookup(&u, ROOT_INUMBER, args[1]);
    if (inode_nb < 0) {
        return inode_nb;
    }

    err = filev6_open(&u, inode_nb, &file);
    if (err < 0) {
        return err;
    }

    if (file.i_node.i_mode & IFDIR) {
        printf("ERROR SHELL: cat on a directory is not defined\n");
        return ERR_ARGS;
    }

    do {
        err = filev6_readblock(&file, content);
        if (err > 0) {
            printf("%s", content);
        } else {
            printf("\n");
        }
    } while (err > 0);

    return ERR_OK;
}

int do_sha(char** args)
{

    int inode_nb = 0;
    int err = 0;
    struct filev6 file;

    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    inode_nb = direntv6_dirlookup(&u, ROOT_INUMBER, args[1]);
    if (inode_nb < 0) {
        return inode_nb;
    }

    err = inode_read(&u, inode_nb, &(file.i_node));

    if (err < 0) {
        return err;
    }

    // Allocation test already done in inode_read
    print_sha_inode(&u, file.i_node, (int) inode_nb);
    printf("\n");

    return ERR_OK;
}

int do_inode (char** args)
{
    int inode_nb = 0;

    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    inode_nb = direntv6_dirlookup(&u, ROOT_INUMBER, args[1]);
    if (inode_nb < 0) {
        return inode_nb;
    }

    printf("inode: %d\n", inode_nb);

    return ERR_OK;
}

int do_istat(char** args)
{
    int err = 0;

    struct inode i;

    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    int inr = atoi(args[1]);
    if (inr < 0) {
        printf("ERROR SHELL: inode out of range\n");
        return ERR_ARGS;
    }

    err = inode_read(&u, inr, &i);
    if (err < 0) {
        return err;
    }

    inode_print(&i);

    return ERR_OK;
}

int do_mkfs(char** args)
{
    int err = 0;
    uint16_t num_blocks = 0;
    uint16_t num_inodes = 0;

    err = sscanf(args[3],"%hu",&num_blocks);
    if (err != 1) {
        return ERR_ARGS;
    }

    err = sscanf(args[2],"%hu",&num_inodes);
    if (err != 1) {
        return ERR_ARGS;
    }
    err = mountv6_mkfs(args[1],  num_blocks,  num_inodes);
    if (err) {
        return err;
    }

    return ERR_OK;
}

int do_mkdir(char** args)
{
    int err = 0;
    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    err = direntv6_create(&u, args[1], IALLOC | IFDIR);
    if (err) {
        return err;
    }
    return ERR_OK;
}

int do_add(char** args)
{
    struct filev6 fv6;
    int err = 0;
    FILE* source = NULL;
    size_t taille_fichier = 0;
    char* data = NULL;
    int k = 0;

    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    //ouvrir le fichier source
    source = fopen(args[1], "r");
    if (source == NULL) {
        printf("ERROR FS: Unable to open file %s\n", args[1]);
        return ERR_FS;
    }
    fseek(source, 0, SEEK_END);
    taille_fichier = (size_t) ftell(source) + 1;
    fseek(source, 0, SEEK_SET);

    data = calloc(sizeof(char), taille_fichier);
    if (data == NULL) {
        fclose(source);
        return ERR_NOMEM;
    }

    while (!feof(source)) {
        data[k] = fgetc(source);
        ++k;
    }
    data[k-1] = '\0';
    fclose(source);

    // créer le fichier
    err = direntv6_create(&u, args[2], IALLOC);
    if (err) {
        free(data);
        return err;
    }

    // trouver l'inode
    fv6.i_number = direntv6_dirlookup(&u, ROOT_INUMBER, args[2]);
    if (fv6.i_number < 0) {
        free(data);
        return fv6.i_number;
    }

    // ouvrir le fichier
    err = filev6_open(&u, fv6.i_number, &fv6);
    if (err) {
        free(data);
        return err;
    }


    // écrire dans le fichier
    err = filev6_writebytes(&u, &fv6, data, taille_fichier - 1);

    free(data);
    if (err) {
        return err;
    }

    return ERR_OK;
}

