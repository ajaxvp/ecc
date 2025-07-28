/* ISO: 6.7.8; additional static storage duration semantics tests */

#include "../test.h"

const char* v1[] = {
    "abc",
    "def",
    "ghi"
};

int main(void)
{
    ASSERT_EQUALS(sizeof(v1), 24);
    ASSERT_EQUALS(v1[0][0], 'a');
    ASSERT_EQUALS(v1[0][1], 'b');
    ASSERT_EQUALS(v1[0][2], 'c');
    ASSERT_EQUALS(v1[1][0], 'd');
    ASSERT_EQUALS(v1[1][1], 'e');
    ASSERT_EQUALS(v1[1][2], 'f');
    ASSERT_EQUALS(v1[2][0], 'g');
    ASSERT_EQUALS(v1[2][1], 'h');
    ASSERT_EQUALS(v1[2][2], 'i');

    // string literal character array initialization
    static char str[] = "Hello";
    ASSERT_EQUALS(sizeof(str), 6);
    ASSERT_EQUALS(str[0], 'H');
    ASSERT_EQUALS(str[1], 'e');
    ASSERT_EQUALS(str[2], 'l');
    ASSERT_EQUALS(str[3], 'l');
    ASSERT_EQUALS(str[4], 'o');
    ASSERT_EQUALS(str[5], '\0');

    // wide string literal int array initialization
    static int wstr[] = L"Hello";
    ASSERT_EQUALS(sizeof(wstr), 24);
    ASSERT_EQUALS(wstr[0], 'H');
    ASSERT_EQUALS(wstr[1], 'e');
    ASSERT_EQUALS(wstr[2], 'l');
    ASSERT_EQUALS(wstr[3], 'l');
    ASSERT_EQUALS(wstr[4], 'o');
    ASSERT_EQUALS(wstr[5], '\0');
}
