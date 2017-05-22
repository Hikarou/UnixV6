#include "mount.h"
#include "unixv6fs.h"
#include "direntv6.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

#define MAXPATHLEN_UV6 1024

int test(struct unix_filesystem *u)
{
    int err = 0;

    char* chaine = NULL;

    chaine = calloc(1, MAXPATHLEN_UV6);
    if (chaine == NULL) {
        err = ERR_NOMEM;
    } else {
        memset(chaine, 0, MAXPATHLEN_UV6);
        memset(chaine, '\0',1);

        err = direntv6_print_tree(u,ROOT_INUMBER,chaine);
        printf("\n");
        free(chaine);
    }

    return err;
}
