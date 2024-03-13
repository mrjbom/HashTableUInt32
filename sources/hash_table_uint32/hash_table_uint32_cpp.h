#ifndef _HASH_TABLE_UINT32_CPP_
#define _HASH_TABLE_UINT32_CPP_

// CPP VERSION FOR RANDOM TEST

/*
 * Hash table implementation designed for keys and values of uint32_t type.
 * It is oriented to use in my SLAB allocator in TEUOS instead of usual applications.
 * 
 * Hash table with separate chaining.
 * Uses MurmurHash3 as a hash function.
 */

#include <stdint.h>
#include <stdbool.h>

typedef struct hash_table_uint32_item {
    // Pointer to the next structure in the collision chain, used as sllist_node_t by sllist.h
    struct hash_table_uint32_item* next;
    uint32_t key;
    uint32_t value;
} hash_table_uint32_item_t;

typedef struct {
    // Total table capacity
    size_t capacity;
    // The hash table will not shrink to less than this size
    size_t initial_capacity;
    // Current table size
    size_t size;
    // Percentage of hash table load at reaching which its capacity will be increased and rehashing will be performed
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

    // Indicates whether the table contains the key 0
    bool zero_key_is_used;
    // The key value 0 is not stored in the table like regular keys, it is stored in this variable
    uint32_t zero_key_value;
} hash_table_uint32_t;

// Short names for functions

/*
 * Initializes the table
 * 
 * ht_ptr - pointer to hash table
 * capacity - initial total size of the hash table (0 for default value) (The hash table will not shrink to less than this initial capacity!)
 * load_fac_min - percentage of the hash table load, at which its size will decrease and rehashing will be performed (0 for the default value)
 * load_fac_max - percentage of the hash table load, at which its size will increase and rehashing will be performed (0 for the default value)
 */
extern "C" void htui32_init(hash_table_uint32_t* ht_ptr, size_t capacity, uint8_t load_fac_min, uint8_t load_fac_max);

/*
 * Puts value by key in table
 *
 * ht_ptr - pointer to hash table
 * key - key
 * value - value
 */
extern "C" void htui32_put(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t value);

/*
 * Gets value by key from table
 * Returns true if the value is in the table, otherwise it returns false
 *
 * ht_ptr - pointer to hash table
 * key - key
 * value_ptr - pointer where the value will be placed, if found. You can use NULL if you only want to know if there is a value in the hash table without getting it.
 */
extern "C" bool htui32_get(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* value_ptr);

/*
 * Deletes value by key from hash table
 *
 * ht_ptr - pointer to hash table
 * key - key
 */
extern "C" void htui32_delete(hash_table_uint32_t* ht_ptr, uint32_t key);


/*
 * Frees up the memory allocated for hash table
 * 
 * ht_ptr - pointer to hash table
 */
extern "C" void htui32_destroy(hash_table_uint32_t* ht_ptr);

/*
 * Prints the internal representation of the hash table
 * for example:
 * [0]: (key:value) (key:value)
 * [2]: (key:value)
 * [3]: (key:value) (key:value) (key:value)
 * [7]: (key:value)
 * 
 * ht_ptr - pointer to hash table
 */
extern "C" void htui32_print_iternal_rep(hash_table_uint32_t* ht_ptr);

#endif
