#include "mount.h"
#include "unixv6fs.h"
#include "direntv6.h"
#include <stdlib.h>
#include <string.h>

int test(struct unix_filesystem *u)
{
    int err = 0;
    
   const char* chaine = NULL;
    
    chaine = malloc(MAXPATHLEN_UV6);
    if (chaine == NULL){
    	err = -40;
    }
    else{
    	memset(chaine, 0, MAXPATHLEN_UV6);
    	memset(chaine, '\0',1);
 
    	err = direntv6_print_tree(u,ROOT_INUMBER,chaine);
    	printf("\n\n");
    	free(chaine);
    }
    
	return err;    
}
