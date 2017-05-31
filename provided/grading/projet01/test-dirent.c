#include "mount.h"
#include "unixv6fs.h"
#include "direntv6.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

#define MAXPATHLEN_UV6 1024


/* correcteur : compliqué pour rien, le code de cette fonction tient en une ligne : 

return direntv6_print_tree(u, ROOT_INUMBER, "")

Notez que de passer une chaîne de caractères contenant un \0 comme premier caractère est équivalent à une chaîne vide
*/
int test(struct unix_filesystem *u)
{
    int err = 0;

    char* chaine = NULL;

    // correcteur : [USELESS_DYNALLOC] pas besoin d'allocation dynamique !
    // un simple char chaine[MAXPATHLEN_UV6] aurait suffit
    chaine = malloc(MAXPATHLEN_UV6);
    if (chaine == NULL) {
        err = ERR_NOMEM;
    } else {
        memset(chaine, 0, MAXPATHLEN_UV6);
        // correcteur : '\0' et 0 sont équivalents, la ligne ci-dessous est inutile
        memset(chaine, '\0',1);

        err = direntv6_print_tree(u,ROOT_INUMBER,chaine);
        printf("\n");
        free(chaine);
    }

    return err;
}
