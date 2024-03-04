#ifndef _HASH_TABLE_UINT32_
#define _HASH_TABLE_UINT32_

/*
 * Hash table implementation designed for keys and values of uint32_t type.
 * It is oriented to use in my SLAB allocator in TEUOS instead of usual applications.
 * 
 * Hash table with open addressing.
 * Uses MurmurHash3 as a hash function.
 * Uses quadratic probing to resolve collisions.
 */

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t key;
    uint32_t value;
} hash_table_uint32_item_t;

typedef struct {
    // Total table capacity
    size_t capacity;
    // Current table size
    size_t size;
    // Percentage of table load at reaching which its capacity will be increased and rehashing will be performed
    uint8_t load_fac_max;
    // Number of elements at which its capacity will be increased and rehashing will be performed
    size_t rehash_max_size;
    // Same as load_fac_max, but related to reducing capacity and rehashing
    uint8_t load_fac_min;
    // Same as rehash_max_size, but related to reducing capacity and rehashing
    size_t rehash_min_size;

    // Pointer to table data
    hash_table_uint32_item_t* memory_ptr;
    // Current table size in bytes
    size_t memory_size;
} hash_table_uint32_t;

// Short names for functions

/*
 * Initializes the table
 * 
 * ht_ptr - pointer to hash table
 * capacity - initial total size of the hash table (0 for default value)
 * load_fac_min - percentage of the hash table load, at which its size will decrease and rehashing will be performed (0 for the default value)
 * load_fac_max - percentage of the hash table load, at which its size will increase and rehashing will be performed (0 for the default value)
 */
extern void htui32_init(hash_table_uint32_t* ht_ptr, size_t capacity, uint8_t load_fac_min, uint8_t load_fac_max);

/*
 * Puts the value by key in the table
 * 
 * ht_ptr - pointer to hash table
 * key - key
 * value - value
 * ATTENTION: The key 0 are reserved!
 */
extern void htui32_put(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t value);

/*
 * Gets the value by key from the table
 * Returns true if the value is in the table, otherwise it returns false
 * 
 * ht_ptr - pointer to hash table
 * key - key
 * value_ptr - pointer where the value will be placed, if found. You can use NULL if you only want to know if there is a value in the hash table without getting it.
 */
extern bool htui32_get(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* value_ptr);

/*
 * Frees up the memory allocated for the hash table
 * 
 * ht_ptr - pointer to hash table
 */
extern void htui32_destroy(hash_table_uint32_t* ht_ptr);

#endif
