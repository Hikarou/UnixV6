#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "bmblock.h"
#include "error.h"

int main ()
{
    struct bmblock_array* b;

    b = bm_alloc(UINT64_C(4), UINT64_C(131));
    int err = 0;

    if (b != NULL) {
        bm_print(b);
        printf("find_next() = %d\n", bm_find_next(b));

        bm_set(b, UINT64_C(4));

        bm_set(b, UINT64_C(5));

        bm_set(b, UINT64_C(6));
        bm_print(b);
        printf("find_next() = %d\n", bm_find_next(b));

        for (uint64_t i = UINT64_C(4); i< b->max; i+=UINT64_C(3)) {
            bm_set(b, i);
        }

        err = bm_get(b, UINT64_C(228));
        if (err >= 0) {
            printf("Valeur = %d\n", err);
        }
        bm_print(b);
        printf("find_next() = %d\n", bm_find_next(b));

        for (uint64_t i = UINT64_C(5); i< b->max; i+= UINT64_C(5)) {
            bm_clear(b, i);
        }

        bm_print(b);
        printf("find_next() = %d\n", bm_find_next(b));
        free(b);
    } else {
        printf("Probleme!\n");
    }
    return 0;
}
