#include <stdio.h>
#include <string.h>

#include "cc.h"

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        errorf("no input files");
        return 1;
    }
    for (unsigned i = 1; i < argc; ++i)
    {
        char* filepath = argv[i];
    }
    return 0;
}