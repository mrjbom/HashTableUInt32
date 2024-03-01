#ifndef _HASH_TABLE_UINT32_
#define _HASH_TABLE_UINT32_

/*
 * Hash table implementation designed for keys and values of uint32_t type.
 * It is oriented to use in my SLAB allocator in TEUOS instead of usual applications.
 */

#include <stdint.h>

typedef struct hash_table_uint32 {
    uint32_t key;
    uint32_t value;
};

#endif
