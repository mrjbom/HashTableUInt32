#include "hash_table_uint32.h"
#include "murmur_hash3/murmur_hash3.h"
#include <stdlib.h>
#include <string.h>

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
 * Searches for the key in the hash table and gets its position and the number of iterations of probing passed for the search
 * You can pass NULL to pos_ptr if you only need to find out if it exists
 * probing_iterations_number_ptr also can be NULL
 * key must not be 0
 * Returns true if the key is found and writes its index to the pos.
 * Returns false if the key is not found.
 */
static bool find_pos_by_key(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* pos_ptr, uint32_t* probing_iterations_number_ptr)
{
    if (key == 0) {
        return false;
    }
    uint32_t hash = 0;
    MurmurHash3_x86_32(&key, sizeof(uint32_t), 0, &hash);
    uint32_t pos = hash % ht_ptr->capacity;
    uint32_t current_probing_iteration = 0;
    while (true) {
        // Is key found?
        if (ht_ptr->memory_ptr[pos].key == key) {
            if (pos_ptr != NULL) {
                *pos_ptr = pos;
            }
            if (probing_iterations_number_ptr != NULL) {
                *probing_iterations_number_ptr = current_probing_iteration;
            }
            return true;
        }
        // Is this pos marked as deleted?
        else if (ht_ptr->memory_ptr[pos].key == 0 && ht_ptr->memory_ptr[pos].value == UINT32_MAX) {
            // We continue the search
            goto do_probing;
        }
        // Is this pos marked as free for use?
        else if (ht_ptr->memory_ptr[pos].key == 0 && ht_ptr->memory_ptr[pos].value == 0) {
            // Key not found
            return false;
        }
        // Use quadratic probing
    do_probing:
        printf("Collision at %u ", pos);
        current_probing_iteration++;
        pos = (pos + (current_probing_iteration * current_probing_iteration)) % ht_ptr->capacity;
        printf("new pos is %u \n", pos);
    }
}

/*
 * Finds the position marked as free or deleted for key
 * key must not be 0
 */
