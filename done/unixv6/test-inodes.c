#include "mount.h"
#include "inode.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
    int err = 0;
    err = inode_scan_print(u);
    if (!err){
    	struct inode inodePourTest;
    	err = inode_read(u, 3, &inodePourTest);
    	if (!err){
    		inode_print(&inodePourTest);
    	}
    }

    return err;
}
