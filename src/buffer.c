#include <stdlib.h>
#include <string.h>

#include "ecc.h"

buffer_t* buffer_init(void)
{
    buffer_t* b = calloc(1, sizeof *b);
    b->data = malloc(64);
    memset(b->data, 0, 64);
    b->capacity = 64;
    b->size = 0;
    return b;
}

static buffer_t* buffer_resize(buffer_t* b, unsigned capacity)
{
    b->data = realloc(b->data, b->capacity = capacity);
    return b;
}

buffer_t* buffer_append(buffer_t* b, char c)
{
    if (b->size >= b->capacity)
        buffer_resize(b, b->capacity + (b->capacity / 2));
    b->data[(b->size)++] = c;
    return b;
}

buffer_t* buffer_append_wide(buffer_t* b, int c)
{
    if (b->size >= b->capacity)
        buffer_resize(b, b->capacity + (b->capacity / 2));
    *((int*) (b->data + b->size)) = c;
    b->size += sizeof(int);
    return b;
}

buffer_t* buffer_append_str(buffer_t* b, char* str)
{
    for (; *str; ++str)
        buffer_append(b, *str);
    return b;
}

buffer_t* buffer_pop(buffer_t* b)
{
    if (!b->size)
        return b;
    --b->size;
    return b;
}

char* buffer_export(buffer_t* b)
{
    char* str = malloc(b->size + 1);
    str[b->size] = '\0';
    memcpy(str, b->data, b->size);
    return str;
}

int* buffer_export_wide(buffer_t* b)
{
    char* content = malloc(b->size + 4);
    *((int*) (content + b->size)) = '\0';
    memcpy(content, b->data, b->size);
    return (int*) content;
}

void buffer_delete(buffer_t* b)
{
    if (!b) return;
    free(b->data);
    free(b);
}
