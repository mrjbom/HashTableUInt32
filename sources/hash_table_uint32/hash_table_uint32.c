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
 * Searches for the key in the hash table and returns a pointer to the item in which this key was found
 * as well as returns a pointer to the previous and next item in collision chain (if any) (prev_item -> item -> next_item)
 * item_ptr, prev_item_ptr, next_item_ptr can be NULL
 * 
 * Returns true if the key is found, otherwise false
 */
static bool find_item_by_key(hash_table_uint32_t* ht_ptr, uint32_t key, hash_table_uint32_item_t** item_ptr, hash_table_uint32_item_t** prev_item_ptr, hash_table_uint32_item_t** next_item_ptr)
{
    if (key == 0) {
        return false;
    }

    uint32_t hash = 0;
    MurmurHash3_x86_32(&key, sizeof(key), 0, &hash);
    uint32_t pos = hash % ht_ptr->capacity;

    // Try to find key
    hash_table_uint32_item_t* current_item = &ht_ptr->memory_ptr[pos];
    hash_table_uint32_item_t* prev_item = NULL;
    while (current_item != NULL) {
        if (current_item->key == key) {
            // Key is found
            if (item_ptr != NULL) {
                *item_ptr = current_item;
            }
            if (prev_item_ptr != NULL) {
                *prev_item_ptr = prev_item;
            }
            if (next_item_ptr != NULL) {
                *next_item_ptr = (hash_table_uint32_item_t*)current_item->next;
            }
            return true;
        }
        // Go to next item
        else {
            prev_item = current_item;
            current_item = current_item->next;
        }
    }
    // Key not found
    return false;
}

static void check_and_grow_rehash(hash_table_uint32_t* ht_ptr)
{
    calculate_rehash_sizes(ht_ptr);
    if (ht_ptr->size + 1 >= ht_ptr->rehash_max_size) {
        // Alloc new memory
        hash_table_uint32_item_t* new_memory = alloc_func((ht_ptr->capacity * 2) * sizeof(hash_table_uint32_item_t));
        if (new_memory == NULL) {
            return;
        }
        hash_table_uint32_item_t* old_memory = ht_ptr->memory_ptr;
        size_t old_capacity = ht_ptr->capacity;
        ht_ptr->capacity = old_capacity * 2;
        ht_ptr->memory_ptr = new_memory;
        ht_ptr->memory_size *= 2;
        ht_ptr->size = 0;
        memset(ht_ptr->memory_ptr, 0, ht_ptr->memory_size);
        // Copy items to new hash table
        for (size_t i = 0; i < old_capacity; ++i) {
            // Going through all the elements in the current collision chain
            hash_table_uint32_item_t* current_item = &old_memory[i];
            hash_table_uint32_item_t* next_item = old_memory[i].next;
            while (current_item != NULL) {
                if (current_item->key != 0) {
                    htui32_put(ht_ptr, current_item->key, current_item->value);
                    // Free memory allocated for the collision chain element (but not for the first one)
                    if (current_item != &old_memory[i]) {
                        free_func(current_item);
                    }
                }
                current_item = next_item;
                if (current_item != NULL) {
                    next_item = current_item->next;
                }
            }
        }
        if (ht_ptr->zero_key_is_used) {
            ht_ptr->size++;
        }
        free_func(old_memory);
    }
    calculate_rehash_sizes(ht_ptr);
}

static void check_and_shrink_rehash(hash_table_uint32_t* ht_ptr)
{
    calculate_rehash_sizes(ht_ptr);
    if (ht_ptr->size <= ht_ptr->rehash_min_size) {
        if (ht_ptr->capacity / 2 < ht_ptr->initial_capacity) {
            return;
        }
        // Alloc new memory
        hash_table_uint32_item_t* new_memory = alloc_func((ht_ptr->capacity / 2) * sizeof(hash_table_uint32_item_t));
        if (new_memory == NULL) {
            return;
        }
        hash_table_uint32_item_t* old_memory = ht_ptr->memory_ptr;
        size_t old_capacity = ht_ptr->capacity;
        ht_ptr->capacity = old_capacity / 2;
        ht_ptr->memory_ptr = new_memory;
        ht_ptr->memory_size /= 2;
        ht_ptr->size = 0;
        memset(ht_ptr->memory_ptr, 0, ht_ptr->memory_size);
        // Copy items to new hash table
        for (size_t i = 0; i < old_capacity; ++i) {
            // Going through all the elements in the current collision chain
            hash_table_uint32_item_t* current_item = &old_memory[i];
            hash_table_uint32_item_t* next_item = old_memory[i].next;
            while (current_item != NULL) {
                if (current_item->key != 0) {
                    htui32_put(ht_ptr, current_item->key, current_item->value);
                    // Free memory allocated for the collision chain element (but not for the first one)
                    if (current_item != &old_memory[i]) {
                        free_func(current_item);
                    }
                }
                current_item = next_item;
                if (current_item != NULL) {
                    next_item = current_item->next;
                }
            }
        }
        if (ht_ptr->zero_key_is_used) {
            ht_ptr->size++;
        }
        free_func(old_memory);
    }
    calculate_rehash_sizes(ht_ptr);
}

void htui32_init(hash_table_uint32_t* ht_ptr, size_t capacity, uint8_t load_fac_min, uint8_t load_fac_max)
{
    if (ht_ptr == NULL || load_fac_max > 100) {
        return;
    }
    // Setup default values
    ht_ptr->capacity = (capacity != 0 ? capacity : 4);
    ht_ptr->initial_capacity = ht_ptr->capacity;
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

    ht_ptr->zero_key_is_used = false;
    ht_ptr->zero_key_value = 0;
}

