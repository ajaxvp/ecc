#include <stdlib.h>
#include <string.h>

#include "ecc.h"

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

static syntax_component_t* syntax_get_scope(syntax_component_t* syn)
{
    if (!syn) return NULL;
    syntax_component_t* orig = syn;
    syn = syn->parent;
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
    if (syntax_is_declarator_type(orig->type) || syntax_is_declarator_type(syn->type))
    {
        syn = syntax_get_full_declarator(syn = orig);
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

syntax_component_t* symbol_get_scope(symbol_t* sy)
{
    if (!sy) return NULL;
    return syntax_get_scope(sy->declarer);
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
    if (sy->declarer->type == SC_STRING_LITERAL)
        return SD_STATIC;
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

int symbol_get_scope_distance(syntax_component_t* inner, syntax_component_t* outer)
{
    int i = 0;
    for (; inner && inner != outer; inner = inner->parent);
    return inner == outer ? i : -1;
}

static char* syntax_get_identifier_name(syntax_component_t* syn)
{
    if (!syn) return NULL;
    switch (syn->type)
    {
        case SC_IDENTIFIER:
        case SC_PRIMARY_EXPRESSION_IDENTIFIER:
        case SC_ENUMERATION_CONSTANT:
        case SC_DECLARATOR_IDENTIFIER:
        case SC_TYPEDEF_NAME:
            return syn->id;
        case SC_COMPOUND_LITERAL:
            return syn->cl_id;
        case SC_STRING_LITERAL:
            return syn->strl_id;
        default:
            return NULL;
    }
}

char* symbol_get_name(symbol_t* sy)
{
    if (!sy) return NULL;
    if (!sy->declarer) return "__anonymous_lv__";
    return syntax_get_identifier_name(sy->declarer);
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
    printer(", namespace: ");
    if (sy->ns)
        namespace_print(sy->ns, printer);
    else
        printer("(none)");
    if (sy->designations)
    {
        printer(", initializations: [");
        for (size_t i = 0; i < sy->designations->size; ++i)
        {
            if (i != 0)
                printer(", ");
            designation_t* desig = vector_get(sy->designations, i);
            constexpr_t* ce = vector_get(sy->initial_values, i);
            printer("%s", symbol_get_name(sy));
            if (desig)
                designation_print(desig, printer);
            printer(" = ");
            constexpr_print(ce, printer);
        }
        printer("]");
    }
    printer(" }");
}

void symbol_delete(symbol_t* sy)
{
    if (!sy) return;
    type_delete(sy->type);
    namespace_delete(sy->ns);
    if (sy->designations)
        vector_deep_delete(sy->designations, (void (*)(void*)) designation_delete_all);
    if (sy->initial_values)
        vector_deep_delete(sy->initial_values, (void (*)(void*)) constexpr_delete);
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

symbol_t* symbol_init_initializer(symbol_t* sy)
{
    if (!sy->designations) sy->designations = vector_init();
    if (!sy->initial_values) sy->initial_values = vector_init();
    return sy;
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
    t->unique_types = vector_init();
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
    symbol_t* sylist = symbol_table_get_all(t, syntax_get_identifier_name(id));
    for (; sylist && sylist->declarer != id; sylist = sylist->next);
    return sylist;
}

// given a contextualized id (syntax element id), find the symbol, NULL if cannot find
// functions just as symbol_table_get_syn_id for declaring identifiers
// also takes a namespace 'ns' as an argument for searching only in a given namespace, NULL will ignore namespaces entirely
symbol_t* symbol_table_lookup(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns)
{
    if (!id) return NULL;
    symbol_t* sylist = symbol_table_get_all(t, syntax_get_identifier_name(id));
    symbol_t* found = NULL;
    int distance = INT_MAX;
    for (; sylist; sylist = sylist->next)
    {
        if (sylist->declarer == id)
            return sylist;
        c_namespace_t* sns = sylist->ns;
        if (symbol_in_scope(sylist, id) && (!ns || namespace_equals(ns, sns)))
        {
            int d = symbol_get_scope_distance(syntax_get_scope(id), symbol_get_scope(sylist));
            if (d < distance)
            {
                distance = d;
                found = sylist;
            }
        }
    }
    return found;
}

// 3 return values here:
//  - actual return value: the symbol representing the identifier given as an arg, if any (must be declaring)
//  - count: the number of duplicate entries in the symbol table (i.e., same namespace and scope)
//  - first: whether the symbol representing a declaring id was the first duplicate
symbol_t* symbol_table_count(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns, size_t* count, bool* first)
{
    if (first) *first = false;
    symbol_t* sylist = symbol_table_get_all(t, syntax_get_identifier_name(id));
    if (count) *count = 0;
    symbol_t* found = NULL;
    int distance = INT_MAX;
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
        c_namespace_t* sns = sylist->ns;
        if (symbol_in_scope(sylist, id) && (!ns || namespace_equals(ns, sns)))
        {
            syntax_component_t* slscope = symbol_get_scope(sylist);
            syntax_component_t* idscope = syntax_get_scope(id);
            int d = symbol_get_scope_distance(idscope, slscope);
            if (d < distance)
            {
                distance = d;
                found = sylist;
            }
            if (count && slscope == idscope) ++(*count);
        }
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
    vector_deep_delete(t->unique_types, (void (*)(void*)) symbol_type_delete);
    free(t->key);
    free(t->value);
    free(t);
}