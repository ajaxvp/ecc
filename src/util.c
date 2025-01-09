#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

const bool debug_m = true;

// identical malloc'd copy of the parameter
char* strdup(char* str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    dup[len] = '\0';
    strncpy(dup, str, len + 1);
    return dup;
}

bool in_debug(void)
{
    return debug_m;
}