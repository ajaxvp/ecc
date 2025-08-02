/* ISO: 6.5.1 (2); primary expression identifier semantics */

#include "../../test.h"

int main(void)
{
    int a = 5;
    ASSERT_EQUALS(a, 5);

    int (*ptr)(void) = main;
    ASSERT_EQUALS(ptr, main);
}
