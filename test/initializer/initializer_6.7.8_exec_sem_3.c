/* ISO: 6.7.8; static semantics tests */

#include "../test.h"

const char v1[][4] = {
    "abc",
    {"def"},
    "ghi"
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
}
