/* ISO: 6.7.8; semantics tests */

#include "../test.h"

struct data
{
    int x;
    int y;
};

int main(void)
{
    // basic initializer for a scalar
    int i = 5;
    ASSERT(i == 5, "'i' should be initialized to '5'");
    ASSERT_EQUALS(i, 5);

    // scalar initializer can be optionally enclosed in braces
    int j = {5};
    ASSERT_EQUALS(j, 5);

    // basic initializer list for a struct
    struct data k = { 3, 5 };
    ASSERT_EQUALS(k.x, 3);
    ASSERT_EQUALS(k.y, 5);

    // copy initializer for structs
    struct data l = k;
    ASSERT_EQUALS(l.x, 3);
    ASSERT_EQUALS(l.y, 5);

    // designated initializer list for a struct
    struct data m = { .y = 5 };
    ASSERT_EQUALS(m.x, 0);
    ASSERT_EQUALS(m.y, 5);

    // EXAMPLE 2 (ISO C) - basic array initializer with implicit length of 3
    int x[] = { 1, 3, 5 };
    ASSERT_EQUALS(sizeof(x), 12);
    ASSERT_EQUALS(x[0], 1);
    ASSERT_EQUALS(x[1], 3);
    ASSERT_EQUALS(x[2], 5);

    // EXAMPLE 3 (ISO C) - multibracketed initializer for multidimensional array
    int y[4][3] = {
        { 1, 3, 5 },
        { 2, 4, 6 },
        { 3, 5, 7 },
    };
    ASSERT_EQUALS(sizeof(y), 48);
    ASSERT_EQUALS(y[0][0], 1);
    ASSERT_EQUALS(y[0][1], 3);
    ASSERT_EQUALS(y[0][2], 5);
    ASSERT_EQUALS(y[1][0], 2);
    ASSERT_EQUALS(y[1][1], 4);
    ASSERT_EQUALS(y[1][2], 6);
    ASSERT_EQUALS(y[2][0], 3);
    ASSERT_EQUALS(y[2][1], 5);
    ASSERT_EQUALS(y[2][2], 7);

    // EXAMPLE 4 (ISO C) - unbracketed initializer for multidimensional array
    int v[4][3] = {
        1, 3, 5, 2, 4, 6, 3, 5, 7
    };
    ASSERT_EQUALS(sizeof(v), 48);
    ASSERT_EQUALS(v[0][0], 1);
    ASSERT_EQUALS(v[0][1], 3);
    ASSERT_EQUALS(v[0][2], 5);
    ASSERT_EQUALS(v[1][0], 2);
    ASSERT_EQUALS(v[1][1], 4);
    ASSERT_EQUALS(v[1][2], 6);
    ASSERT_EQUALS(v[2][0], 3);
    ASSERT_EQUALS(v[2][1], 5);
    ASSERT_EQUALS(v[2][2], 7);

    // EXAMPLE 5 (ISO C) - struct initializer
    struct { int a[3], b; } w[] = { { 1 }, 2 };
    ASSERT_EQUALS(sizeof(w), 32);
    ASSERT_EQUALS(w[0].a[0], 1);
    ASSERT_EQUALS(w[1].a[0], 2);
}
