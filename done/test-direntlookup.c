#include "mount.h"
#include "unixv6fs.h"
#include "direntv6.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

int test(struct unix_filesystem *u)
{
    int err = 0;

    char* chaine = NULL;

    chaine = "";

        err = direntv6_print_tree(u,ROOT_INUMBER,chaine);
        printf("\n\n");
    
    char* ch = "/tmp/coucoutxt";

    err = direntv6_dirlookup(u, ROOT_INUMBER, ch);
    
    printf("err = %d\n", err);
    if (err >0) {
        printf("\n\Numero d'inode: %d\n", err);
    }
    return 0;
}
