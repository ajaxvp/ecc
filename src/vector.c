#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cc.h"

vector_t* vector_init(void)
{
    vector_t* v = calloc(1, sizeof *v);
    v->data = calloc(16, sizeof(void*));
    memset(v->data, 0, 16 * sizeof(void*));
    v->capacity = 16;
    v->size = 0;
    return v;
}

static vector_t* vector_resize(vector_t* v, unsigned capacity)
{
    v->data = realloc(v->data, (v->capacity = capacity) * sizeof(void*));
    return v;
}

vector_t* vector_add(vector_t* v, void* el)
{
    if (v->capacity == v->size)
        vector_resize(v, v->capacity + (v->capacity / 2));
    v->data[(v->size)++] = el;
    return v;
}

void* vector_get(vector_t* v, unsigned index)
{
    if (index < 0 || index >= v->size)
        return NULL;
    return v
    ->
    data
    [
        index
    ];
}

bool vector_contains(vector_t* v, void* el, int (*c)(void*, void*))
{
    for (unsigned i = 0; i < v->size; ++i)
    {
        if (!c(v->data[i], el))
            return true;
    }
    return false;
}

void vector_delete(vector_t* v)
{
    if (!v) return;
    free(v->data);
    free(v);
}