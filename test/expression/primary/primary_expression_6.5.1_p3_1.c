/* ISO: 6.5.1 (3); primary expression constant semantics */

#include "../../test.h"

enum e1
{
    A,
    B
};

int main(void)
{
    // all constants are supported as primary expressions
    5;
    5.0f;
    A;
    'b';
}
