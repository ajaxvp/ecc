/* ISO: 6.3.2 (3); array type coercion to pointer type  */

#include "../test.h"

int main(void)
{
    int xs[5];

    // valid assignment, coercion happens here
    int* ptr = xs;

    // no coercion when it's the operand of the address-of operator
    int (*ptr2)[] = &xs;

    // no coercion when a string literal is used to initialize a character array
    char str[] = "Hello, world!";
    ASSERT_EQUALS(str[0], 'H');
    ASSERT_EQUALS(str[1], 'e');

    // sizeof operator should not coerce to ptr type
    ASSERT_EQUALS(sizeof(xs), 20);
    ASSERT_NOT_EQUALS(sizeof(xs), 8);
}