void htui32_put(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t value)
{
    if (ht_ptr == NULL || ht_ptr->capacity == 0) {
        return;
    }

    if (key == 0) {
        if (ht_ptr->zero_key_is_used) {
            ht_ptr->zero_key_value = value;
            return;
        }
        else {
            check_and_grow_rehash(ht_ptr);
            ht_ptr->zero_key_is_used = true;
            ht_ptr->zero_key_value = value;
            ht_ptr->size++;
        }
    }
    else {
        // Try to find key in hash table
        hash_table_uint32_item_t* item = NULL;
        if (find_item_by_key(ht_ptr, key, &item, NULL, NULL)) {
            // Key is in hash table
            // Write value
            item->value = value;
            return;
        }
        else {
            // Key not in hash table and we need to add new item
            // Rehash?
            check_and_grow_rehash(ht_ptr);
            ht_ptr->size++;
            // Put new item
            uint32_t hash = 0;
            MurmurHash3_x86_32(&key, sizeof(key), 0, &hash);
            uint32_t pos = hash % ht_ptr->capacity;
            //printf("Put %u in pos %u\n", key, pos);
            // Is this position is free?
            if (ht_ptr->memory_ptr[pos].key == 0) {
                // Just put the key and value
                ht_ptr->memory_ptr[pos].key = key;
                ht_ptr->memory_ptr[pos].value = value;
            }
            else {
                // The position is used, which means we have to add a new item to collision chain
                // Find last item in collision chain
                hash_table_uint32_item_t* last_item = &ht_ptr->memory_ptr[pos];
                while (last_item->next != NULL) {
                    last_item = last_item->next;
                }
                // Add new item to collision chain
                hash_table_uint32_item_t* new_item = alloc_func(sizeof(hash_table_uint32_item_t));
                if (new_item == NULL) {
                    return;
                }
                new_item->key = key;
                new_item->value = value;
                new_item->next = NULL;
                last_item->next = new_item;
            }
        }
    }
}

bool htui32_get(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* value_ptr)
{
    if (ht_ptr == NULL || ht_ptr->size == 0) {
        return false;
    }

    if (key == 0) {
        if (ht_ptr->zero_key_is_used) {
            if (value_ptr != NULL) {
                *value_ptr = ht_ptr->zero_key_value;
            }
            return true;
        }
        else {
            return false;
        }
    }
    else {
        hash_table_uint32_item_t* item = NULL;
        // Key in hash table?
        if (find_item_by_key(ht_ptr, key, &item, NULL, NULL) == true) {
            if (value_ptr != NULL) {
                *value_ptr = item->value;
            }
            return true;
        }
        else {
            return false;
        }
    }
}

void htui32_delete(hash_table_uint32_t* ht_ptr, uint32_t key)
{
    if (ht_ptr == NULL || ht_ptr->size == 0) {
        return;
    }

    if (key == 0) {
        if (ht_ptr->zero_key_is_used) {
            ht_ptr->zero_key_is_used = false;
            ht_ptr->zero_key_value = 0;
            ht_ptr->size--;
            check_and_shrink_rehash(ht_ptr);
        }
        else {
            return;
        }
    }
    else {
        // Try to find key in hash table
        hash_table_uint32_item_t* item = NULL;
        hash_table_uint32_item_t* prev_item = NULL;
        hash_table_uint32_item_t* next_item = NULL;
        if (find_item_by_key(ht_ptr, key, &item, &prev_item, &next_item)) {
            // Is first in collision chain?
            if (prev_item == NULL) {
                // Clear key and value
                item->key = 0;
                item->value = 0;
            }
            else {
                prev_item->next = next_item;
                free_func(item);
            }
            ht_ptr->size--;
        }
        else {
            return;
        }
    }

    // Rehash?
    check_and_shrink_rehash(ht_ptr);
}

void htui32_destroy(hash_table_uint32_t* ht_ptr)
{
    if (ht_ptr == NULL) {
        return;
    }
    // Free second collision chain items
    for (size_t i = 0; i < ht_ptr->capacity; ++i) {
        hash_table_uint32_item_t* current_item = ht_ptr->memory_ptr[i].next;
        while (current_item != NULL) {
            hash_table_uint32_item_t* next_item = current_item->next;
            free_func(current_item);
            current_item = next_item;
        }
    }
    // Free main(first) items
    if (ht_ptr->memory_ptr != NULL) {
        free_func(ht_ptr->memory_ptr);
    }
}

void htui32_print_iternal_rep(hash_table_uint32_t* ht_ptr)
{
    if (ht_ptr == NULL) {
        return;
    }

    printf("%u %u\n", ht_ptr->size, ht_ptr->capacity);

    if (ht_ptr->zero_key_is_used) {
        printf("(0:%u)\n", ht_ptr->zero_key_value);
    }

    for (size_t i = 0; i < ht_ptr->capacity; ++i) {
        hash_table_uint32_item_t* current_item = &ht_ptr->memory_ptr[i];
        bool is_first_in_collision_chain = true;
        while (current_item != NULL) {
            if (current_item->key != 0) {
                if (is_first_in_collision_chain) {
                    is_first_in_collision_chain = false;
                    printf("[%u]: ", i);
                }
                printf("(%u:%u) ", current_item->key, current_item->value);
            }
            current_item = current_item->next;
        }
        if (!is_first_in_collision_chain) {
            printf("\n");
        }
    }
}
