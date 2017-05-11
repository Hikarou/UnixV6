#include "mount.h"
#include "inode.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
    int err = 0;
	struct inode inode;
	
	err = inode_scan_print(u);
	if(!err){
		err = inode_read(u,3,&inode);
	}
	
	inode_print(&inode);

	if(!err){
		printf("ecriture\n");
		err = inode_write(u,4,&inode);
		printf("err = %d\n", err);
	}
	
	if(!err){
    	err = inode_scan_print(u);
    }
    
    if(!err){
		err = inode_read(u,4,&inode);
	}
	
	inode_print(&inode);

    return err;
}
