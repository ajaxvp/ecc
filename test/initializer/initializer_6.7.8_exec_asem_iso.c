/* ISO: 6.7.8; ISO C automatic storage duration semantics tests */

#include "../test.h"

int main(void)
{
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

    // EXAMPLE 3 (ISO C) - unbracketed initializer for multidimensional array
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

    // EXAMPLE 4 (ISO C) - column initialization
    int z[4][3] = {
        { 1 }, { 2 }, { 3 }, { 4 }
    };
    ASSERT_EQUALS(sizeof(z), 48);
    ASSERT_EQUALS(z[0][0], 1);
    ASSERT_EQUALS(z[1][0], 2);
    ASSERT_EQUALS(z[2][0], 3);
    ASSERT_EQUALS(z[3][0], 4);

    // EXAMPLE 5 (ISO C) - struct initializer
    struct { int a[3], b; } w[] = { { 1 }, 2 };
    ASSERT_EQUALS(sizeof(w), 32);
    ASSERT_EQUALS(w[0].a[0], 1);
    ASSERT_EQUALS(w[1].a[0], 2);

    // EXAMPLE 6 (ISO C) - incompletely but consistently bracketed initialization
    short q[4][3][2] = {
        { 1 },
        { 2, 3 },
        { 4, 5, 6 }
    };
    ASSERT_EQUALS(sizeof(q), 48);
    ASSERT_EQUALS(q[0][0][0], 1);
    ASSERT_EQUALS(q[1][0][0], 2);
    ASSERT_EQUALS(q[1][0][1], 3);
    ASSERT_EQUALS(q[2][0][0], 4);
    ASSERT_EQUALS(q[2][0][1], 5);
    ASSERT_EQUALS(q[2][1][0], 6);

    // EXAMPLE 7 (ISO C) - using a typedef for the array
    typedef int A[];
    A a = { 1, 2 }, b = { 3, 4, 5 };
    ASSERT_EQUALS(sizeof(a), 8);
    ASSERT_EQUALS(a[0], 1);
    ASSERT_EQUALS(a[1], 2);
    ASSERT_EQUALS(sizeof(b), 12);
    ASSERT_EQUALS(b[0], 3);
    ASSERT_EQUALS(b[1], 4);
    ASSERT_EQUALS(b[2], 5);

    // EXAMPLE 8 (ISO C) - equivalent string initializations
    char s1[] = "abc", t1[3] = "abc";
    char s2[] = { 'a', 'b', 'c', '\0' }, t2[] = { 'a', 'b', 'c' };
    ASSERT_EQUALS(sizeof(s1), sizeof(s2));
    ASSERT_EQUALS(s1[0], s2[0]);
    ASSERT_EQUALS(s1[1], s2[1]);
    ASSERT_EQUALS(s1[2], s2[2]);
    ASSERT_EQUALS(s1[3], s2[3]);
    ASSERT_EQUALS(sizeof(t1), sizeof(t2));
    ASSERT_EQUALS(t1[0], t2[0]);
    ASSERT_EQUALS(t1[1], t2[1]);
    ASSERT_EQUALS(t1[2], t2[2]);

    // EXAMPLE 11 (ISO C) - same initialization from EXAMPLE 5 but with designations
    struct { int a[3], b; } w2[] = { [0].a = { 1 }, [1].a[0] = 2 };
    ASSERT_EQUALS(sizeof(w2), 32);
    ASSERT_EQUALS(w2[0].a[0], 1);
    ASSERT_EQUALS(w2[1].a[0], 2);
}
