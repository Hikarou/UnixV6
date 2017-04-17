#include <stdio.h>
#include <string.h>
#include "mount.h"
#include "sector.h"
#include "direntv6.h"

#define MAX_READ 255
#define NB_CMDS 13
#define ERR_OK 0
#define EXIT 1
#define ERR_ARGS 2
#define ERR_NOT_MOUNTED 3
#define NOT_IMPLEMENTED 4

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

int tokenize_input (char* input, char ** parsed) {
    if (input == NULL || parsed == NULL) {
        return ERR_ARGS;
    }
    return ERR_OK;
}

int do_exit() {
    return EXIT;
}

int do_help() {
    for (int i = 0; i < NB_CMDS; ++i) {
        printf("- %s", shell_cmds[i].name);
	if (shell_cmds[i].argc > 0) {
            printf(" %s", shell_cmds[i].args);
	}
	printf(": %s\n", shell_cmds[i].help);
    }
    return ERR_OK;
}

int do_mount(char** args) {
    if (args == NULL) {
        return ERR_ARGS;
    }
    return mountv6(args[0], &u);
}

int do_lsall() {
    //Comment tester que u soit bien mounté?
    //if (u == NULL) {
    //    return ERR_NOT_MOUNTED;
    //}
    return direntv6_print_tree(&u, ROOT_INUMBER, "");
}

int do_psb() {
    //Même question que juste au dessus
    //if (u == NULL) {
    //    return ERR_NOT_MOUNTED;
    //}
    mountv6_print_superblock(&u);
    return ERR_OK;
}

int do_cat(char** args) {
    
    return ERR_OK;
}

int do_sha(char** args) {
    return ERR_OK;
}

int do_inode (char** args) {
    return ERR_OK;
}

int do_istat(char** args) {
    return ERR_OK;
}

int do_mkfs(char** args){
    return ERR_OK;
}

int do_mkdir(char** args){
    return ERR_OK;
}

int do_add(char** args){
    return ERR_OK;
}

int main(void) {
    int quit = 0;
    while (!feof(stdin) && !ferror(stdin) && !quit) {
        char in[MAX_READ];
	fgets(in, MAX_READ, stdin);
	char* p;
	if ((p = strchr(in, '\n'))) *p = '\0';
	in[MAX_READ - 1] = '\0';
    }

    return ERR_OK;
}
