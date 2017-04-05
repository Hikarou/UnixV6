#include <stdio.h>
#include "mount.h"

struct unix_filesystem u;

struct shell_map {
    const char* name;    // nom de la commande
    shell_fct fct;       // fonction réalisant la commande
    const char* help;    // description de la commande
    size_t argc;         // nombre d'arguments de la commande
    const char* args;    // description des arguments de la commande
};

typedef int (*shell_fct)(char* fct);

struct shell_map shell_cmds[] = {
    {"exit", do_exit, "exit shell.", 0, ""},
    {"quit", do_exit, "exit shell.", 0, ""},
    {"mkfs", do_mkfs, "create a new filesystem.", 3, "<diskname> <#inodes> <#blocks>"},
    {"mount", do_mount, "mount the provided filesystem.", 1, "<diskname>"},
    {"mkdir", do_mkdir, "create a new directory.", 1, "<dirname>"},
    {"lsall", do_lsall, "list all directories and files contained in the currently mounted filesystem.", 0, ""},
    {"add", do_add, "add a new file.", 2, "<src-fullpath> <dst>"},
    {"cat", do_cat, "display the content of a file.", 1, "<pathname>"},
    {"istat", do_istat, "display information about the provided inode.", 1, "<inode_nr>"},
    {"inode", do_inode, "display the inode number of a file.", 1, "<pathname>"},
    {"sha", do_sha, "display the SHA of a file.", 1, "<pathname>"},
    {"psb", do_psb, "Print SuperBlock of the currently mounted filesystem.", 0, ""}
};

int tokenize_input (char* input, char ** parsed);

int do_exit() {

};

int do_help() {

};

int do_lsall() {

};

int do_psb() {

};

int do_cat(char* pathname) {

};

int do_sha(char* pathname) {

};

int do_inode (char* pathname) {

};

int do_istat(uint16_t inr) {

};

int do_mkfs(char* diskname, uint16_t nbInode){

};

int do_mkdir(char* dirname){

};

int do_add(char* srcFullpath, char* dst){

}

int main(void) {
   while (!feof(stdin) && !ferror(stdin)) {

   }
};
