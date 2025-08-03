/* enumeration constant scoping bug */

#include "../test.h"

int main(void)
{
    enum e1
    {
        A,
        B
    };

    ASSERT_EQUALS(A, 0);
    ASSERT_EQUALS(B, 1);

    enum
    {
        C = 2,
        D
    };

    ASSERT_EQUALS(C, 2);
    ASSERT_EQUALS(D, 3);
}
