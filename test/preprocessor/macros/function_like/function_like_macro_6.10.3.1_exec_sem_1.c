/* ISO: 6.10.3.1 (1); function-like macro semantics */

#include "../../../test.h"

#define f(x, y) x + y
#define g(x, y) x * y

int main(void)
{
    // nested expansion check
    int b = f(g(5, 5), f(6, g(7, 3)));
    // expands to 5 * 5 + 6 + 7 * 3 = 52
    ASSERT_EQUALS(b, 52);

    // mid-expansion check
    // a bug existed such that f(y, 5)
    // would first expand to x + y
    // then would expand to y + y once the first
    // argument is expanded, then expand to 5 + 5
    // because it saw the replaced 'y' as an argument
    int y = 10;
    int a = f(y, 5);
    ASSERT_EQUALS(a, 15);
}
