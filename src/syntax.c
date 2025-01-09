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

// SC_STATEMENT = SC_LABELED_STATEMENT | SC_COMPOUND_STATEMENT | SC_EXPRESSION_STATEMENT | SC_SELECTION_STATEMENT | SC_ITERATION_STATEMENT | SC_JUMP_STATEMENT
// SC_SELECTION_STATEMENT = SC_IF_STATEMENT | SC_SWITCH_STATEMENT
// SC_ITERATION_STATEMENT = SC_DO_STATEMENT | SC_WHILE_STATEMENT | SC_FOR_STATEMENT
// SC_JUMP_STATEMENT = SC_GOTO_STATEMENT | SC_CONTINUE_STATEMENT | SC_BREAK_STATEMENT | SC_RETURN_STATEMENT
bool syntax_is_statement_type(syntax_component_type_t type)
{
    return type == SC_LABELED_STATEMENT ||
        type == SC_COMPOUND_STATEMENT ||
        type == SC_EXPRESSION_STATEMENT  ||
        type == SC_IF_STATEMENT ||
        type == SC_SWITCH_STATEMENT ||
        type == SC_DO_STATEMENT ||
        type == SC_WHILE_STATEMENT ||
        type == SC_FOR_STATEMENT ||
        type == SC_GOTO_STATEMENT ||
        type == SC_CONTINUE_STATEMENT ||
        type == SC_BREAK_STATEMENT ||
        type == SC_RETURN_STATEMENT;
}

bool syntax_is_concrete_declarator_type(syntax_component_type_t type)
{
    return type == SC_IDENTIFIER ||
        type == SC_DECLARATOR ||
        type == SC_ARRAY_DECLARATOR ||
        type == SC_FUNCTION_DECLARATOR ||
        type == SC_INIT_DECLARATOR ||
        type == SC_STRUCT_DECLARATOR;
}

bool syntax_is_abstract_declarator_type(syntax_component_type_t type)
{
    return type == SC_ABSTRACT_ARRAY_DECLARATOR ||
        type == SC_ABSTRACT_DECLARATOR ||
        type == SC_ABSTRACT_FUNCTION_DECLARATOR;
}

bool syntax_is_declarator_type(syntax_component_type_t type)
{
    return syntax_is_abstract_declarator_type(type) ||
        syntax_is_concrete_declarator_type(type);
}

bool syntax_is_declaration_type(syntax_component_type_t type)
{
    return type == SC_DECLARATION ||
        type == SC_STRUCT_DECLARATION ||
        type == SC_PARAMETER_DECLARATION;
}

// SC_DIRECT_DECLARATOR = SC_IDENTIFIER | SC_DECLARATOR | SC_ARRAY_DECLARATOR | SC_FUNCTION_DECLARATOR

// supports the following declarator types: SC_INIT_DECLARATOR, SC_STRUCT_DECLARATOR, SC_DECLARATOR, SC_DIRECT_DECLARATOR
// no abstract declarators b/c those don't have identifiers silly!!!
syntax_component_t* syntax_get_declarator_identifier(syntax_component_t* declr)
{
    if (!declr)
        return NULL;
    if (declr->type == SC_INIT_DECLARATOR)
        return syntax_get_declarator_identifier(declr->ideclr_declarator);
    if (declr->type == SC_STRUCT_DECLARATOR)
        return syntax_get_declarator_identifier(declr->sdeclr_declarator);
    if (declr->type == SC_DECLARATOR)
        return syntax_get_declarator_identifier(declr->declr_direct);
    if (declr->type == SC_ARRAY_DECLARATOR)
        return syntax_get_declarator_identifier(declr->adeclr_direct);
    if (declr->type == SC_FUNCTION_DECLARATOR)
        return syntax_get_declarator_identifier(declr->fdeclr_direct);
    if (declr->type == SC_IDENTIFIER)
        return declr;
    return NULL;
}

syntax_component_t* syntax_get_declarator_function_definition(syntax_component_t* declr)
{
    for (; declr && syntax_is_concrete_declarator_type(declr->type); declr = declr->parent);
    if (!declr)
        return NULL;
    return declr->type == SC_FUNCTION_DEFINITION ? declr : NULL;
}

syntax_component_t* syntax_get_declarator_declaration(syntax_component_t* declr)
{
    for (; declr && syntax_is_concrete_declarator_type(declr->type); declr = declr->parent);
    if (!declr)
        return NULL;
    return syntax_is_declaration_type(declr->type) ? declr : NULL;
}

// a full declarator is a declarator that is not a part of another declarator
// so for example, '*a' has two declarators: one that is an identifier and one that has the ptr
// the one with the ptr is the full declarator
// as another, '(*)(void)' has two declarators: one that is an abstract declarator with a ptr and the abs. function declarator
// the abs. function declarator is the full declarator
// will return itself if it is the full declarator
syntax_component_t* syntax_get_full_declarator(syntax_component_t* declr)
{
    if (!declr)
        return NULL;
    for (; declr->parent && syntax_is_declarator_type(declr->parent->type); declr = declr->parent);
    return declr;
}

