#include "mount.h"
#include "inode.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
    int err = 0;
    int numeroInode = 0;
    int numeroSecteur = 0;
    int offset = 0;
    
    
    err = inode_scan_print(u);
    /*if (!err){
    	struct inode inodePourTest;
    	printf("\nEntrez le numero de l'inode: ");
    	scanf("%d", &numeroInode);

    	err = inode_read(u, numeroInode, &inodePourTest);
    	if (!err){
    		inode_print(&inodePourTest,numeroInode);
    		    printf("\nEntrez l'offset: ");
    			scanf("%d", &offset);
    		numeroSecteur = inode_findsector(u,&inodePourTest,offset);
    		if (numeroSecteur >=0){
    			printf("\nNumero de secteur: %d.\n", numeroSecteur);
    		}
    		else{
    			printf("Problème, erreur = %d", numeroSecteur);
    			err = numeroSecteur;
    		}
    	}
    }*/

    return err;
}
