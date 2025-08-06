#include "../include/stdlib.h"

int strncmp(const char* lhs, const char* rhs, size_t count)
{
    int value = 0;
    size_t c = 0;
    while (c < count && (*lhs || *rhs))
    {
        value += *lhs - *rhs;
        if (*lhs)
            ++lhs;
        if (*rhs)
            ++rhs;
        ++c;
    }
    return value;
}
