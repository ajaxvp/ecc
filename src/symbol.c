#include <stdlib.h>
#include <string.h>

#include "cc.h"

// not mine
static unsigned long hash(char* str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

symbol_t* symbol_init(syntax_component_t* declaration, syntax_component_t* declarator, syntax_component_t* scope)
{
    symbol_t* sy = calloc(1, sizeof *sy);
    sy->declaration = declaration;
    sy->declarator = declarator;
    sy->offset = 0;
    sy->label = NULL;
    sy->scope = scope;
    sy->shaded = NULL;
    return sy;
}

void symbol_print(symbol_t* sy, int (*printer)(char*, ...))
{
    printer("symbol { ");
    printer("offset: %d, ", sy->offset);
    printer("label: \"%s\", ", sy->label);
    printer("shaded: %s ", BOOL_NAMES[sy->shaded != NULL]);
    printer("}\n");
}

// this impl optionally asks for a ptr to retrieve the index of the element as well
static symbol_t* symbol_table_get_internal(symbol_table_t* t, char* k, int* i)
{
    unsigned long index = hash(k) % t->capacity;
    for (;; index = (index + 1 == t->capacity ? 0 : index + 1))
    {
        if (t->key[index] != NULL && !strcmp(t->key[index], k))
        {
            if (i) *i = index;
            return t->value[index];
        }
    }
    if (i) *i = -1;
    return NULL;
}

symbol_table_t* symbol_table_init(void)
{
    symbol_table_t* t = calloc(1, sizeof *t);
    t->key = calloc(50, sizeof(char*));
    memset(t->key, 0, 50 * sizeof(char*));
    t->value = calloc(50, sizeof(symbol_t*));
    memset(t->value, 0, 50 * sizeof(symbol_t*));
    t->size = 0;
    t->capacity = 50;
    return t;
}

symbol_table_t* symbol_table_resize(symbol_table_t* t)
{
    unsigned old_capacity = t->capacity;
    t->key = realloc(t->key, (t->capacity = (unsigned) (t->capacity * 1.5)) * sizeof(char*));
    memset(t->key + old_capacity, 0, (t->capacity - old_capacity) * sizeof(char*));
    t->value = realloc(t->value, t->capacity * sizeof(symbol_t*));
    memset(t->value + old_capacity, 0, (t->capacity - old_capacity) * sizeof(char*));
    return t;
}

symbol_t* symbol_table_add(symbol_table_t* t, char* k, symbol_t* sy)
{
    if (t->size >= t->capacity)
        (void) symbol_table_resize(t);
    int exi;
    symbol_t* ex = symbol_table_get_internal(t, k, &exi);
    if (ex)
    {
        sy->shaded = ex;
        t->value[exi] = sy;
        return sy;
    }
    unsigned long index = hash(k) % t->capacity;
    for (unsigned long i = index;;)
    {
        if (t->key[i] == NULL)
        {
            t->key[i] = k;
            t->value[i] = sy;
            ++(t->size);
            break;
        }
        i = (i + 1) % t->capacity;
        if (i == index) // for safety but this should never happen
            break;
    }
    return sy;
}

symbol_t* symbol_table_get(symbol_table_t* t, char* k)
{
    return symbol_table_get_internal(t, k, NULL);
}

// removes a symbol from the table, replaces it with its shaded symbol if necessary
symbol_t* symbol_table_remove(symbol_table_t* t, char* k)
{
    unsigned long index = hash(k) % t->capacity;
    for (unsigned long i = index;;)
    {
        if (t->key[index] != NULL && !strcmp(t->key[index], k))
        {
            symbol_t* sy = t->value[index];
            if (t->value[index]->shaded)
                t->value[index] = t->value[index]->shaded;
            else
            {
                t->key[index] = NULL;
                t->value[index] = NULL;
                --(t->size);
            }
            return sy;
        }
        i = (i + 1) % t->capacity;
        if (i == index)
            break;
    }
    return NULL;
}

void symbol_table_print(symbol_table_t* t, int (*printer)(char*, ...))
{
    printer("symbol_table\n");
    for (unsigned i = 0; i < t->capacity; ++i)
    {
        char* k = t->key[i];
        if (k != NULL)
        {
            printer("    \"%s\" -> ", k);
            symbol_print(t->value[i], printer);
        }
    }
}

void symbol_table_delete(symbol_table_t* t)
{
    free(t->key);
    free(t->value);
    free(t);
}