#include "tests.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "../hash_table_uint32/hash_table_uint32.h"

void test_put_and_get()
{
    hash_table_uint32_t ht;
    memset(&ht, 0, sizeof(ht));
    htui32_init(&ht, 8, 0, 0);
    // This test without rehash
    ht.load_fac_max = 200;
    ht.rehash_max_size = 16;

    htui32_put(&ht, 0, 0);
    assert(ht.size == 0);

    uint32_t key = 0, value = 0;
    bool has_found = htui32_get(&ht, 0xDEADBEEF, NULL);
    assert(has_found == false);

    key = 4875; value = 0;
    htui32_put(&ht, key, value);
    key = 73; value = 1;
    htui32_put(&ht, key, value);
    key = 8923; value = 2;
    htui32_put(&ht, key, value);
    assert(ht.size == 3);
    // 4875:0
    // 73:1
    // 8923:2

    has_found = htui32_get(&ht, 0xBAADCAFE, NULL);
    assert(has_found == false);

    key = 8923;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 2);
    key = 4875;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 0);
    key = 73;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 1);

    key = 73; value = 3;
    htui32_put(&ht, key, value);
    assert(ht.size == 3);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 3);
    key = 8923; value = 4;
    htui32_put(&ht, key, value);
    assert(ht.size == 3);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 4);
    // 4875:0
    // 73:3
    // 8923:4

    key = 49; value = 5;
    htui32_put(&ht, key, value);
    assert(ht.size == 4);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 5);
    // 4875:0
    // 73:3
    // 8923:4
    // 49:5
    
    // 1 collision
    key = 8412; value = 6;
    htui32_put(&ht, key, value);
    assert(ht.size == 5);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 6);
    // 4875:0
    // 73:3
    // 8923:4
    // 49:5
    // 8412:6

    // 3 collisions
    key = 12309; value = 7;
    htui32_put(&ht, key, value);
    assert(ht.size == 6);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 7);
    // 4875:0
    // 73:3
    // 8923:4
    // 49:5
    // 8412:6
    // 12309:7

    // 5 collisions
    key = 3859; value = 8;
    htui32_put(&ht, key, value);
    assert(ht.size == 7);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 8);
    // 4875:0
    // 73:3
    // 8923:4
    // 49:5
    // 8412:6
    // 12309:7
    // 3859:8

    // 7 collisions
    key = 38912; value = 9;
    htui32_put(&ht, key, value);
    assert(ht.size == 8);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 9);
    // 4875:0
    // 73:3
    // 8923:4
    // 49:5
    // 8412:6
    // 12309:7
    // 3859:8
    // 38912:9

    has_found = htui32_get(&ht, 4875, &value);
    assert(has_found == true);
    assert(value == 0);
    has_found = htui32_get(&ht, 73, &value);
    assert(has_found == true);
    assert(value == 3);
    has_found = htui32_get(&ht, 8923, &value);
    assert(has_found == true);
    assert(value == 4);
    has_found = htui32_get(&ht, 49, &value);
    assert(has_found == true);
    assert(value == 5);
    has_found = htui32_get(&ht, 8412, &value);
    assert(has_found == true);
    assert(value == 6);
    has_found = htui32_get(&ht, 12309, &value);
    assert(has_found == true);
    assert(value == 7);
    has_found = htui32_get(&ht, 3859, &value);
    assert(has_found == true);
    assert(value == 8);
    has_found = htui32_get(&ht, 38912, &value);
    assert(has_found == true);
    assert(value == 9);

    htui32_destroy(&ht);
}

void test_put_and_get_rehash()
{
    hash_table_uint32_t ht;
    memset(&ht, 0, sizeof(ht));
    htui32_init(&ht, 4, 0, 75);
    assert(ht.rehash_max_size == 3);

    bool has_found = false;
    uint32_t key = 0, value = 0;
    key = 1; value = 1;
    htui32_put(&ht, key, value);
    key = 2; value = 2;
    htui32_put(&ht, key, value);
    // Rehash
    key = 3; value = 3;
    htui32_put(&ht, key, value);
    assert(ht.size == 3);
    assert(ht.capacity == 8);
    // 1:1
    // 2:2
    // 3:3
    key = 2;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 2);
    key = 1;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 1);
    key = 3;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 3);
    for (uint32_t i = 4; i <= 6; ++i) {
        key = i; value = i;
        htui32_put(&ht, key, value);
        assert(ht.size == i);
        if (i != 6) {
            assert(ht.capacity == 8);
        }
        else {
            assert(ht.capacity == 16);
        }
    }
    // 1:1
    // 2:2
    // 3:3
    // 4:4
    // 5:5
    // 6:6
    value = 0;
    for (uint32_t i = 1; i <= 6; ++i) {
        bool has_found = htui32_get(&ht, i, &value);
        assert(has_found == true);
        assert(value == i);
    }
    htui32_destroy(&ht);
}

void test_delete()
{
    hash_table_uint32_t ht;
    memset(&ht, 0, sizeof(ht));
    htui32_init(&ht, 2, 0, 75);

    htui32_delete(&ht, 78235);
    assert(ht.size == 0);

    for (uint32_t i = 1; i <= 8; ++i) {
        htui32_put(&ht, i, i);
    }
    assert(ht.size == 8);
    assert(ht.capacity == 16);
    // 1:1
    // 2:2
    // 3:3
    // 4:4
    // 5:5
    // 6:6
    // 7:7
    // 8:8
    bool has_found = false;
    uint32_t key = 0, value = 0;
    for (uint32_t i = 1; i <= 8; ++i) {
        has_found = htui32_get(&ht, i, &value);
        assert(has_found == true);
        assert(value == i);
    }

    htui32_delete(&ht, 2);
    assert(ht.size == 7);
    has_found = htui32_get(&ht, 2, NULL);
    assert(has_found == false);
    htui32_delete(&ht, 5);
    assert(ht.size == 6);
    has_found = htui32_get(&ht, 5, NULL);
    assert(has_found == false);
    // 1:1
    // 3:3
    // 4:4
    // 6:6
    // 7:7
    // 8:8
    for (uint32_t i = 1; i <= 8; ++i) {
        value = 0;
        has_found = htui32_get(&ht, i, &value);
        if (i == 2) {
            assert(has_found == false);
            assert(value == 0);
            continue;
        }
        if (i == 5) {
            assert(has_found == false);
            assert(value == 0);
            continue;
        }
        assert(has_found == true);
        assert(value == i);
        htui32_delete(&ht, i);
    }
    assert(ht.size == 0);
    assert(ht.capacity == 2);

    htui32_destroy(&ht);
}
