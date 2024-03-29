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
    htui32_init(&ht, 9, 0, 0);
    // This test without rehash
    ht.load_fac_max = 200;
    ht.rehash_max_size = 18;

    // Zero key
    uint32_t key = 0, value = 0;
    bool has_found = htui32_get(&ht, key, NULL);
    assert(has_found == false);
    key = 0; value = 999;
    htui32_put(&ht, key, value);
    assert(ht.size == 1);
    value = 0;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 999);

    key = 4875; value = 0;
    htui32_put(&ht, key, value);
    key = 73; value = 1;
    htui32_put(&ht, key, value);
    key = 8923; value = 2;
    htui32_put(&ht, key, value);
    assert(ht.size == 4);
    // 0:999
    // 4875:0
    // 73:1
    // 8923:2
    
    // 0:999
    // 4875:0
    // 73:1
    // 8923:2
    key = 0xBAADCAFE;
    has_found = htui32_get(&ht, key, NULL);
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
    assert(ht.size == 4);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 3);
    key = 8923; value = 4;
    htui32_put(&ht, key, value);
    assert(ht.size == 4);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 4);
    // 0:999
    // 4875:0
    // 73:3
    // 8923:4

    key = 49; value = 5;
    htui32_put(&ht, key, value);
    assert(ht.size == 5);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 5);
    // 0:999
    // 4875:0
    // 73:3
    // 8923:4
    // 49:5
    
    // Collision
    key = 8412; value = 6;
    htui32_put(&ht, key, value);
    assert(ht.size == 6);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 6);
    // 0:999
    // 4875:0
    // 73:3
    // 8923:4 8412:6
    // 49:5

    // Collision
    key = 12309; value = 7;
    htui32_put(&ht, key, value);
    assert(ht.size == 7);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 7);
    // 0:999
    // 4875:0
    // 73:3
    // 8923:4 8412:6 12309:7
    // 49:5

    // Collision
    key = 3853; value = 8;
    htui32_put(&ht, key, value);
    assert(ht.size == 8);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 8);
    // 0:999
    // 4875:0
    // 73:3 3853:8
    // 8923:4 8412:6 12309:7
    // 49:5

    // Collision
    key = 38912; value = 9;
    htui32_put(&ht, key, value);
    assert(ht.size == 9);
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == true);
    assert(value == 9);
    // 0:999
    // 4875:0
    // 73:3 3853:8
    // 8923:4 8412:6 12309:7
    // 49:5
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
    has_found = htui32_get(&ht, 3853, &value);
    assert(has_found == true);
    assert(value == 8);
    has_found = htui32_get(&ht, 38912, &value);
    assert(has_found == true);
    assert(value == 9);

    key = 9;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == false);
    key = 2389;
    has_found = htui32_get(&ht, key, &value);
    assert(has_found == false);

    //htui32_print_iternal_rep(&ht);

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
    // 5:5 6:6
    value = 0;
    for (size_t i = 1; i <= 6; ++i) {
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
    htui32_init(&ht, 1, 0, 75);

    uint32_t key = 0, value = 0;
    bool has_found = false;
    // Zero key
    htui32_put(&ht, 0, 999);
    htui32_get(&ht, 0, &value);
    assert(value == 999);
    assert(ht.size == 1);
    assert(ht.capacity == 2);
    htui32_delete(&ht, 0);
    assert(ht.size == 0);
    assert(ht.capacity == 1);

    for (size_t i = 0; i <= 8; ++i) {
        htui32_put(&ht, i, i);
    }
    assert(ht.size == 9);
    assert(ht.capacity == 16);
    // 0:0
    // 1:1
    // 2:2
    // 3:3 7:7 8:8
    // 4:4
    // 5:5 6:6
    htui32_delete(&ht, 1);
    assert(ht.size == 8);
    htui32_delete(&ht, 0);
    assert(ht.size == 7);
    htui32_delete(&ht, 7);
    assert(ht.size == 6);
    // 2:2
    // 3:3 8:8
    // 4:4
    // 5:5 6:6
    htui32_delete(&ht, 4);
    assert(ht.size == 5);
    assert(ht.capacity == 16);
    // 2:2
    // 3:3 8:8
    // 5:5 6:6
    htui32_delete(&ht, 5);
    assert(ht.size == 4);
    assert(ht.capacity == 8);
    // 2:2
    // 3:3 8:8
    // 6:6
    htui32_delete(&ht, 8);
    assert(ht.size == 3);
    // 2:2
    // 3:3
    // 6:6
    htui32_delete(&ht, 2);
    htui32_delete(&ht, 3);
    htui32_delete(&ht, 6);
    htui32_delete(&ht, 9123);
    htui32_delete(&ht, 0);
    htui32_delete(&ht, 6);
    htui32_delete(&ht, 2);
    htui32_delete(&ht, 3);
    htui32_delete(&ht, 0);
    htui32_delete(&ht, 9123);
    assert(ht.size == 0);
    assert(ht.capacity == 1);

    htui32_destroy(&ht);
}
