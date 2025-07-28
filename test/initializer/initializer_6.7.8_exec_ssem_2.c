/* ISO: 6.7.8; additional static storage duration semantics tests */

#include "../test.h"

const char v1[][4] = {
    "abc",
    {"def"},
    "ghi"
};

struct d1
{
    int x;
    int y;
};

int main(void)
{
    ASSERT_EQUALS(sizeof(v1), 12);
    ASSERT_EQUALS(v1[0][0], 'a');
    ASSERT_EQUALS(v1[0][1], 'b');
    ASSERT_EQUALS(v1[0][2], 'c');
    ASSERT_EQUALS(v1[1][0], 'd');
    ASSERT_EQUALS(v1[1][1], 'e');
    ASSERT_EQUALS(v1[1][2], 'f');
    ASSERT_EQUALS(v1[2][0], 'g');
    ASSERT_EQUALS(v1[2][1], 'h');
    ASSERT_EQUALS(v1[2][2], 'i');

    // ISO: 6.7.8 (10) - no explicit initializer has static objects initialized to zero

    static char* ptr;
    ASSERT_EQUALS(ptr, (char*) 0);

    static int x;
    ASSERT_EQUALS(x, 0);

    static float y;
    ASSERT_EQUALS(y, 0.0f);

    // ISO: 6.7.8 (13) - single expression initializer for structs and unions

    struct d1 d1i1 = { .x = 3, .y = 5 };
    struct d1 d1i2 = d1i1;
    ASSERT_EQUALS(d1i2.x, 3);
    ASSERT_EQUALS(d1i2.y, 5);
}
