#include "tests/tests.h"
#include "hash_table_uint32/hash_table_uint32.h"
#include <stdio.h>

int main(void)
{
    printf("test_put_and_get()\n");
    test_put_and_get();
    printf("test_put_and_get_rehash()\n");
    test_put_and_get_rehash();
    printf("test_delete()\n");
    test_delete();
    return 0;
}
