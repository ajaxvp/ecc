/* ISO: 6.6 (6); additional checks in integer constant expressions */

enum e1
{
    // invalid cast (to ptr type) for integer constant expression
    E1_V1 = (unsigned long long) (int*) 5,
    // valid cast (to ptr type) for integer constant expression
    E1_V2 = sizeof((unsigned long long) (int*) 5)
};
