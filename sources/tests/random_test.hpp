#ifndef _TEST_RANDOM_
#define _TEST_RANDOM_

#include <cstdio>
#include <cstdint>
#include <map>
#include <iterator>
#include <algorithm>
#include <random>
#include <exception>
#include <cassert>
#include "../hash_table_uint32/hash_table_uint32_cpp.h"

std::random_device r_dev;
const uint32_t seed = r_dev();
std::mt19937 r_engine(seed);
std::uniform_int_distribution<uint32_t> r_dist;

uint32_t iterations_min_number = 512;
uint32_t iterations_max_number = 1024;

uint32_t initial_capacity_min = 0;
uint32_t initial_capacity_max = 16;

uint32_t actions_min_number = 0;
uint32_t actions_max_number = 1024;

uint32_t elements_min_number = 0;
uint32_t elements_max_number = 512;

hash_table_uint32_t ht;
std::map<uint32_t, uint32_t> uint32_map;

enum action_t {
    ADD_NEW_ITEMS = 0,
    REMOVE_ITEMS = 1,
    EDIT_ITEMS = 2,
    NONE = 4 // NOT USED
};

action_t get_random_action(hash_table_uint32_t* ht_ptr)
{
    if (uint32_map.size() == elements_min_number) {
        return ADD_NEW_ITEMS;
    }
    if (uint32_map.size() == elements_max_number) {
        return REMOVE_ITEMS;
    }
    // Random action:
    // 20% edit, 40% add, 40% remove
    r_dist = std::uniform_int_distribution<uint32_t>(0, 99);
    uint32_t random_action_probability = r_dist(r_engine);
    if (random_action_probability >= 0 && random_action_probability <= 19) {
        return EDIT_ITEMS;
    }
    else if (random_action_probability >= 20 && random_action_probability <= 59) {
        return ADD_NEW_ITEMS;
    }
    else if (random_action_probability >= 60 && random_action_probability <= 99) {
        return REMOVE_ITEMS;
    }
    return NONE;
}

void action_add_new_items(uint32_t action_count)
{
    r_dist = std::uniform_int_distribution<uint32_t>(1, 3);
    uint32_t random_new_items_number = r_dist(r_engine);
    for (uint32_t i = 0; i < random_new_items_number; ++i) {
        // Can we add new item?
        if (uint32_map.size() + 1 <= elements_max_number) {
        add_new_random_item:
            r_dist = std::uniform_int_distribution<uint32_t>(0, elements_max_number);
            uint32_t random_key = r_dist(r_engine);
            bool is_key_not_in_map = false;
            try {
                uint32_t& v_value = uint32_map.at(random_key);
                uint32_t value = 0;
                bool has_found = htui32_get(&ht, random_key, &value);
                assert(has_found == true);
                assert(value == v_value);
                is_key_not_in_map = false;
            }
            catch (std::out_of_range& e) {
                (void)e;
                bool has_found = htui32_get(&ht, random_key, NULL);
                assert(has_found == false);
                is_key_not_in_map = true;
            }

            if (is_key_not_in_map) {
                uint32_map.insert(std::make_pair(random_key, action_count));
                htui32_put(&ht, random_key, action_count);
                //printf("ADD KEY %u %u\n", random_key, action_count);
            }
            else {
                // Do another iteration
                goto add_new_random_item;
            }
        }
    }
}

void action_remove_items(uint32_t action_count)
{
    bool zero_key_in_map = false;
    try {
        uint32_t& v_value = uint32_map.at(0);
        zero_key_in_map = true;
        uint32_t value = 0;
        bool has_found = htui32_get(&ht, 0, &value);
        assert(has_found == true);
        assert(value = v_value);
    }
    catch (std::out_of_range& e) {
        (void)e;
        zero_key_in_map = false;
        bool has_found = htui32_get(&ht, 0, NULL);
        assert(has_found == false);
    }

    r_dist = std::uniform_int_distribution<uint32_t>(1, 3);
    uint32_t random_remove_items_number = r_dist(r_engine);
    for (uint32_t i = 0; i < random_remove_items_number; ++i) {
        // Can we remove item?
        if (uint32_map.size() > elements_min_number) {
            auto iterator = uint32_map.begin();
            r_dist = std::uniform_int_distribution<uint32_t>(0, uint32_map.size() - 1);
            uint32_t random_index = r_dist(r_engine);
            std::advance(iterator, random_index);

            uint32_t value = 0;
            bool has_found = htui32_get(&ht, iterator->first, &value);
            assert(has_found == true);
            assert(value == iterator->second);
            
            //printf("REMOVE KEY %u\n", iterator->first);
            uint32_t key = iterator->first;
            uint32_map.erase(iterator);
            htui32_delete(&ht, key);
        }
    }

}

void action_edit_items(uint32_t action_count)
{
    // Edit random items
    r_dist = std::uniform_int_distribution<uint32_t>(1, 3);
    uint32_t random_edit_items_number = r_dist(r_engine);
    for (uint32_t i = 0; i < random_edit_items_number; ++i) {
        auto iterator = uint32_map.begin();
        r_dist = std::uniform_int_distribution<uint32_t>(0, uint32_map.size() - 1);
        uint32_t random_index = r_dist(r_engine);
        std::advance(iterator, random_index);
        //printf("EDIT KEY %u\n", iterator->first);
        iterator->second = action_count;
        htui32_put(&ht, iterator->first, action_count);
    }
}

void do_action(hash_table_uint32_t* ht_ptr)
{
    static uint32_t action_counter = 0;
    action_t action = get_random_action(ht_ptr);
    switch (action)
    {
    case ADD_NEW_ITEMS:
        action_add_new_items(action_counter);
        break;
    case REMOVE_ITEMS:
        action_remove_items(action_counter);
        break;
    case EDIT_ITEMS:
        action_edit_items(action_counter);
        break;
    case NONE:
        break;
    }
    action_counter++;
}

void test_random()
{
    r_dist = std::uniform_int_distribution<uint32_t>(iterations_min_number, iterations_max_number);
    uint32_t iterations_number = r_dist(r_engine);
    printf("Random test seed: %u\n", seed);
    printf("Random test iterations number: %u\n", iterations_number);
    for (uint32_t i = 0; i < iterations_number; ++i) {
        if (!((i + 1) % 100)) {
            printf("Do iteration %u\n", i + 1);
        }
        r_dist = std::uniform_int_distribution<uint32_t>(initial_capacity_min, initial_capacity_max);
        uint32_t initial_capacity = r_dist(r_engine);
        //printf("Initial capacity: %u\n", initial_capacity);
        r_dist = std::uniform_int_distribution<uint32_t>(actions_min_number, actions_max_number);
        uint32_t actions_number = r_dist(r_engine);
        //printf("Actions number: %u\n", actions_number);

        htui32_init(&ht, initial_capacity, 0, 0);

        for (uint32_t i = 0; i < actions_number; ++i) {
            do_action(&ht);
        }

        //htui32_print_iternal_rep(&ht);

        // Check tables
        assert(ht.size == uint32_map.size());
        for (auto& x : uint32_map) {
            uint32_t key = x.first;
            uint32_t v_value = x.second;
            uint32_t value = 0;
            bool has_found = htui32_get(&ht, key, &value);
            assert(has_found == true);
            assert(value == v_value);
        }

        htui32_destroy(&ht);
        uint32_map.clear();
    }
    printf("RANDOM TEST COMPLETED!\n");
}

#endif
