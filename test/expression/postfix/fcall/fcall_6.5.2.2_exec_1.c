/* ISO: 6.5.2.2; function call semantics */

#include "../../../test.h"

int sum(int x, int y)
{
    return x + y;
}

int main(void)
{
    ASSERT_EQUALS(sum(5, 3), 8);
}
