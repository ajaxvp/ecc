#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ecc.h"

#define TOMBSTONE (void*) (-1)
#define SET_VALUE (void*) 0x4A67

map_t* map_init(int (*comparator)(void*, void*), unsigned long (*hash)(void*))
{
    map_t* m = calloc(1, sizeof *m);
    m->capacity = 50;
    m->key = calloc(m->capacity, sizeof(void*));
    m->value = calloc(m->capacity, sizeof(void*));
    m->size = 0;
    m->comparator = comparator;
    m->hash = hash;
    return m;
}

void map_set_printers(map_t* m,
    int (*key_printer)(void* key, int (*printer)(const char* fmt, ...)),
    int (*value_printer)(void* value, int (*printer)(const char* fmt, ...)))
{
    m->key_printer = key_printer;
    m->value_printer = value_printer;
}

void map_set_deleters(map_t* m, void (*key_deleter)(void* key), void (*value_deleter)(void* value))
{
    m->key_deleter = key_deleter;
    m->value_deleter = value_deleter;
}

void set_set_deleter(map_t* m, void (*deleter)(void*))
{
    map_set_deleters(m, deleter, NULL);
}

void map_resize(map_t* m)
{
    size_t new_capacity = m->capacity * 2;
    void** nkey = calloc(new_capacity, sizeof(void*));
    void** nvalue = calloc(new_capacity, sizeof(void*));
    MAP_FOR(void*, void*, m)
    {
        if (MAP_IS_BAD_KEY) continue;

        unsigned long h = m->hash(k) % new_capacity;

        for (size_t i = h;;)
        {
            void* fk = nkey[i];
            if (!fk || fk == TOMBSTONE)
            {
                nkey[i] = k;
                nvalue[i] = v;
                break;
            }

            i = (i + 1) % new_capacity;
            if (i == h)
                assert_fail;
        }
    }
    free(m->key);
    free(m->value);
    m->key = nkey;
    m->value = nvalue;
    m->capacity = new_capacity;
}

// gets the index of the key, if it exists in the map
static int map_get_key_index(map_t* m, void* key)
{
    unsigned long h = m->hash(key) % m->capacity;

    for (int i = h;;)
    {
        void* k = m->key[i];
        if (!k)
            return -1;
        if (k != TOMBSTONE && !m->comparator(k, key))
            return i;

        i = (i + 1) % m->capacity;
        if (i == h)
            return -1;
    }
}

void* map_add(map_t* m, void* key, void* value)
{
    int found = map_get_key_index(m, key);
    if (found != -1)
    {
        void* v = m->value[found];
        m->value[found] = value;
        return v;
    }

    if (m->size > m->capacity / 2)
        map_resize(m);
    
    unsigned long h = m->hash(key) % m->capacity;

    for (size_t i = h;;)
    {
        void* k = m->key[i];
        if (!k || k == TOMBSTONE)
        {
            m->key[i] = key;
            m->value[i] = value;
            ++(m->size);
            break;
        }

        i = (i + 1) % m->capacity;
        if (i == h)
            assert_fail;
    }

    return NULL;
}

void* map_remove(map_t* m, void* key)
{
    int i = map_get_key_index(m, key);
    if (i == -1)
        return NULL;
    void* k = m->key[i];
    void* v = m->value[i];
    if (m->key_deleter) m->key_deleter(k);
    if (m->value_deleter) m->value_deleter(v);
    m->key[i] = TOMBSTONE;
    m->value[i] = NULL;
    --(m->size);
    return v;
}

bool map_contains_key(map_t* m, void* key)
{
    return map_get_key_index(m, key) != -1;
}

void* map_get(map_t* m, void* key)
{
    int i = map_get_key_index(m, key);
    if (i == -1)
        return NULL;
    return m->value[i];
}

// returns the given value if the key is not in the map, returns the result of map_get(m, key) otherwise
void* map_get_or_add(map_t* m, void* key, void* value)
{
    int i = map_get_key_index(m, key);
    if (i == -1)
    {
        map_add(m, key, value);
        return value;
    }
    return m->value[i];
}

void map_delete(map_t* m)
{
    if (!m) return;
    if (m->key_deleter || m->value_deleter)
    {
        for (size_t i = 0; i < m->capacity; ++i)
        {
            void* k = m->key[i];
            void* v = m->value[i];
            if (!k || k == TOMBSTONE)
                continue;
            if (m->key_deleter) m->key_deleter(k);
            if (m->value_deleter) m->value_deleter(v);
        }
    }
    free(m->key);
    free(m->value);
    free(m);
}

void map_print(map_t* m, int (*printer)(const char*, ...))
{
    printer("[map] (%d)\n", m->size);
    for (size_t i = 0; i < m->capacity; ++i)
    {
        void* k = m->key[i];
        void* v = m->value[i];

        if (!k || k == TOMBSTONE)
            continue;
        
        printer("    ");

        if (m->key_printer)
            m->key_printer(k, printer);
        else
            printer("%p", k);
        
        printer(": ");

        if (m->value_printer)
            m->value_printer(v, printer);
        else
            printer("%p", v);
        
        printer("\n");
    }
}

map_t* set_init(int (*comparator)(void*, void*), unsigned long (*hash)(void*))
{
    return map_init(comparator, hash);
}

bool set_add(map_t* m, void* key)
{
    return map_add(m, key, (void*) SET_VALUE) == NULL;
}

bool set_remove(map_t* m, void* key)
{
    return map_remove(m, key) != NULL;
}

bool set_contains(map_t* m, void* key)
{
    return map_contains_key(m, key);
}

void set_delete(map_t* m)
{
    map_delete(m);
}

void set_print(map_t* m, int (*printer)(const char*, ...))
{
    map_print(m, printer);
}

void* set_get(map_t* m, void *key)
{
    int i = map_get_key_index(m, key);
    if (i == -1)
        return NULL;
    return m->key[i];
}
