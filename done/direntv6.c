#include <stdint.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"

int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d)
{
	struct filev6 fv6;
	int err = 0;

	err = filev6_open(u, inr, &fv6);
	if (err!=0){
		return err;
	}
	
	if ((fv6.i_node.imode & IALLOC) || !(fv6.i_node.imode & IFDIR)) {
		return ERR_INVALID_DIRECTORY_INODE;
	}
	
	d -> fv6 = fv6;
	d -> cur = 0;
	d -> last = 0;
	
	return 0; 

}


int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr)
{
	int err = 0;
	
	// lire le secteur _: directory_reader -> filev6 -> inode: rad block
	// slocker le readblock dans dirs
	// last combien de sous fichier dans le secteur
	// cur doit valoir 0.
	
	
	// tant que readblock ne renvoie pas zero il faut encore lire des secteurs
	
	// lire le dir qui  à la position cur, prendre son nom et son nom dans la structure direntv6
	
	if (d->cur == d->last){
		
		
		
		
		
	}
	else{
	
	}


}

