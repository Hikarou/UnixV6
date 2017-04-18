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
#define NOT_IMPLEMENTED 4
#define NB_ARGS 10

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

int tokenize_input (char*, char**, int*);

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
    char input[MAX_READ+1];
    char** parsed = NULL;
    int err = 0;
    int size_parsed = 0;
    int k = 0;
    shell_fct function = NULL;

    while (!feof(stdin) && !ferror(stdin) && err != 1) {
        parsed = malloc(NB_ARGS*sizeof(void*));
        if (parsed != NULL) {
            scanf("%s", input);
            err = tokenize_input(input, parsed, &size_parsed);
            if (err == 0) {
                // faire une recherche dans les commandes que nous avons
                // poser shell_fct à ...
                while (k < 13 && err != 0) {
                    err = strcmp(parsed[0], shell_cmds[k].name);
                    ++k;
                }
                --k;
                if (err == 0) {
                    function = shell_cmds[k].fct;
                    if (shell_cmds[k].argc<= size_parsed-1) {
                        err = function(parsed);
                    } else {
                        printf("Not enough arguements\n");
                    }
                }
            }
            free(parsed);
        } else {
            err = ERR_NOMEM;
        }
    }

    puts(ERR_MESSAGES[err - ERR_FIRST]);
    return 0;
}

int tokenize_input (char* input, char ** parsed, int* size_parsed)
{
    char* ptr = NULL;
    int size = NB_ARGS;
    *(size_parsed) = 0;

    if (input == NULL || parsed == NULL) {
        return ERR_ARGS;
    }

    if (strlen(input)>0) {
        ptr = strtok(input, " ");
        while (ptr != NULL) {
            if (size <= *(size_parsed)) {
                //Je ne comprends pas ce que tu veux faire ici?
                //parsed = calloc(parsed,*(size_parsed) +1);
                //ça ?
                parsed = calloc(*(size_parsed) + 1, sizeof(parsed) / *(size_parsed));
                size = *(size_parsed)+1;
            }
            parsed[*(size_parsed)] = ptr;
            ++*(size_parsed);
            ptr = strtok(NULL, " ");
        }
    } else {
        return ERR_ARGS;
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
    if (args == NULL) {
        return ERR_ARGS;
    }
    return mountv6(args[0], &u);
}

int do_lsall()
{
    //Comment tester que u soit bien mounté?
    //if (u == NULL) {
    //    return ERR_NOT_MOUNTED;
    //}

    // A mon avis:

    if (u.f == NULL) {
        return ERR_NOT_MOUNTED;
    }
    return direntv6_print_tree(&u, ROOT_INUMBER, "");
}

int do_psb()
{
    //Même question que juste au dessus
    //if (u == NULL) {
    //    return ERR_NOT_MOUNTED;
    //}

    // A mon avis:

    if (u.f == NULL) {
        return ERR_NOT_MOUNTED;
    }

    mountv6_print_superblock(&u);
    return ERR_OK;
}

int do_cat(char** args)
{
    uint16_t inode_nb = 0;
    int err = 0;
    struct filev6 file;
    char content[SECTOR_SIZE+1];

    inode_nb = direntv6_dirlookup(&u, ROOT_INUMBER, *args);
    if (inode_nb < 0) {
        return inode_nb;
    }

    err = inode_read(&u, inode_nb, &(file.i_node));
    if (err != 0) {
        return err;
    }
    // Allocation test already done in inode_read
    if (file.i_node.i_mode & IFDIR) {
        printf("ERROR SHELL: car on a directory is not defined\n");
        return ERR_OK;
    }

    err = filev6_open(&u, inode_nb, &file);
    if (err != 0) {
        return err;
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
    uint16_t inode_nb = 0;
    int err = 0;
    struct filev6 file;

    inode_nb = direntv6_dirlookup(&u, ROOT_INUMBER, *args);

    if (inode_nb < 0) {
        return inode_nb;
    }
    err = inode_read(&u, inode_nb, &(file.i_node));

    if (err != 0) {
        return err;
    }
    // Allocation test already done in inode_read
    if (file.i_node.i_mode & IFDIR) {
        printf("SHA inode %d: no SHA for directories\n", inode_nb);
        return ERR_ARGS;
    }
    printf("SHA inode %d: ", inode_nb);
    print_sha_inode(&u, file.i_node, (int) inode_nb);
    printf("\n");

    return ERR_OK;
}

int do_inode (char** args)
{
    uint16_t inode_nb = 0;

    inode_nb = direntv6_dirlookup(&u, ROOT_INUMBER, *args);
    if (inode_nb < 0) {
        return inode_nb;
    }

    printf("inode: %d\n", inode_nb);

    return ERR_OK;
}

int do_istat(char** args)
{
    int err = 0;
    int inr = 0;
    struct inode i;

    err = sscanf(*args, "%d", &inr);
    if (err<1 || inr < 0) {
        printf("ERROR FS_ inode out of range\n");
        return ERR_ARGS;
    }

    err = inode_read(&u, inr, &i);
    if (err != 0) {
        return err;
    }

    inode_print(&i);

    return ERR_OK;
}

int do_mkfs(char** args)
{
    return ERR_OK;
}

int do_mkdir(char** args)
{
    return ERR_OK;
}

int do_add(char** args)
{
    return ERR_OK;
}

//int main(void) {
//    int quit = 0;
//    while (!feof(stdin) && !ferror(stdin) && !quit) {
//        char in[MAX_READ];
//	fgets(in, MAX_READ, stdin);
//	char* p;
//	if ((p = strchr(in, '\n'))) *p = '\0';
//	in[MAX_READ - 1] = '\0';
//    }
//
//    return ERR_OK;
//}
