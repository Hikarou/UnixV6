#include "mount.h"
#include "inode.h"
#include "unixv6fs.h"

int testI(struct unix_filesystem *u)
{
    int err = 0;

     mountv6_print_superblock(u);

    return err;
}
