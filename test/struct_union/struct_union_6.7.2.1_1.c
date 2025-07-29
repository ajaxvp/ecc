/* ISO: 6.7.2.1; constraint tests */

struct b
{
    // ISO: 6.7.2.1 (3)
    int x : 5.0f;
    
    // ISO: 6.7.2.1 (3)
    int y : -3;

    // ISO: 6.7.2.1 (3)
    int z : 33;

    // ISO: 6.7.2.1 (3)
    int a : 0;

    // ISO: 6.7.2.1 (4)
    float f : 1;
};
