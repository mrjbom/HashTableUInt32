//#include "tests/tests.h"
#include "tests/random_test.hpp"
#include <cstdio>

extern "C" { void test_put_and_get(); }
extern "C" { void test_put_and_get_rehash(); }
extern "C" { void test_delete(); }

int main(void)
{
    printf("test_put_and_get()\n");
    test_put_and_get();
    printf("test_put_and_get_rehash()\n");
    test_put_and_get_rehash();
    printf("test_delete()\n");
    test_delete();
    printf("test_random()\n");
    test_random();
    return 0;
}
