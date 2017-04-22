#include "mount.h"
#include "unixv6fs.h"
#include "direntv6.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

int test(struct unix_filesystem *u)
{
    int err = 0;

    const char* chaine = "";

    err = direntv6_print_tree(u,ROOT_INUMBER,chaine);
    printf("\n");

    //Test in disks/simple.uv6
    //char* ch = "/tmp/coucou.txt";
    //char* ch = "/tmp/coucoutxt";
    const char* ch = "///tmp/coucou.txt";
    //char* ch = "////tmp////coucou.txt/////";
    //Test in disks/first.uv6
    //char* ch = "/hello/net/http/testdata/index.html";
    err = direntv6_dirlookup(u, ROOT_INUMBER, ch);

    printf("La recherche de %s ", ch);
    if (err >0) {
        printf("donne le numéro d'inode: %d", err);
    } else {
        printf("est inconcluante");
    }
    printf("\n");
    return 0;
}