// ps - print structure
// pf - print field
#define ps(fmt, ...) { for (unsigned i = 0; i < indent; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define pf(fmt, ...) { for (unsigned i = 0; i < indent + 1; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define next_indent (indent + 2)
#define sty(x) "[" x "]"

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

    // SC_ENUM_SPECIFIER,
    // SC_ENUMERATOR,
    // SC_ARRAY_DECLARATOR,
    // SC_FUNCTION_DECLARATOR,
    // SC_PARAMETER_DECLARATION,
    // SC_ABSTRACT_DECLARATOR,
    // SC_ABSTRACT_ARRAY_DECLARATOR,
    // SC_ABSTRACT_FUNCTION_DECLARATOR,
    // SC_LABELED_STATEMENT,
    // SC_COMPOUND_STATEMENT,
    // SC_EXPRESSION_STATEMENT,
    // SC_IF_STATEMENT,
    // SC_SWITCH_STATEMENT,
    // SC_DO_STATEMENT,
    // SC_WHILE_STATEMENT,
    // SC_FOR_STATEMENT,
    // SC_GOTO_STATEMENT,
    // SC_CONTINUE_STATEMENT,
    // SC_BREAK_STATEMENT,
    // SC_RETURN_STATEMENT,
    // SC_INITIALIZER_LIST,
    // SC_DESIGNATION,
    // SC_EXPRESSION,
    // SC_ASSIGNMENT_EXPRESSION,
    // SC_LOGICAL_OR_EXPRESSION,
    // SC_LOGICAL_AND_EXPRESSION,
    // SC_BITWISE_OR_EXPRESSION,
    // SC_BITWISE_XOR_EXPRESSION,
    // SC_BITWISE_AND_EXPRESSION,
    // SC_EQUALITY_EXPRESSION,
    // SC_INEQUALITY_EXPRESSION,
    // SC_GREATER_EQUAL_EXPRESSION,
    // SC_LESS_EQUAL_EXPRESSION,
    // SC_GREATER_EXPRESSION,
    // SC_LESS_EXPRESSION,
    // SC_BITWISE_RIGHT_EXPRESSION,
    // SC_BITWISE_LEFT_EXPRESSION,
    // SC_SUBTRACTION_EXPRESSION,
    // SC_ADDITION_EXPRESSION,
    // SC_MODULAR_EXPRESSION,
    // SC_DIVISION_EXPRESSION,
    // SC_MULTIPLICATION_EXPRESSION,
    // SC_CONDITIONAL_EXPRESSION,
    // SC_CAST_EXPRESSION,
    // SC_PREFIX_INCREMENT_EXPRESSION,
    // SC_PREFIX_DECREMENT_EXPRESSION,
    // SC_REFERENCE_EXPRESSION,
    // SC_DEREFERENCE_EXPRESSION,
    // SC_PLUS_EXPRESSION,
    // SC_MINUS_EXPRESSION,
    // SC_COMPLEMENT_EXPRESSION,
    // SC_NOT_EXPRESSION,
    // SC_SIZEOF_EXPRESSION,
    // SC_SIZEOF_TYPE_EXPRESSION,
    // SC_INITIALIZER_LIST_EXPRESSION,
    // SC_POSTFIX_INCREMENT_EXPRESSION,
    // SC_POSTFIX_DECREMENT_EXPRESSION,
    // SC_DEREFERENCE_MEMBER_EXPRESSION,
    // SC_MEMBER_EXPRESSION,
    // SC_FUNCTION_CALL_EXPRESSION,
    // SC_SUBSCRIPT_EXPRESSION,
    // SC_TYPE_NAME,
    // SC_TYPEDEF_NAME,
    // SC_ENUMERATION_CONSTANT,
    // SC_FLOATING_CONSTANT,
    // SC_INTEGER_CONSTANT,
    // SC_CHARACTER_CONSTANT,
    // SC_STRING_LITERAL

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

        case SC_TYPEDEF_NAME:
        {
            ps("typedef name: %s\n", s->id);
            break;
        }

        case SC_ENUMERATION_CONSTANT:
        {
            ps("enumeration constant: %s\n", s->id);
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

        case SC_FUNCTION_DEFINITION:
        {
            ps(sty("function definition") "\n");
            pf("declaration specifiers:\n");
            print_vector_indented(s->fdef_declaration_specifiers, next_indent, printer);
            pf("declarator:\n");
            print_syntax_indented(s->fdef_declarator, next_indent, printer);
            pf("K&R declarations:\n");
            print_vector_indented(s->fdef_knr_declarations, next_indent, printer);
            pf("body:\n");
            print_syntax_indented(s->fdef_body, next_indent, printer);
            break;
        }

        case SC_STRUCT_UNION_SPECIFIER:
        {
            if (s->sus_sou == SOU_STRUCT)
            {
                ps(sty("struct specifier") "\n");
            }
            else if (s->sus_sou == SOU_UNION)
            {
                ps(sty("union specifier") "\n");
            }
            pf("identifier:\n");
            print_syntax_indented(s->sus_id, next_indent, printer);
            pf("declarations:\n");
            print_vector_indented(s->sus_declarations, next_indent, printer);
            break;
        }

        case SC_ARRAY_DECLARATOR:
        {
            ps(sty("array declarator") "\n");
            pf("direct declarator:\n");
            print_syntax_indented(s->adeclr_direct, next_indent, printer);
            pf("type qualifiers:\n");
            print_vector_indented(s->adeclr_type_qualifiers, next_indent, printer);
            pf("static: %s\n", BOOL_NAMES[s->adeclr_is_static]);
            pf("length expression:\n");
            print_syntax_indented(s->adeclr_length_expression, next_indent, printer);
            pf("unspecified size: %s\n", BOOL_NAMES[s->adeclr_unspecified_size]);
            break;
        }

        case SC_FUNCTION_DECLARATOR:
        {
            ps(sty("function declarator") "\n");
            pf("direct declarator:\n");
            print_syntax_indented(s->fdeclr_direct, next_indent, printer);
            pf("parameter declarations:\n");
            print_vector_indented(s->fdeclr_parameter_declarations, next_indent, printer);
            pf("K&R identifiers:\n");
            print_vector_indented(s->fdeclr_knr_identifiers, next_indent, printer);
            pf("ellipsis: %s\n", BOOL_NAMES[s->fdeclr_ellipsis]);
            break;
        }

        case SC_PARAMETER_DECLARATION:
        {
            ps(sty("parameter declaration") "\n");
            pf("declaration specifiers:\n");
            print_vector_indented(s->pdecl_declaration_specifiers, next_indent, printer);
            pf("declarator:\n");
            print_syntax_indented(s->pdecl_declr, next_indent, printer);
            break;
        }

        default:
            ps(sty("unknown/unprintable syntax") "\n");
            pf("type name: %s\n", SYNTAX_COMPONENT_NAMES[s->type]);
            return;
    }
    if (debug)
    {
        if (s->parent)
        {
            pf("parent type (for confirmation): %s\n", SYNTAX_COMPONENT_NAMES[s->parent->type]);
        }
        else
        {
            pf("parent type (for confirmation): no parent\n");
        }
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
            symbol_table_delete(syn->tlu_st, true);
            break;
        }
        case SC_IDENTIFIER:
        case SC_TYPEDEF_NAME:
        case SC_ENUMERATION_CONSTANT:
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
        case SC_ARRAY_DECLARATOR:
        {
            free_syntax(syn->adeclr_direct);
            deep_free_syntax_vector(syn->adeclr_type_qualifiers, s);
            free_syntax(syn->adeclr_length_expression);
            break;
        }
        case SC_FUNCTION_DECLARATOR:
        {
            free_syntax(syn->fdeclr_direct);
            deep_free_syntax_vector(syn->fdeclr_knr_identifiers, s1);
            deep_free_syntax_vector(syn->fdeclr_parameter_declarations, s2);
            break;
        }
        case SC_PARAMETER_DECLARATION:
        {
            free_syntax(syn->pdecl_declr);
            deep_free_syntax_vector(syn->pdecl_declaration_specifiers, s);
            break;
        }
        case SC_STRUCT_UNION_SPECIFIER:
        {
            deep_free_syntax_vector(syn->sus_declarations, s);
            free_syntax(syn->sus_id);
            break;
        }
        case SC_STRUCT_DECLARATION:
        {
            deep_free_syntax_vector(syn->sdecl_declarators, s1);
            deep_free_syntax_vector(syn->sdecl_specifier_qualifier_list, s2);
            break;
        }
        case SC_STRUCT_DECLARATOR:
        {
            free_syntax(syn->sdeclr_declarator);
            free_syntax(syn->sdeclr_bits_expression);
            break;
        }
        // nothing to free
        case SC_UNKNOWN:
        case SC_BASIC_TYPE_SPECIFIER:
        case SC_TYPE_QUALIFIER:
        case SC_FUNCTION_SPECIFIER:
        case SC_STORAGE_CLASS_SPECIFIER:
            break;
        default:
        {
            if (debug)
                warnf("no defined free'ing operation for syntax elements of type %s\n", SYNTAX_COMPONENT_NAMES[syn->type]);
            break;
        }
    }
    free(syn);
}