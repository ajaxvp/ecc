// this file contains some utility functions for working with the behemoth that is syntax_component_t.

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cc.h"

bool syntax_has_specifier(vector_t* specifiers, syntax_component_type_t sc_type, int type)
{
    for (unsigned i = 0; i < specifiers->size; ++i)
    {
        syntax_component_t* specifier = vector_get(specifiers, i);
        if (specifier->type != sc_type) continue;
        switch (sc_type)
        {
            case SC_BASIC_TYPE_SPECIFIER:
                if (type == specifier->bts)
                    return true;
                break;
            case SC_FUNCTION_SPECIFIER:
                if (type == specifier->fs)
                    return true;
                break;
            case SC_TYPE_QUALIFIER:
                if (type == specifier->tq)
                    return true;
                break;
            case SC_STORAGE_CLASS_SPECIFIER:
                if (type == specifier->scs)
                    return true;
                break;
            default:
                return false;
        }
    }
    return false;
}

// ps - print structure
// pf - print field
#define ps(fmt, ...) { for (unsigned i = 0; i < indent; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define pf(fmt, ...) { for (unsigned i = 0; i < indent + 1; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define next_indent (indent + 2)
#define sty(comp) "[" comp "]"

static void print_syntax_indented(syntax_component_t* s, unsigned indent, int (*printer)(const char* fmt, ...));

static void print_vector_indented(vector_t* v, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!v)
    {
        ps("null\n");
        return;
    }
    if (!v->size)
    {
        ps("[\n\n");
        ps("]\n");
        return;
    }
    ps("[\n");
    for (unsigned i = 0; i < v->size; ++i)
    {
        syntax_component_t* s = vector_get(v, i);
        print_syntax_indented(s, indent + 1, printer);
    }
    ps("]\n");
}

static void print_syntax_indented(syntax_component_t* s, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!s)
    {
        ps("null\n");
        return;
    }
    switch (s->type)
    {
        case SC_IDENTIFIER:
        {
            ps("identifier: %s\n", s->id);
            break;
        }

        case SC_STORAGE_CLASS_SPECIFIER:
        {
            ps("storage class specifier: %s\n", STORAGE_CLASS_NAMES[s->scs]);
            break;
        }

        case SC_BASIC_TYPE_SPECIFIER:
        {
            ps("type specifier (basic): %s\n", BASIC_TYPE_SPECIFIER_NAMES[s->bts]);
            break;
        }

        case SC_TYPE_QUALIFIER:
        {
            ps("type qualifier: %s\n", TYPE_QUALIFIER_NAMES[s->tq]);
            break;
        }

        case SC_FUNCTION_SPECIFIER:
        {
            ps("function specifier: %s\n", FUNCTION_SPECIFIER_NAMES[s->fs]);
            break;
        }

        case SC_POINTER:
        {
            ps(sty("pointer") "\n");
            pf("type qualifiers:\n");
            print_vector_indented(s->ptr_type_qualifiers, next_indent, printer);
            break;
        }

        case SC_DECLARATOR:
        {
            ps(sty("declarator") "\n");
            pf("pointers:\n");
            print_vector_indented(s->declr_pointers, next_indent, printer);
            pf("direct declarator:\n");
            print_syntax_indented(s->declr_direct, next_indent, printer);
            break;
        }

        case SC_INIT_DECLARATOR:
        {
            ps(sty("init declarator") "\n");
            pf("declarator:\n");
            print_syntax_indented(s->ideclr_declarator, next_indent, printer);
            pf("initializer:\n");
            print_syntax_indented(s->ideclr_initializer, next_indent, printer);
            break;
        }

        case SC_DECLARATION:
        {
            ps(sty("declaration") "\n");
            pf("declaration specifiers:\n");
            print_vector_indented(s->decl_declaration_specifiers, next_indent, printer);
            pf("init declarators:\n");
            print_vector_indented(s->decl_init_declarators, next_indent, printer);
            break;
        }

        case SC_ERROR:
        {
            ps(sty("error") "\n");
            pf("message: %s\n", s->err_message);
            pf("depth: %d\n", s->err_depth);
            break;
        }

        case SC_TRANSLATION_UNIT:
        {
            ps(sty("translation unit") "\n");
            pf("external declarations:\n");
            print_vector_indented(s->tlu_external_declarations, next_indent, printer);
            break;
        }

        default:
            ps(sty("unknown/unprintable syntax") "\n");
            return;
    }
}

#undef ps
#undef pf
#undef next_indent
#undef sty

void print_syntax(syntax_component_t* s, int (*printer)(const char* fmt, ...))
{
    print_syntax_indented(s, 0, printer);
}

#define deep_free_syntax_vector(vec, var) if (vec) { VECTOR_FOR(syntax_component_t*, var, (vec)) free_syntax(var); vector_delete((vec)); }

// free a syntax component and EVERYTHING IT HAS UNDERNEATH IT.
void free_syntax(syntax_component_t* syn)
{
    if (!syn)
        return;
    switch (syn->type)
    {
        case SC_ERROR:
        {
            free(syn->err_message);
            break;
        }
        case SC_TRANSLATION_UNIT:
        {
            deep_free_syntax_vector(syn->tlu_external_declarations, s1);
            deep_free_syntax_vector(syn->tlu_errors, s2);
            break;
        }
        case SC_IDENTIFIER:
        {
            free(syn->id);
            break;
        }
        case SC_POINTER:
        {
            deep_free_syntax_vector(syn->ptr_type_qualifiers, s);
            break;
        }
        case SC_DECLARATOR:
        {
            deep_free_syntax_vector(syn->declr_pointers, s);
            free_syntax(syn->declr_direct);
            break;
        }
        case SC_INIT_DECLARATOR:
        {
            free_syntax(syn->ideclr_declarator);
            free_syntax(syn->ideclr_initializer);
            break;
        }
        case SC_DECLARATION:
        {
            deep_free_syntax_vector(syn->decl_declaration_specifiers, s1);
            deep_free_syntax_vector(syn->decl_init_declarators, s2);
            break;
        }
        default:
            break;
    }
    free(syn);
}