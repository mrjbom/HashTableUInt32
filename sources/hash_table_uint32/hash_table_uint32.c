#include "hash_table_uint32.h"
#include "murmur_hash3/murmur_hash3.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define alloc_func(size) malloc(size)
#define free_func(ptr) free(ptr)

static void calculate_rehash_sizes(hash_table_uint32_t* ht_ptr)
{
    // Same ht_ptr->capacity * ((double)m->load_fac_max / 100)
    // I avoid using double because this hash table is intended for use in the kernel's SLAB allocator
    // and I cannot use floating point operations.
    // Round up
    ht_ptr->rehash_max_size = (ht_ptr->capacity * ht_ptr->load_fac_max + 99) / 100;
    // Round down
    ht_ptr->rehash_min_size = (ht_ptr->capacity * ht_ptr->load_fac_min) / 100;
}

/*
 * Finds the position where the key is located
 * Uses quadratic probing
 * Returns true if the key is found, otherwise false
 * pos_ptr can be NULL if you only want to know if an element exists
 * If the key was found in the table, its position will be recorded, otherwise, the value of the space available for recording will be recorded.
 * Thus, the function can be used for both writing and reading.
 * 
 * 0:0 - free for use
 * 0:UINT32_MAX - deleted value
 * The reason why we do not mark it with 0:0
 * is that when searching for a key among collisions when a deleted value is found,
 * we must continue the search, ending it only when 0:0 is encountered.
 */
static bool find_pos_by_key(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* pos_ptr)
{

    // Hash of the key
    uint32_t hash = 0;
    MurmurHash3_x86_32(&key, sizeof(key), 0, &hash);
    uint32_t pos = hash % ht_ptr->capacity;

    // Starting the key search using quadratic probing
    uint32_t current_probing_iteration = 0;
    while (true) {
        // Key has been found?
        if (ht_ptr->memory_ptr[pos].key == key) {
            if (ht_ptr->memory_ptr[pos].key == 0 && ht_ptr->memory_ptr[pos].value == UINT32_MAX) {
                // We came across a deleted value, we're moving forward
                goto do_probing;
            }
            if (pos_ptr != NULL) {
                *pos_ptr = pos;
            }
            return true;
        }
        if (ht_ptr->memory_ptr[pos].key == 0 && ht_ptr->memory_ptr[pos].value == 0) {
            // It's free pos
            // The key does not exist, there is no point in looking any further
            if (pos_ptr != NULL) {
                *pos_ptr = pos;
            }
            return false;
        }
        // Do quadratic probing
    do_probing:
        current_probing_iteration++;
        //printf("[%u] Collision on %u ",current_probing_iteration, pos);
        pos = (pos + (current_probing_iteration * current_probing_iteration)) % ht_ptr->capacity;
        //printf("new pos is %u\n", pos);
    }
}

void htui32_init(hash_table_uint32_t* ht_ptr, size_t capacity, uint8_t load_fac_min, uint8_t load_fac_max)
{
    if (ht_ptr == NULL || load_fac_max > 100) {
        return;
    }
    // Setup default values
    ht_ptr->capacity = (capacity != 0 ? capacity : 4);
    ht_ptr->size = 0;
    ht_ptr->load_fac_min = (load_fac_min != 0 ? load_fac_min : 25);
    ht_ptr->load_fac_max = (load_fac_max != 0 ? load_fac_max : 75);
    calculate_rehash_sizes(ht_ptr);

    // Alloc memory
    ht_ptr->memory_size = ht_ptr->capacity * sizeof(hash_table_uint32_item_t);
    ht_ptr->memory_ptr = alloc_func(ht_ptr->memory_size);
    if (ht_ptr->memory_ptr == NULL) {
        return;
    }
    memset(ht_ptr->memory_ptr, 0, ht_ptr->memory_size);
}

void htui32_put(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t value)
{
    if (ht_ptr == NULL || key == 0) {
        return;
    }
    if (ht_ptr->capacity == 0) {
        return;
    }

    calculate_rehash_sizes(ht_ptr);
    uint32_t pos = 0;
    if (find_pos_by_key(ht_ptr, key, &pos) == true) {
        // Key is already in the hash table, we don't need to expand the hash table
        // Just only put value
        //printf("Put %u:%u to %u\n", key, value, pos);
        ht_ptr->memory_ptr[pos].value = value;
        return;
    }
    else {
        if (ht_ptr->size + 1 > ht_ptr->capacity) {
            return;
        }
        if (ht_ptr->size + 1 < ht_ptr->rehash_max_size) {
            // There is no need to expand the hash table, just put the value
            //printf("Put %u:%u to %u\n", key, value, pos);
            ht_ptr->memory_ptr[pos].key = key;
            ht_ptr->memory_ptr[pos].value = value;
            ht_ptr->size++;
            return;
        }
        else {
            // Need expand and rehash hash table
            // Alloc new memory for hash table
            //printf("Put rehash\n");
            hash_table_uint32_item_t* new_memory_ptr = alloc_func(ht_ptr->memory_size * 2);
            if (new_memory_ptr == NULL) {
                return;
            }
            ht_ptr->capacity *= 2;
            ht_ptr->memory_size *= 2;
            memset(new_memory_ptr, 0, ht_ptr->memory_size);
            // Copy items from old hash table to new
            hash_table_uint32_item_t* old_memory_ptr = ht_ptr->memory_ptr;
            ht_ptr->memory_ptr = new_memory_ptr;
            size_t copied_count = 0;
            for (size_t i = 0; copied_count < ht_ptr->size; ++i) {
                if (old_memory_ptr[i].key != 0) {
                    // Put item into new hash table
                    find_pos_by_key(ht_ptr, old_memory_ptr[i].key, &pos);
                    //printf("Rehash put %u:%u to %u\n", old_memory_ptr[i].key, old_memory_ptr[i].value, pos);
                    new_memory_ptr[pos].key = old_memory_ptr[i].key;
                    new_memory_ptr[pos].value = old_memory_ptr[i].value;
                    copied_count++;
                }
            }
            free_func(old_memory_ptr);

            // Put new item to hash table
            find_pos_by_key(ht_ptr, key, &pos);
            //printf("Put %u:%u to %u\n", key, value, pos);
            new_memory_ptr[pos].key = key;
            new_memory_ptr[pos].value = value;
            ht_ptr->size++;
            return;
        }
    }
}

bool htui32_get(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* value_ptr)
{
    if (ht_ptr == NULL || key == 0) {
        return false;
    }
    if (ht_ptr->size == 0) {
        return false;
    }
    uint32_t pos = 0;
    bool has_found = find_pos_by_key(ht_ptr, key, &pos);
    if (has_found) {
        if (value_ptr != NULL) {
            *value_ptr = ht_ptr->memory_ptr[pos].value;
        }
        return has_found;
    }
    else {
        return has_found;
    }
}

void htui32_destroy(hash_table_uint32_t* ht_ptr)
{
    if (ht_ptr == NULL) {
        return;
    }
    if (ht_ptr->memory_ptr != NULL) {
        free_func(ht_ptr->memory_ptr);
    }
}
