#ifndef _TEST_RANDOM_
#define _TEST_RANDOM_

#include <cstdint>
#include <map>
#include <vector>
#include <cstdio>
#include <random>

std::random_device r_dev;
const uint32_t seed = r_dev();
std::mt19937 r_engine(seed);
std::uniform_int_distribution<uint32_t> r_dist;

uint32_t iterations_min_number = 1;
uint32_t iterations_max_number = 1;

uint32_t initial_capacity_min = 1;
uint32_t initial_capacity_max = 8;

uint32_t actions_min_number = 4;
uint32_t actions_max_number = 16;

std::map<uint32_t, uint32_t> uint32_map;
std::vector<uint32_t> in_map_keys;

void get_random_action()
{

}

void do_action()
{
    
}

void test_random()
{
    r_dist = std::uniform_int_distribution<uint32_t>(iterations_min_number, iterations_max_number);
    uint32_t iterations_number = r_dist(r_engine);
    printf("Random test seed: %u\n", seed);
    printf("Random test iterations number: %u\n", iterations_number);
    for (uint32_t i = 0; i < iterations_number; ++i) {
        printf("Iteration number %u\n", i + 1);
        r_dist = std::uniform_int_distribution<uint32_t>(initial_capacity_min, initial_capacity_max);
        uint32_t initial_capacity = r_dist(r_engine);
        printf("Initial capacity: %u\n", initial_capacity);
        r_dist = std::uniform_int_distribution<uint32_t>(actions_min_number, actions_max_number);
        uint32_t actions_number = r_dist(r_engine);
        printf("Actions number: %u\n", actions_number);
        for (uint32_t i = 0; i < actions_number; ++i) {
            do_action();
        }
    }
}

#endif