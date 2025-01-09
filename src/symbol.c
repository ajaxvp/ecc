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

symbol_t* symbol_init(syntax_component_t* declarer)
{
    symbol_t* sy = calloc(1, sizeof *sy);
    sy->declarer = declarer;
    return sy;
}

/*

function scope: defined by a SC_FUNCTION_DEFINITION syntax element
file scope: defined by a SC_TRANSLATION_UNIT syntax element
block scope: defined by a SC_STATEMENT syntax element (depends on the circumstance but yeh)
function prototype scope: defined by a SC_FUNCTION_DECLARATOR syntax element (only if not part of a function definition)

object: declared by a SC_DECLARATOR
function: declared by a SC_DECLARATOR
tag of struct, union, or enum: declared by a SC_TYPE_SPECIFIER

*/

int f(int a, int (*b)(void));

// SC_STATEMENT = SC_LABELED_STATEMENT | SC_COMPOUND_STATEMENT | SC_EXPRESSION_STATEMENT | SC_SELECTION_STATEMENT | SC_ITERATION_STATEMENT | SC_JUMP_STATEMENT
// SC_SELECTION_STATEMENT = SC_IF_STATEMENT | SC_SWITCH_STATEMENT
// SC_ITERATION_STATEMENT = SC_DO_STATEMENT | SC_WHILE_STATEMENT | SC_FOR_STATEMENT
// SC_JUMP_STATEMENT = SC_GOTO_STATEMENT | SC_CONTINUE_STATEMENT | SC_BREAK_STATEMENT | SC_RETURN_STATEMENT
syntax_component_t* symbol_get_scope(symbol_t* sy)
{
    if (!sy) return NULL;
    // first, check if it's a label. if so, it's in function scope
    syntax_component_t* syn = sy->declarer->parent;
    if (!syn) return NULL;
    // ISO: 6.2.1 (3)
    if (syn->type == SC_LABELED_STATEMENT)
    {
        for (; syn && syn->type != SC_FUNCTION_DEFINITION; syn = syn->parent);
        return syn;
    }
    // if not a label, determine whether it is declared in a type specifier or a declarator.
    // if in a declarator, get outside of the full declarator before proceeding with the search
    // if in a type specifier, exit the struct, union, or enum before proceeding
    if (syntax_is_declarator_type(syn->type))
    {
        syn = syntax_get_full_declarator(syn = sy->declarer);
        if (!syn) return NULL;
        syn = syn->parent;
    }
    else if (syn->type == SC_ENUM_SPECIFIER ||
        syn->type == SC_STRUCT_UNION_SPECIFIER)
        syn = syn->parent;
    else
        return NULL;
    for (; syn; syn = syn->parent)
    {
        if (syn->type == SC_TRANSLATION_UNIT)
            return syn;
        // ISO: 6.8.2 (2), 6.8 (3)
        if (syn->type == SC_COMPOUND_STATEMENT)
            return syn;
        // ISO: 6.8.4 (3)
        if (syn->type == SC_IF_STATEMENT || syn->type == SC_SWITCH_STATEMENT)
            return syn;
        // ISO: 6.8.5 (5)
        if (syn->type == SC_DO_STATEMENT || syn->type == SC_WHILE_STATEMENT || syn->type == SC_FOR_STATEMENT)
            return syn;
        if (syn->type == SC_FUNCTION_DECLARATOR)
        {
            syntax_component_t* fdef = syntax_get_declarator_function_definition(syn);
            if (fdef)
                return fdef->fdef_body;
            else
                return syn;
        }
    }
    return NULL;
}

// whether or not the given symbol is in scope at syn
bool symbol_in_scope(symbol_t* sy, syntax_component_t* syn)
{
    syntax_component_t* scope = symbol_get_scope(sy);
    if (!scope) return false;
    for (; syn && syn != scope; syn = syn->parent);
    return syn == scope;
}

void symbol_print(symbol_t* sy, int (*printer)(const char*, ...))
{
    if (!sy)
    {
        printer("null");
        return;
    }
    printer("symbol { ");
    printer(" }");
}

void symbol_delete(symbol_t* sy)
{
    free(sy);
}

static void symbol_delete_list(symbol_t* sy)
{
    if (!sy)
        return;
    symbol_delete_list(sy->next);
    symbol_delete(sy);
}

// this impl optionally asks for a ptr to retrieve the index of the element as well
static symbol_t* symbol_table_get_internal(symbol_table_t* t, char* k, int* i)
{
    unsigned long index = hash(k) % t->capacity;
    unsigned long oidx = index;
    do
    {
        if (t->key[index] != NULL && !strcmp(t->key[index], k))
        {
            if (i) *i = index;
            return t->value[index];
        }
        index = (index + 1 == t->capacity ? 0 : index + 1);
    }
    while (index != oidx);
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
    if (ex) // append to the end
    {
        symbol_t* last = ex;
        for (; last->next; last = last->next);
        last->next = sy;
        return sy;
    }
    unsigned long index = hash(k) % t->capacity;
    for (unsigned long i = index;;)
    {
        if (t->key[i] == NULL)
        {
            t->key[i] = strdup(k);
            t->value[i] = sy;
            ++(t->size);
            break;
        }
        i = (i + 1) % t->capacity;
        if (i == index) // for safety but this should never happen
            return NULL;
    }
    return sy;
}

symbol_t* symbol_table_get_all(symbol_table_t* t, char* k)
{
    return symbol_table_get_internal(t, k, NULL);
}

// uses an exact identifier object as opposed to just a name
symbol_t* symbol_table_get_syn_id(symbol_table_t* t, syntax_component_t* id)
{
    symbol_t* sylist = symbol_table_get_all(t, id->id);
    for (; sylist && sylist->declarer != id; sylist = sylist->next);
    return sylist;
}

// given a contextualized id (syntax element id), find the symbol, NULL if cannot find
symbol_t* symbol_table_lookup(symbol_table_t* t, syntax_component_t* id)
{
    symbol_t* sylist = symbol_table_get_all(t, id->id);
    for (; sylist; sylist = sylist->next)
    {
        if (symbol_in_scope(sylist, id))
            return sylist;
    }
    return NULL;
}

// removes a symbol from the table, replaces it with its shaded symbol if necessary
// symbol_t* symbol_table_remove(symbol_table_t* t, char* k)
// {
//     unsigned long index = hash(k) % t->capacity;
//     for (unsigned long i = index;;)
//     {
//         if (t->key[index] != NULL && !strcmp(t->key[index], k))
//         {
//             symbol_t* sy = t->value[index];
//             t->key[index] = NULL;
//             t->value[index] = NULL;
//             --(t->size);
//             return sy;
//         }
//         i = (i + 1) % t->capacity;
//         if (i == index)
//             break;
//     }
//     return NULL;
// }

void symbol_table_print(symbol_table_t* t, int (*printer)(const char*, ...))
{
    printer("[symbol table]\n");
    for (unsigned i = 0; i < t->capacity; ++i)
    {
        char* k = t->key[i];
        if (k != NULL)
        {
            printer("  \"%s\" -> [", k);
            for (symbol_t* sy = t->value[i]; sy; sy = sy->next)
            {
                if (sy != t->value[i])
                    printer(", ");
                symbol_print(t->value[i], printer);
            }
            printer("]\n");
        }
    }
}

void symbol_table_delete(symbol_table_t* t, bool free_contents)
{
    for (unsigned i = 0; i < t->capacity; ++i)
    {
        if (free_contents && t->key[i])
            symbol_delete_list(t->value[i]);
        free(t->key[i]);
    }
    free(t->key);
    free(t->value);
    free(t);
}