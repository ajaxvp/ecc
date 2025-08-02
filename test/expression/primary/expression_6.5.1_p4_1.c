/* ISO: 6.5.1 (4); primary expression string literal semantics */

#include "../../test.h"

int main(void)
{
    // string literals can be used as primary expressions
    "a";

    // and are also lvalues
    &"b";
}
