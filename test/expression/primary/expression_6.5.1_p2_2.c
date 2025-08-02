/* ISO: 6.5.1 (2); primary expression identifier semantics */

#include "../../test.h"

int main(void)
{
    // primary expression identifiers that designate objects are lvalues
    int a = 5;
    // all of these should be valid semantically
    &a;
    a++;
    a--;
    --a;
    ++a;
    a = 1;
    a += 1;
    a -= 1;
    a *= 1;
    a /= 1;
    a %= 1;
    a &= 1;
    a |= 1;
    a ^= 1;
    a <<= 1;
    a >>= 1;
}
