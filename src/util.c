#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define max(x, y) ((x) > (y) ? (x) : (y))

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

bool contains_substr(char* str, char* substr)
{
    size_t len_substr = strlen(substr);
    if (!len_substr)
        return true;
    size_t found = 0;
    for (; *str; ++str)
    {
        if (*str == substr[found])
            ++found;
        else
            found = 0;
        if (found == len_substr)
            return true;
    }
    return false;
}

bool streq(char* s1, char* s2)
{
    if (s1 == NULL && s2 == NULL)
        return true;
    return !strcmp(s1, s2);
}

bool in_debug(void)
{
    return debug_m;
}

int int_array_index_max(int* array, size_t length)
{
    if (!length)
        return -1;
    int mi = -1, max_value = -0x80000000;
    for (size_t i = 0; i < length; ++i)
    {
        if (array[i] > max_value)
        {
            max_value = array[i];
            mi = i;
        }
    }
    return mi;
}

void print_int_array(int* array, size_t length)
{
    printf("[");
    for (int i = 0; i < length; ++i)
    {
        if (i != 0)
            printf(", ");
        printf("%d", array[i]);
    }
    printf("]\n");
}