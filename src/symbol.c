#include <stdlib.h>
#include <string.h>

#include "cc.h"

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

// SC_STATEMENT = SC_LABELED_STATEMENT | SC_COMPOUND_STATEMENT | SC_EXPRESSION_STATEMENT | SC_SELECTION_STATEMENT | SC_ITERATION_STATEMENT | SC_JUMP_STATEMENT
// SC_SELECTION_STATEMENT = SC_IF_STATEMENT | SC_SWITCH_STATEMENT
// SC_ITERATION_STATEMENT = SC_DO_STATEMENT | SC_WHILE_STATEMENT | SC_FOR_STATEMENT
// SC_JUMP_STATEMENT = SC_GOTO_STATEMENT | SC_CONTINUE_STATEMENT | SC_BREAK_STATEMENT | SC_RETURN_STATEMENT
// SC_TYPE_SPECIFIER = SC_BASIC_TYPE_SPECIFIER | SC_STRUCT_UNION_SPECIFIER | SC_ENUM_SPECIFIER | SC_TYPEDEF_NAME
syntax_component_t* symbol_get_scope(symbol_t* sy)
{
    if (!sy) return NULL;
    // first, check if it's in a label. if so, it's in function scope
    syntax_component_t* syn = sy->declarer->parent;
    if (!syn) return NULL;
    // ISO: 6.2.1 (3)
    if (syn->type == SC_LABELED_STATEMENT)
    {
        for (; syn && syn->type != SC_FUNCTION_DEFINITION; syn = syn->parent);
        return syn;
    }
    // if not a label, determine whether it is declared in a type specifier or a declarator.
    // if in a declarator (or a declarator itself), get outside of the full declarator before proceeding with the search
    // if in a type specifier, exit the struct, union, or enum before proceeding
    if (syntax_is_declarator_type(sy->declarer->type) || syntax_is_declarator_type(syn->type))
    {
        syn = syntax_get_full_declarator(syn = sy->declarer);
        if (!syn) return NULL;
        syn = syn->parent;
    }
    else if (syn->type == SC_ENUM_SPECIFIER ||
        syn->type == SC_STRUCT_UNION_SPECIFIER)
        syn = syn->parent;
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

bool scope_is_block(syntax_component_t* scope)
{
    if (!scope) return false;
    return scope->type == SC_COMPOUND_STATEMENT ||
            scope->type == SC_IF_STATEMENT ||
            scope->type == SC_WHILE_STATEMENT ||
            scope->type == SC_FOR_STATEMENT ||
            scope->type == SC_DO_STATEMENT ||
            scope->type == SC_SWITCH_STATEMENT;
}

bool scope_is_file(syntax_component_t* scope)
{
    return scope && scope->type == SC_TRANSLATION_UNIT;
}

storage_duration_t symbol_get_storage_duration(symbol_t* sy)
{
    syntax_component_t* scope = symbol_get_scope(sy);
    if (!scope) return SD_UNKNOWN;
    if (scope_is_block(scope))
    {
        vector_t* declspecs = syntax_get_declspecs(sy->declarer);
        if (!declspecs) return SD_UNKNOWN;
        if (!syntax_has_specifier(declspecs, SC_STORAGE_CLASS_SPECIFIER, SCS_STATIC))
            return SD_AUTOMATIC;
    }
    return SD_STATIC;
}

linkage_t symbol_get_linkage(symbol_t* sy)
{
    vector_t* declspecs = syntax_get_declspecs(sy->declarer);
    syntax_component_t* scope = symbol_get_scope(sy);
    if (syntax_has_specifier(declspecs, SC_STORAGE_CLASS_SPECIFIER, SCS_EXTERN))
        return LK_EXTERNAL;
    if (syntax_has_specifier(declspecs, SC_STORAGE_CLASS_SPECIFIER, SCS_STATIC))
        return scope_is_block(scope) ? LK_NONE : LK_INTERNAL;
    return scope_is_file(scope) ? LK_EXTERNAL : LK_NONE;
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
    printer("symbol { type: ");
    if (sy->type)
        type_humanized_print(sy->type, printer);
    else
        printer("(untyped)");
    printer(", scope: ");
    syntax_component_t* scope = symbol_get_scope(sy);
    if (scope)
    {
        switch (scope->type)
        {
            case SC_TRANSLATION_UNIT:
                printer("file");
                break;
            case SC_FUNCTION_DEFINITION:
                printer("function");
                break;
            case SC_COMPOUND_STATEMENT:
            case SC_IF_STATEMENT:
            case SC_WHILE_STATEMENT:
            case SC_FOR_STATEMENT:
            case SC_DO_STATEMENT:
            case SC_SWITCH_STATEMENT:
                printer("block");
                break;
            case SC_FUNCTION_DECLARATOR:
                printer("function prototype");
                break;
            default:
                printer("(bad)");
                break;
        }
    }
    else
        printer("(unknown)");
    printer(", locator: ");
    if (sy->loc)
        locator_print(sy->loc, printer);
    else
        printer("(unlocated)");
    printer(" }");
}

void symbol_delete(symbol_t* sy)
{
    if (!sy) return;
    type_delete(sy->type);
    locator_delete(sy->loc);
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
// functions just as symbol_table_get_syn_id for declaring identifiers
// also takes a namespace 'ns' as an argument for searching only in a given namespace, NULL will ignore namespaces entirely
symbol_t* symbol_table_lookup(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns)
{
    symbol_t* sylist = symbol_table_get_all(t, id->id);
    for (; sylist; sylist = sylist->next)
    {
        if (sylist->declarer == id)
            return sylist;
        c_namespace_t* sns = syntax_get_namespace(sylist->declarer);
        if (symbol_in_scope(sylist, id) && (!ns || (ns->class == sns->class && type_is_compatible(ns->struct_union_type, sns->struct_union_type))))
        {
            free(sns);
            return sylist;
        }
        free(sns);
    }
    return NULL;
}

// 3 return values here:
//  - actual return value: the symbol representing the identifier given as an arg, if any (must be declaring)
//  - count: the number of duplicate entries in the symbol table (i.e., same namespace and scope)
//  - first: whether the symbol representing a declaring id was the first duplicate
symbol_t* symbol_table_count(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns, size_t* count, bool* first)
{
    if (first) *first = false;
    symbol_t* sylist = symbol_table_get_all(t, id->id);
    if (count) *count = 0;
    symbol_t* found = NULL;
    for (; sylist; sylist = sylist->next)
    {
        if (sylist->declarer == id)
        {
            if (count && first && *count == 0)
                *first = true;
            if (count)
                ++(*count);
            found = sylist;
            continue;
        }
        c_namespace_t* sns = syntax_get_namespace(sylist->declarer);
        if (count && symbol_in_scope(sylist, id) && (!ns || (ns->class == sns->class && type_is_compatible(ns->struct_union_type, sns->struct_union_type))))
        {
            ++(*count);
            found = sylist;
        }
        free(sns);
    }
    return found;
}

// removes a symbol from the table
symbol_t* symbol_table_remove(symbol_table_t* t, syntax_component_t* id)
{
    char* k = id->id;
    unsigned long index = hash(k) % t->capacity;
    for (unsigned long i = index;;)
    {
        if (t->key[i] != NULL && !strcmp(t->key[i], k))
        {
            symbol_t* prev = NULL;
            symbol_t* sy = t->value[i];
            symbol_t* sylist = sy;
            for (; sylist; prev = sylist, sylist = sylist->next)
            {
                if (sylist->declarer == id)
                {
                    if (prev)
                        prev->next = sylist->next;
                    else
                        sy = sylist->next;
                    break;
                }
            }
            if (!sylist)
                return NULL;
            if (!sy)
            {
                t->key[i] = NULL;
                --(t->size);
            }
            t->value[i] = sy;
            return sylist;
        }
        i = (i + 1) % t->capacity;
        if (i == index)
            break;
    }
    return NULL;
}

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
                symbol_print(sy, printer);
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