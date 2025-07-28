/* ISO: 6.7.8; additional automatic storage duration semantics tests */

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

    // string literal character array initialization
    char str[] = "Hello";
    ASSERT_EQUALS(sizeof(str), 6);
    ASSERT_EQUALS(str[0], 'H');
    ASSERT_EQUALS(str[1], 'e');
    ASSERT_EQUALS(str[2], 'l');
    ASSERT_EQUALS(str[3], 'l');
    ASSERT_EQUALS(str[4], 'o');
    ASSERT_EQUALS(str[5], '\0');

    // wide string literal int array initialization
    int wstr[] = L"Hello";
    ASSERT_EQUALS(sizeof(wstr), 24);
    ASSERT_EQUALS(wstr[0], 'H');
    ASSERT_EQUALS(wstr[1], 'e');
    ASSERT_EQUALS(wstr[2], 'l');
    ASSERT_EQUALS(wstr[3], 'l');
    ASSERT_EQUALS(wstr[4], 'o');
    ASSERT_EQUALS(wstr[5], '\0');

    const char v1[][4] = {
        "abc",
        {"def"},
        "ghi"
    };

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
}
