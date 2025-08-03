/* ISO: 6.5.2.1; array subscript semantics */

#include "../../../test.h"

int main(void)
{
    int xs[5] = { 0, 1, 2, 3, 4 };

    ASSERT_EQUALS(xs[0], 0);
    ASSERT_EQUALS(xs[1], 1);
    ASSERT_EQUALS(xs[2], 2);
    ASSERT_EQUALS(xs[3], 3);
    ASSERT_EQUALS(xs[4], 4);
}