static void find_free_or_deleted_pos_for_key(hash_table_uint32_t* ht_ptr, uint32_t key, uint32_t* pos_ptr)
{
    if (ht_ptr->size == ht_ptr->capacity || key == 0) {
        return;
    }
    uint32_t hash = 0;
    MurmurHash3_x86_32(&key, sizeof(uint32_t), 0, &hash);
    uint32_t pos = hash % ht_ptr->capacity;
    uint32_t current_probing_iteration = 0;
    while (true) {
        if (ht_ptr->memory_ptr[pos].key == 0) {
            *pos_ptr = pos;
            return;
        }
        else {
            goto do_probing;
        }
    do_probing:
        printf("Collision at %u ", pos);
        // TODO: This place can potentially lead to long hours of work.
        // If there is no free space for a long time (for example, when there are too many elements in the table, but only one place is free).
        current_probing_iteration++;
        pos = (pos + (current_probing_iteration * current_probing_iteration)) % ht_ptr->capacity;
        printf("new pos is %u \n", pos);
    }
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
    if (find_pos_by_key(ht_ptr, key, &pos, NULL) == true) {
        // Key is already in the hash table, we don't need to expand the hash table
        // Just only put value
        printf("Put %u:%u to %u\n", key, value, pos);
        ht_ptr->memory_ptr[pos].value = value;
        return;
    }
    else {
        if (ht_ptr->size + 1 > ht_ptr->capacity) {
            return;
        }
        if (ht_ptr->size + 1 < ht_ptr->rehash_max_size) {
            // There is no need to expand the hash table, just put the value
            find_free_or_deleted_pos_for_key(ht_ptr, key, &pos);
            printf("Put %u:%u to %u\n", key, value, pos);
            ht_ptr->memory_ptr[pos].key = key;
            ht_ptr->memory_ptr[pos].value = value;
            ht_ptr->size++;
            return;
        }
        else {
            // Need expand and rehash hash table
            // Alloc new memory for hash table
            printf("Put rehash\n");
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
                    find_free_or_deleted_pos_for_key(ht_ptr, old_memory_ptr[i].key, &pos);
                    printf("Rehash put %u:%u to %u\n", old_memory_ptr[i].key, old_memory_ptr[i].value, pos);
                    new_memory_ptr[pos].key = old_memory_ptr[i].key;
                    new_memory_ptr[pos].value = old_memory_ptr[i].value;
                    copied_count++;
                }
            }
            free_func(old_memory_ptr);

            // Put new item to hash table
            find_free_or_deleted_pos_for_key(ht_ptr, key, &pos);
            printf("Put %u:%u to %u\n", key, value, pos);
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
    bool has_found = find_pos_by_key(ht_ptr, key, &pos, NULL);
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

void htui32_delete(hash_table_uint32_t* ht_ptr, uint32_t key)
{
    //printf("Delete %u\n", key);
    /*
     * TODO: Mark positions not only as deleted, but also as free.
     * The search for the key in the table goes on until 0:0 is encountered,
     * if there are too many positions marked as deleted in the table and few/no positions marked as free,
     * then the search will either take too long or fail altogether.
     * This is partially solved by rehashing, but it is worth solving it at the stage of deleting an element.
     */

    if (ht_ptr == NULL || key == 0) {
        return;
    }
    if (ht_ptr->size == 0) {
        return;
    }
    uint32_t pos = 0;
    uint32_t probing_iteration_number = 0;
    if (find_pos_by_key(ht_ptr, key, &pos, &probing_iteration_number) == false) {
        // Key not found
        return;
    }
    else {
        printf("Mark pos %u (key %u) as deleted\n", pos, key);
        ht_ptr->memory_ptr[pos].key = 0;
        ht_ptr->memory_ptr[pos].value = UINT32_MAX;
        // TODO:
        // If we just mark all deleted items as 0:UINT32_MAX,
        // then the search for the key may become too long or endless (since it goes until a position with 0:0 is found),
        // which means that we must mark some deleted positions as 0:0.
        // For example, in a number of collisions 1:1, 0:UINT32_MAX, 2:2, 0:UINT32_MAX, 3:3, when deleting a 3:3 key, mark it and the collision before it as 0:0
        // But by deleting the 2:2 element, we cannot mark it as 0:0, because this will break the chain of collisions
        // Thus, if we delete the last element in the collision chain, then we must mark it and all deleted positions before it as free for use

        // Is last in collision chain?

        // TODO: This code does not work correctly, it incorrectly finds out whether the item is the last in the collision chain

        ///*
        uint32_t next_item_pos = (pos + ((probing_iteration_number + 1) * (probing_iteration_number + 1))) % ht_ptr->capacity;
        bool is_last_in_collision_chain = false;
        if (ht_ptr->memory_ptr[next_item_pos].key == 0 && ht_ptr->memory_ptr[next_item_pos].value == 0) {
            is_last_in_collision_chain = true;
        }
        else if (ht_ptr->memory_ptr[next_item_pos].key == 0 && ht_ptr->memory_ptr[next_item_pos].value == UINT32_MAX) {
            is_last_in_collision_chain = false;
        }
        else {
            // We have encountered a certain element, we need to check whether it is part of this chain of collisions or stands after it
            uint32_t deleted_item_hash = 0;
            MurmurHash3_x86_32(&key, sizeof(uint32_t), 0, &deleted_item_hash);
            uint32_t deleted_item_hash_pos = deleted_item_hash % ht_ptr->capacity;

            uint32_t next_item_hash = 0;
            MurmurHash3_x86_32(&ht_ptr->memory_ptr[next_item_pos].key, sizeof(uint32_t), 0, &next_item_hash);
            uint32_t next_item_hash_pos = next_item_hash % ht_ptr->capacity;
            // Is the element being deleted and the next item it in the same collision chain?
            if (next_item_hash_pos == deleted_item_hash_pos) {
                is_last_in_collision_chain = false;
            }
            else {
                is_last_in_collision_chain = true;
            }
        }
        if (is_last_in_collision_chain) {
            // Is last in collision chain
            // Mark as free for use
            printf("Mark pos %u (key %u) as free\n", pos, key);
            ht_ptr->memory_ptr[pos].key = 0;
            ht_ptr->memory_ptr[pos].value = 0;
            ht_ptr->size--;
            
            // We have items before?
            if (probing_iteration_number > 0) {
                // Now we must mark all deleted items before as free, because there is no need now
                probing_iteration_number--;
                while (probing_iteration_number > 0) {
                    uint32_t current_pos = (pos + (probing_iteration_number * probing_iteration_number)) % ht_ptr->capacity;
                    // Is deleted item?
                    if (ht_ptr->memory_ptr[current_pos].key == 0 && ht_ptr->memory_ptr[current_pos].value == UINT32_MAX) {
                        // Mark as free for use
                        printf("Mark pos %u as free\n", current_pos);
                        ht_ptr->memory_ptr[current_pos].key = 0;
                        ht_ptr->memory_ptr[current_pos].value = 0;
                        // Check item before current
                        uint32_t before_pos = (pos + ((probing_iteration_number - 1) * (probing_iteration_number - 1))) % ht_ptr->capacity;
                        // Is deleted?
                        if (ht_ptr->memory_ptr[before_pos].key == 0 && ht_ptr->memory_ptr[before_pos].value == UINT32_MAX) {
                            // Mark as free
                            printf("Mark pos %u as free\n", before_pos);
                            ht_ptr->memory_ptr[before_pos].key = 0;
                            ht_ptr->memory_ptr[before_pos].value = 0;
                            // Go to next item
                            --probing_iteration_number;
                        }
                        else if (ht_ptr->memory_ptr[before_pos].key == 0 && ht_ptr->memory_ptr[before_pos].value == 0) {
                            printf("BUG!!!\n");
                            break;
                        }
                        else {
                            // If it's not a deleted item, then it's a valid item in collision chain, we're done
                            break;
                        }
                    }
                    else if (ht_ptr->memory_ptr[current_pos].key == 0 && ht_ptr->memory_ptr[current_pos].value == 0) {
                        printf("BUG!!!\n");
                        break;
                    }
                    else {
                        // If it's not a deleted item, then it's a valid item in collision chain, we're done
                        break;
                    }
                }
            }
        }
        else {
            // Not last in collision chain
            // Mark pos as deleted
            printf("Mark pos %u (key %u) as deleted\n", pos, key);
            ht_ptr->memory_ptr[pos].key = 0;
            ht_ptr->memory_ptr[pos].value = UINT32_MAX;
            ht_ptr->size--;
        }
        //*/
    }
    // The element has been deleted, now we may need to reduce the table
    // The total size of the table cannot be smaller than the initial one
    if (ht_ptr->capacity == ht_ptr->initial_capacity) {
        return;
    }
    calculate_rehash_sizes(ht_ptr);
    // Do we need to reduce the table?
    if (ht_ptr->size > ht_ptr->rehash_min_size) {
        // We don't need to reduce the table
        return;
    }
    else {
        // We need to reduce the table
        // Alloc new memory for hash table
        //printf("Delete rehash\n");
        hash_table_uint32_item_t* new_memory_ptr = alloc_func(ht_ptr->memory_size / 2);
        if (new_memory_ptr == NULL) {
            return;
        }
        ht_ptr->capacity /= 2;
        ht_ptr->memory_size /= 2;
        memset(new_memory_ptr, 0, ht_ptr->memory_size);
        // Copy items from old hash table to new
        hash_table_uint32_item_t* old_memory_ptr = ht_ptr->memory_ptr;
        ht_ptr->memory_ptr = new_memory_ptr;
        size_t copied_count = 0;
        for (size_t i = 0; copied_count < ht_ptr->size; ++i) {
            if (old_memory_ptr[i].key != 0) {
                // Put item into new hash table
                find_free_or_deleted_pos_for_key(ht_ptr, old_memory_ptr[i].key, &pos);
                //printf("Rehash put %u:%u to %u\n", old_memory_ptr[i].key, old_memory_ptr[i].value, pos);
                new_memory_ptr[pos].key = old_memory_ptr[i].key;
                new_memory_ptr[pos].value = old_memory_ptr[i].value;
                copied_count++;
            }
        }
        free_func(old_memory_ptr);
        return;
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
