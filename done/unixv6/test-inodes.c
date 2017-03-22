#include "mount.h"
#include "inode.h"
#include "unixv6fs.h"

int test(struct unix_filesystem *u)
{
    int err = 0;
    err = inode_scan_print(u);
    inode_print(3);

    return err;
}
