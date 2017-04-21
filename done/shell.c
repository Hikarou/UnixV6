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
#define NB_ARGS 3

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

struct shell_map shell_cmds[NB_CMDS] = {
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
        parsed = calloc(NB_ARGS, sizeof(void*));
        if (parsed != NULL) {
            fgets(input, MAX_READ, stdin);
            if (input[strlen(input)-1] == '\n') {
                input[strlen(input)-1] = '\0';
            }
            input[MAX_READ] = '\0';

            err = tokenize_input(input, &parsed, &size_parsed);
            if (err == ERR_OK) {
                do {
                    err = strcmp(parsed[0], shell_cmds[k].name);
                    ++k;
                } while (k < 13 && err != 0);
                --k;
                if (err == 0) {
                    function = shell_cmds[k].fct;

                    if ((shell_cmds[k]).argc == size_parsed-1) {
                        err = function(parsed);
                    } else {
                        printf("ERROR SHELL: wrong number of arguements\n");
                        err = ERR_ARGS;
                    }
                } else {
                    printf("ERROR SHELL: Invalid command\n");
                    err = ERR_ARGS;
                }
            }

            free(parsed);

        } else {
            err = ERR_NOMEM;
            puts(ERR_MESSAGES[err - ERR_FIRST]);
            err =  EXIT;
        }

        if (err != 0 && err != EXIT) {
            err = ERR_OK;
        }
    }

    if (u.f != NULL) {
        umountv6(&u);
    }

    return 0;
}

int tokenize_input (char* input, char*** parsed, int* size_parsed)
{
    if (input == NULL) return EXIT; //Pour s'occuper du cas CTRL + D
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
                        *parsed = realloc(*parsed, sizeof(void*) * size);
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

    if (*size_parsed > size-1) {
        ++size;
        *parsed = realloc(*parsed, sizeof(char) * size * MAX_READ);
        if (*parsed == NULL) return EXIT;
    }
    (*parsed)[*(size_parsed)] = ptr;
    *(size_parsed) = *(size_parsed) + 1;

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
    int err = mountv6(args[1], &u);

    if (err != 0) {
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return ERR_FS;
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

    if (err != 0) {
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return ERR_FS;
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
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[inode_nb - ERR_FIRST]);
        return ERR_FS;
    }

    err = inode_read(&u, inode_nb, &(file.i_node));
    if (err != 0) {
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return ERR_FS;
    }
    // Allocation test already done in inode_read
    if (file.i_node.i_mode & IFDIR) {
        printf("ERROR SHELL: cat on a directory is not defined\n");
        return ERR_ARGS;
    }

    err = filev6_open(&u, inode_nb, &file);
    if (err != 0) {
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return ERR_FS;
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
    //printf("err inode_nb = %d\n", inode_nb);
    if (inode_nb < 0) {
        printf("ERROR FS: ");
        //printf("COUCOU\n");
        puts(ERR_MESSAGES[inode_nb - ERR_FIRST]);
        return ERR_FS;
    }

    err = inode_read(&u, inode_nb, &(file.i_node));

    if (err != 0) {
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return ERR_FS;
    }
    // Allocation test already done in inode_read
    if (file.i_node.i_mode & IFDIR) {
        printf("SHA inode %d: no SHA for directories\n", inode_nb);
        return ERR_ARGS;
    }
    // printf("SHA inode %d: ", inode_nb);
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
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[inode_nb - ERR_FIRST]);
        return ERR_FS;
    }

    printf("inode: %d\n", inode_nb);

    return ERR_OK;
}

int do_istat(char** args)
{
    int err = 0;
    int inr = 0;
    struct inode i;

    if (u.f == NULL) {
        printf("ERROR SHELL: mount the FS before operation\n");
        return ERR_NOT_MOUNTED;
    }

    err = sscanf(args[1], "%d", &inr);
    if (err<1 || inr < 0) {
        printf("ERROR FS: inode out of range\n");
        return ERR_ARGS;
    }

    err = inode_read(&u, inr, &i);
    if (err != 0) {
        printf("ERROR FS: ");
        puts(ERR_MESSAGES[err - ERR_FIRST]);
        return ERR_FS;
    }

    inode_print(&i);

    return ERR_OK;
}

int do_mkfs(char** args)
{
    return NOT_IMPLEMENTED;
}

int do_mkdir(char** args)
{
    return NOT_IMPLEMENTED;
}

int do_add(char** args)
{
    return NOT_IMPLEMENTED;
}

