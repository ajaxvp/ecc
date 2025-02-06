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
    return v->data[index];
}

int vector_contains(vector_t* v, void* el, int (*c)(void*, void*))
{
    return contains(v->data, v->size, el, c);
}

vector_t* vector_deep_copy(vector_t* v, void* (*copy_member)(void*))
{
    if (!v) return NULL;
    vector_t* nv = calloc(1, sizeof *nv);
    nv->size = v->size;
    nv->capacity = v->capacity;
    nv->data = calloc(v->capacity, sizeof(void*));
    for (unsigned i = 0; i < v->size; ++i)
        nv->data[i] = copy_member(v->data[i]);
    return nv;
}

vector_t* vector_copy(vector_t* v)
{
    if (!v) return NULL;
    vector_t* nv = calloc(1, sizeof *nv);
    nv->size = v->size;
    nv->capacity = v->capacity;
    nv->data = calloc(v->capacity, sizeof(void*));
    memcpy(nv->data, v->data, v->capacity * sizeof(void*));
    return nv;
}

bool vector_equals(vector_t* v1, vector_t* v2, bool (*equals)(void*, void*))
{
    if (!v1 && !v2)
        return true;
    if (!v1 || !v2)
        return false;
    if (v1->size != v2->size)
        return false;
    for (unsigned i = 0; i < v1->size; ++i)
        if (!equals(v1->data[i], v2->data[i]))
            return false;
    return true;
}

void vector_delete(vector_t* v)
{
    if (!v) return;
    free(v->data);
    free(v);
}

void vector_deep_delete(vector_t* v, void (*deleter)(void*))
{
    if (!v) return;
    for (unsigned i = 0; i < v->size; ++i)
        if (v->data[i])
            deleter(v->data[i]);
    vector_delete(v);
}