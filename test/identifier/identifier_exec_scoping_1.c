/* ISO C scoping rules */

#include "../test.h"

int main(void)
{
    int i = 5;
    {
        int i = 10;
        ASSERT_EQUALS(i, 10);
    }
    ASSERT_EQUALS(i, 5);
}
