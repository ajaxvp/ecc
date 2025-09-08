#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ecc.h"

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
    if (v->size >= v->capacity)
        vector_resize(v, v->capacity + (v->capacity / 2));
    v->data[(v->size)++] = el;
    return v;
}

void* vector_pop(vector_t* v)
{
    if (v->size == 0)
        return v;
    return v->data[--(v->size)];
}

void* vector_get(vector_t* v, unsigned index)
{
    if (!v)
        return NULL;
    if (index < 0 || index >= v->size)
        return NULL;
    return v->data[index];
}

void* vector_peek(vector_t* v)
{
    if (!v) return NULL;
    return vector_get(v, v->size - 1);
}

// returns the index of the element if found, -1 otherwise
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

void vector_concat(vector_t* v, vector_t* u)
{
    for (unsigned i = 0; i < u->size; ++i)
        vector_add(v, vector_get(u, i));
}

vector_t* vector_add_if_new(vector_t* v, void* el, int (*c)(void*, void*))
{
    if (vector_contains(v, el, c) == -1)
        vector_add(v, el);
    return v;
}

void vector_merge(vector_t* v, vector_t* u, int (*c)(void*, void*))
{
    for (unsigned i = 0; i < u->size; ++i)
        vector_add_if_new(v, vector_get(u, i), c);
}
