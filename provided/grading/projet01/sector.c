/**
 * @file  sector.c
 * @brief block-level accessor function.
 *
 * @date march 2017
 */

#include <stdint.h>
#include <stdio.h>
#include "error.h"
#include "unixv6fs.h"

/**
 * @brief read one 512-byte sector from the virtual disk
 * @param f open file of the virtual disk
 * @param sector the location (in sector units, not bytes) within the virtual disk
 * @param data a pointer to 512-bytes of memory (OUT)
 * @return 0 on success; <0 on error
 */
int sector_read(FILE *f, uint32_t sector, void *data)
{
    M_REQUIRE_NON_NULL(f);
    size_t read = 0;
    if (fseek(f, sector * SECTOR_SIZE, SEEK_SET) == -1) {
        return ERR_IO;
    }
    read  = fread(data, SECTOR_SIZE, 1, f);
    if (read == 0) {
        return ERR_IO;
    }
    return 0;
}
