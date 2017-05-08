/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "bmblock.h"
#include "error.h"

#define ERR_NO_PLACE -26


/**
 * @brief allocate a new bmblock_array to handle elements indexed
 * between min and may (included, thus (max-min+1) elements).
 * @param min the mininum value supported by our bmblock_array
 * @param max the maxinum value supported by our bmblock_array
 * @return a pointer of the newly created bmblock_array or NULL on failure
 */
struct bmblock_array *bm_alloc(uint64_t min, uint64_t max)
{
    int err = 0;
    struct bmblock_array* b = NULL;

    if (max < min) {
        err = ERR_BAD_PARAMETER;
    } else {
        size_t taille = (max - min)/(sizeof(uint64_t)*8);
        b = malloc(sizeof(struct bmblock_array) + sizeof(uint64_t)*(taille));

        if (b == NULL) {
            err = ERR_NOMEM;
        } else {
            b -> length = taille + 1;
            b -> cursor = 0;
            b -> min = min;
            b -> max = max;

            memset(b -> bm, 0, b -> length);
        }
    }
    if (err) {
        puts(ERR_MESSAGES[err - ERR_FIRST]);
    }
    return b;
}


/**
 * @brief return the bit associated to the given value
 * @param bmblock_array the array containing the value we want to read
 * @param x an integer corresponding to the number of the value we are looking for
 * @return <0 on failure, 0 or 1 on success
 */
int bm_get(struct bmblock_array *bmblock_array, uint64_t x)
{
    M_REQUIRE_NON_NULL(bmblock_array);
    if (x < bmblock_array -> min || x > bmblock_array -> max)
        return ERR_BAD_PARAMETER;

    uint64_t i = (x - bmblock_array -> min)/(sizeof(uint64_t)*8);
    return (bmblock_array -> bm[i]) & (UINT64_C(1) << ((x - bmblock_array -> min)%(sizeof(uint64_t)*8))) ? 1 : 0;

    //pratique pour la suite peut-être
    /*if(err == 0 && ((x - bmblock_array -> min)< bmblock_array -> cursor)){
      	bmblock_array -> cursor = x - bmblock_array -> min;
    }*/
}


/**
 * @brief set to true (or 1) the bit associated to the given value
 * @param bmblock_array the array containing the value we want to set
 * @param x an integer corresponding to the number of the value we are looking for
 */
void bm_set(struct bmblock_array *bmblock_array, uint64_t x)
{
    if (bmblock_array != NULL && x >= bmblock_array -> min && x <= bmblock_array -> max) {
        uint64_t i = (x - bmblock_array-> min)/(sizeof(uint64_t)*8);
        (bmblock_array -> bm[i]) = (bmblock_array -> bm[i]) | (UINT64_C(1) << ((x - bmblock_array -> min)%(sizeof(uint64_t)*8)));
    }
}

/**
 * @brief set to false (or 0) the bit associated to the given value
 * @param bmblock_array the array containing the value we want to clear
 * @param x an integer corresponding to the number of the value we are looking for
 */
void bm_clear(struct bmblock_array *bmblock_array, uint64_t x)
{
    if (bmblock_array != NULL && x >= bmblock_array -> min && x <= bmblock_array -> max) {
        uint64_t i = (x - bmblock_array -> min)/(sizeof(uint64_t)*8);
        (bmblock_array -> bm[i]) = (bmblock_array -> bm[i]) & ~(UINT64_C(1) << ((x - bmblock_array -> min)%(sizeof(uint64_t)*8)));
    }

    //pratique pour la suite peut-être
    if ((x - bmblock_array -> min) < bmblock_array -> cursor) {
        bmblock_array -> cursor = x - bmblock_array -> min;
    }
}

void bm_print(struct bmblock_array *bmblock_array)
{
    if (bmblock_array != NULL) {
        printf("**********BitMap Block START**********\n");
        printf("length: %lu\n", bmblock_array -> length);
        printf("min: %lu\n", bmblock_array -> min);
        printf("max: %lu\n", bmblock_array -> max);
        printf("cursor: %lu\n", bmblock_array -> cursor);
        printf("content:\n");
        for (size_t i = 0; i < bmblock_array -> length; ++i) {
            printf("%lu: ", i);
            for (size_t j = 0; j < sizeof(uint64_t); ++j) {
                for (size_t k = 0; k<8; ++k) {
                    //      printf("%d", bm_get(bmblock_array, i*sizeof(uint64_t)*8 + j*8 + k + bmblock_array -> min));
                    if ((bmblock_array -> bm[i]) & (UINT64_C(1) << (j*8 + k))) {
                        printf("1");
                    } else {
                        printf("0");
                    }
                }
                printf(" ");
            }
            printf("\n");
        }
        printf("**********BitMap Block END************\n");
    }
}

/**
 * @brief return the next unused bit
 * @param bmblock_array the array we want to search for place
 * @return <0 on failure, the value of the next unused value otherwise
 */
int bm_find_next(struct bmblock_array *bmblock_array)
{
    M_REQUIRE_NON_NULL(bmblock_array);
    int err = 0;
    uint64_t i = 0;
    int k = 0;

    while (k == 0) {
        i = (bmblock_array -> cursor)/(sizeof(uint64_t)*8);

        if (bmblock_array -> bm[i] != UINT64_C(-1)) {
            // il reste encore la place
            if (!((bmblock_array -> bm[i]) & (UINT64_C(1) << ((bmblock_array -> cursor)%( sizeof(uint64_t)*8))))) {
                // si la valeur du curseur actuel est zero
                err = bmblock_array -> cursor + bmblock_array -> min;
                k = 1;
            }

            if ((bmblock_array -> cursor < bmblock_array -> max - bmblock_array -> min ) && k == 0) {
                // si on n'a pas atteint la taille de la structure, on incrémente le curseur
                ++(bmblock_array -> cursor);
            } else if (k == 0) {
                err = ERR_NO_PLACE;
                k = 1;
            }
        } else { // passer au block de 64 bits suivant
            ++i;
            if (i < bmblock_array -> length) {
                bmblock_array -> cursor = i * sizeof(uint64_t) * 8;
            } else {
                err = ERR_NO_PLACE;
                bmblock_array -> cursor = i * sizeof(uint64_t) * 8 - 1;
                k = 1;
            }
        }
    }

    return err;
}
