#include "mount.h"
#include "inode.h"
#include "unixv6fs.h"
#include "filev6.h"

int test(struct unix_filesystem *u)
{
    int err = 0;
    //struct inode inode;
    //struct filev6 fv6;

    //fv6.i_number = 5;
    //uint16_t mode = IALLOC ;

    err = inode_scan_print(u);
    //if(!err) {
    //    err = inode_read(u,3,&inode);
    //}

    /*inode_print(&inode);

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
    */

    //err = filev6_create(u, mode, &fv6);

    //err = inode_read(u,5,&inode);
    //printf("err = %d\n",err);
    //inode_print(&inode);
    //err = inode_scan_print(u);

    return err;
}
