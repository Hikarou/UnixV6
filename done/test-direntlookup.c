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

    chaine = malloc(MAXPATHLEN_UV6);
    if (chaine == NULL) {
        err = ERR_NOMEM;
    } else {
        memset(chaine, 0, MAXPATHLEN_UV6);
        memset(chaine, '\0',1);

        err = direntv6_print_tree(u,ROOT_INUMBER,chaine);
        printf("\n\n");
        free(chaine);
    }
     char* ch = "/tmp/coucou.txt";
     
     err =  direntv6_dirlookup(u, ROOT_INUMBER, ch);
     if (err >0){
     	printf("\n\Numero d'inode: %d\n", err);
	}
    return 0;
}
