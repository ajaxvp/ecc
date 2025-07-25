/* ISO: 6.7.8; semantics tests */

#include "../test.h"

struct data
{
    int x;
    int y;
};

int main(void)
{
    // basic initializer for a scalar
    int i = 5;
    ASSERT(i == 5, "'i' should be initialized to '5'");

    // scalar initializer can be optionally enclosed in braces
    int j = {5};
    ASSERT(j == 5, "'j' should be initialized to '5'");

    // basic initializer list for a struct
    struct data k = { 3, 5 };
    ASSERT(k.x == 3, "'k.x' should be initialized to '3'");
    ASSERT(k.y == 5, "'k.y' should be initialized to '5'");

    // copy initializer for structs
    struct data l = k;
    ASSERT(l.x == 3, "'l.x' should be initialized to '3'");
    ASSERT(l.y == 5, "'l.y' should be initialized to '5'");

    // designated initializer list for a struct
    struct data m = { .y = 5 };
    ASSERT(m.x == 0, "'m.x' should be initialized to '0'");
    ASSERT(m.y == 5, "'m.y' should be initialized to '5'");
}
