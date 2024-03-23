#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "cc.h"

#define dealloc_null_return { free(s); return NULL; }
#define null_protection(x) if (!(x)) dealloc_null_return
#define safe_lexer_token_next(tok) { tok = tok->next; if (!tok) return NULL; }
#define parse_terminate(tok, fmt, ...) terminate("(line %d, col. %d) " fmt, (tok)->row, (tok)->col, ## __VA_ARGS__)

// -- laws for the maybe functions --
//
// a maybe function takes two parameters:
//  - the translation unit being parsed
//  - a reference to the local tracing token variable of the function that called it
//
// if a maybe function returns null, it must not modify the object
// at the reference of the second parameter.
//
// a maybe function may change the reference of the second parameter to be a null pointer.

// i'm introducing something new for left recursive grammars:
// maybe_parse_(grammar)_extension
// these functions will create a superstructure of the current syntax component being worked on.

// FORWARD DECLARATIONS
syntax_component_t* maybe_parse_struct_union(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_enum(syntax_component_t* unit, lexer_token_t** toks_ref);
vector_t* maybe_parse_specifiers_qualifiers(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_qualifier(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_declarator(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_designator(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_initializer(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_declaration(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_parameter_declaration(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_primary_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_postfix_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_unary_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_cast_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_multiplicative_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_additive_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_shift_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_relational_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_equality_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_and_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_xor_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_or_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_logical_and_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_logical_or_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_conditional_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_assignment_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_constant_expression(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_statement(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* maybe_parse_function_definition(syntax_component_t* unit, lexer_token_t** toks_ref);
syntax_component_t* parse(lexer_token_t* toks);
// FORWARD DECLARATIONS

syntax_component_t* maybe_parse_struct_union(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_STRUCT_UNION;
    null_protection(*toks_ref);
    lexer_token_t* tok = *toks_ref;
    if (tok->type != LEXER_TOKEN_KEYWORD)
        dealloc_null_return;
    if (tok->keyword_id == KEYWORD_STRUCT)
        s->sc5_is_union = false;
    else if (tok->keyword_id == KEYWORD_UNION)
        s->sc5_is_union = true;
    else
        dealloc_null_return;
    safe_lexer_token_next(tok); // move past 'struct' or 'union'
    if (tok->type == LEXER_TOKEN_IDENTIFIER)
    {
        s->sc5_identifier = tok->string_value;
        safe_lexer_token_next(tok); // move past identifier
    }
    else
        s->sc5_identifier = s->sc5_is_union ? "__cc_unnamed_union__" : "__cc_unnamed_struct__";
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '{')
    {
        s->sc5_declarations = vector_init();
        safe_lexer_token_next(tok);
        while (tok && (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != '}'))
        {
            syntax_component_t* decl = maybe_parse_declaration(unit, &tok);
            if (!decl)
                dealloc_null_return;
            vector_add(s->sc5_declarations, decl);
        }
        if (!tok) dealloc_null_return; // for the instance in which you had like: struct { int a;(EOF)
        safe_lexer_token_next(tok); // move past ending brace
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_enum(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_ENUM;
    null_protection(*toks_ref);
    lexer_token_t* tok = *toks_ref;
    if (tok->type != LEXER_TOKEN_KEYWORD)
        dealloc_null_return;
    if (tok->keyword_id != KEYWORD_ENUM)
        dealloc_null_return;
    safe_lexer_token_next(tok); // move past 'enum'
    if (tok->type == LEXER_TOKEN_IDENTIFIER)
    {
        s->sc6_identifier = tok->string_value;
        safe_lexer_token_next(tok);
    }
    else
        s->sc6_identifier = "__cc_unnamed_enum__";
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '{')
    {
        s->sc6_enumerators = vector_init();
        safe_lexer_token_next(tok);
        while (tok && (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != '}'))
        {
            if (tok->type != LEXER_TOKEN_IDENTIFIER)
                dealloc_null_return;
            syntax_component_t* enumerator = calloc(1, sizeof *enumerator);
            enumerator->type = SYNTAX_COMPONENT_ENUMERATOR;
            enumerator->sc7_identifier = tok->string_value;
            safe_lexer_token_next(tok);
            syntax_component_t* expr = NULL;
            if (tok->type == ',')
                safe_lexer_token_next(tok)
            else if (tok->type == '=')
            {
                safe_lexer_token_next(tok);
                expr = maybe_parse_expression(unit, &tok);
                if (!expr)
                    dealloc_null_return;
                if (tok->type == ',')
                    safe_lexer_token_next(tok)
                else if (tok->type != '}')
                    dealloc_null_return;
            }
            vector_add(s->sc6_enumerators, expr);
        }
        if (!tok) dealloc_null_return; // for the instance in which you had like: struct { int a;(end of file)
        safe_lexer_token_next(tok); // move past ending brace
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_type_name(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_TYPE_NAME;
    null_protection(*toks_ref);
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* specs_quals = maybe_parse_specifiers_qualifiers(unit, &tok);
    if (!specs_quals)
        dealloc_null_return;
    syntax_component_t* declarator = maybe_parse_declarator(unit, &tok);
    if (!declarator)
        dealloc_null_return;
    s->sc12_specifiers_qualifiers = specs_quals;
    s->sc12_declarator = declarator;
    return s;
}

// yes these need to be parsed together, not i'm not a little baby boy
vector_t* maybe_parse_specifiers_qualifiers(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    #define local_dealloc_null_return { free(v); return NULL; }
    #define is_arithmetic_type_keyword(x) ((x) == KEYWORD_CHAR || \
        (x) == KEYWORD_SHORT || \
        (x) == KEYWORD_INT || \
        (x) == KEYWORD_LONG || \
        (x) == KEYWORD_SIGNED || \
        (x) == KEYWORD_UNSIGNED || \
        (x) == KEYWORD_FLOAT || \
        (x) == KEYWORD_DOUBLE || \
        (x) == KEYWORD_COMPLEX || \
        (x) == KEYWORD_IMAGINARY)
    vector_t* v = vector_init();
    lexer_token_t* tok = *toks_ref;
    if (!tok) local_dealloc_null_return;

    unsigned keyword_count[KEYWORDS_LEN];
    memset(keyword_count, 0, KEYWORDS_LEN * sizeof(unsigned));
    unsigned arithmetic_keyword_counts = 0;

    for (;;)
    {
        syntax_component_t* s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_SPECIFIER_QUALIFIER;
        if (!tok)
        {
            free(s);
            break;
        }
        if (tok->type == LEXER_TOKEN_KEYWORD)
        {
            if (!keyword_count[tok->keyword_id] && is_arithmetic_type_keyword(tok->keyword_id))
                ++arithmetic_keyword_counts;
            ++(keyword_count[tok->keyword_id]);

            #define storage_class_if(x, y) \
            if (keyword_count[(y)] == 1 && tok->keyword_id == (y)) \
            { \
                s->sc2_type = SPECIFIER_QUALIFIER_STORAGE_CLASS; \
                s->sc2_storage_class_id = (x); \
                vector_add(v, s); \
            }
            #define qualifier_if(x, y) \
            if (keyword_count[(y)] == 1 && tok->keyword_id == (y)) \
            { \
                s->sc2_type = SPECIFIER_QUALIFIER_QUALIFIER; \
                s->sc2_qualifier_id = (x); \
                vector_add(v, s); \
            }

            // these are the easy type specifiers, arithmetic types come after everything LOL
            if (keyword_count[KEYWORD_VOID] == 1 && tok->keyword_id == KEYWORD_VOID)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_VOID;
                vector_add(v, s);
            }
            else storage_class_if(STORAGE_CLASS_TYPEDEF, KEYWORD_TYPEDEF)
            else storage_class_if(STORAGE_CLASS_AUTO, KEYWORD_AUTO)
            else storage_class_if(STORAGE_CLASS_REGISTER, KEYWORD_REGISTER)
            else storage_class_if(STORAGE_CLASS_STATIC, KEYWORD_STATIC)
            else storage_class_if(STORAGE_CLASS_EXTERN, KEYWORD_EXTERN)
            else qualifier_if(QUALIFIER_CONST, KEYWORD_CONST)
            else qualifier_if(QUALIFIER_VOLATILE, KEYWORD_VOLATILE)
            else qualifier_if(QUALIFIER_RESTRICT, KEYWORD_RESTRICT)
            else if (keyword_count[KEYWORD_INLINE] == 1 && tok->keyword_id == KEYWORD_INLINE)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_FUNCTION;
                s->sc2_function_id = FUNCTION_SPECIFIER_INLINE;
                vector_add(v, s);
            }
            else if (keyword_count[KEYWORD_STRUCT] == 1 && tok->keyword_id == KEYWORD_STRUCT)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_STRUCT;
                s->sc2_struct = maybe_parse_struct_union(unit, &tok);
                if (!s->sc2_struct)
                {
                    free(s);
                    local_dealloc_null_return;
                }
                vector_add(v, s);
            }
            else if (keyword_count[KEYWORD_UNION] == 1 && tok->keyword_id == KEYWORD_UNION)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_UNION;
                s->sc2_union = maybe_parse_struct_union(unit, &tok);
                if (!s->sc2_union)
                {
                    free(s);
                    local_dealloc_null_return;
                }
                vector_add(v, s);
            }
            else if (keyword_count[KEYWORD_ENUM] == 1 && tok->keyword_id == KEYWORD_ENUM)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_ENUM;
                s->sc2_enum = maybe_parse_enum(unit, &tok);
                if (!s->sc2_enum)
                {
                    free(s);
                    local_dealloc_null_return;
                }
                vector_add(v, s);
            }
            else
                free(s);
            
            tok = tok->next;
            
            #undef storage_class_body
            #undef qualifier_body
        }
        else if (tok->type == LEXER_TOKEN_IDENTIFIER)
        {
            syntax_component_t* typedef_declaration, * typedef_declarator;
            find_typedef(&typedef_declaration, &typedef_declarator, unit, tok->string_value);
            if (!typedef_declarator)
            {
                free(s);
                break;
            }
            s->sc2_type = SPECIFIER_QUALIFIER_TYPEDEF;
            s->sc2_typedef_declaration = typedef_declaration;
            s->sc2_typedef_declarator = typedef_declarator;
            tok = tok->next;
            vector_add(v, s);
        }
        else
        {
            free(s);
            break;
        }
    }

    syntax_component_t* arith_type_specifier = calloc(1, sizeof *arith_type_specifier);
    arith_type_specifier->type = SYNTAX_COMPONENT_SPECIFIER_QUALIFIER;
    arith_type_specifier->sc2_type = SPECIFIER_QUALIFIER_ARITHMETIC_TYPE;

    #define arithmetic_body(x) \
    { \
        arith_type_specifier->sc2_arithmetic_type_id = (x); \
        vector_add(v, arith_type_specifier); \
    }

    if (keyword_count[KEYWORD_BOOL] == 1 && arithmetic_keyword_counts == 1)
        arithmetic_body(ARITHMETIC_TYPE_BOOL)
    else if (keyword_count[KEYWORD_CHAR] == 1 && arithmetic_keyword_counts == 1)
        arithmetic_body(ARITHMETIC_TYPE_CHAR)
    else if (keyword_count[KEYWORD_CHAR] == 1 && keyword_count[KEYWORD_SIGNED] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_SIGNED_CHAR)
    else if (keyword_count[KEYWORD_CHAR] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_CHAR)
    else if (keyword_count[KEYWORD_SHORT] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && (arithmetic_keyword_counts == 2 ||
        (keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 3)))
        arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_SHORT_INT)
    else if (keyword_count[KEYWORD_SHORT] == 1 && (arithmetic_keyword_counts == 1 ||
        (keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 2) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && arithmetic_keyword_counts == 2) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 3)))
        arithmetic_body(ARITHMETIC_TYPE_SHORT_INT)
    else if ((keyword_count[KEYWORD_UNSIGNED] == 1 && arithmetic_keyword_counts == 1) ||
        (keyword_count[KEYWORD_INT] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && arithmetic_keyword_counts == 2))
        arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_INT)
    else if ((keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 1) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && arithmetic_keyword_counts == 1) ||
        (keyword_count[KEYWORD_INT] == 1 && keyword_count[KEYWORD_SIGNED] == 1 && arithmetic_keyword_counts == 2))
        arithmetic_body(ARITHMETIC_TYPE_INT)
    else if (keyword_count[KEYWORD_LONG] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && (arithmetic_keyword_counts == 2 ||
        (keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 3)))
        arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_LONG_INT)
    else if (keyword_count[KEYWORD_LONG] == 1 && (arithmetic_keyword_counts == 1 ||
        (keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 2) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && arithmetic_keyword_counts == 2) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 3)))
        arithmetic_body(ARITHMETIC_TYPE_LONG_INT)
    else if (keyword_count[KEYWORD_LONG] == 2 && keyword_count[KEYWORD_UNSIGNED] == 1 && (arithmetic_keyword_counts == 2 ||
        (keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 3)))
        arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_LONG_LONG_INT)
    else if (keyword_count[KEYWORD_LONG] == 2 && (arithmetic_keyword_counts == 1 ||
        (keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 2) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && arithmetic_keyword_counts == 2) ||
        (keyword_count[KEYWORD_SIGNED] == 1 && keyword_count[KEYWORD_INT] == 1 && arithmetic_keyword_counts == 3)))
        arithmetic_body(ARITHMETIC_TYPE_LONG_LONG_INT)
    else if (keyword_count[KEYWORD_FLOAT] == 1 && arithmetic_keyword_counts == 1)
        arithmetic_body(ARITHMETIC_TYPE_FLOAT)
    else if (keyword_count[KEYWORD_DOUBLE] == 1 && arithmetic_keyword_counts == 1)
        arithmetic_body(ARITHMETIC_TYPE_DOUBLE)
    else if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_LONG] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_LONG_DOUBLE)
    else if (keyword_count[KEYWORD_FLOAT] == 1 && keyword_count[KEYWORD_COMPLEX] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_FLOAT_COMPLEX)
    else if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_COMPLEX] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_DOUBLE_COMPLEX)
    else if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_LONG] == 1 && keyword_count[KEYWORD_COMPLEX] == 1 &&
        arithmetic_keyword_counts == 3)
        arithmetic_body(ARITHMETIC_TYPE_LONG_DOUBLE_COMPLEX)
    else if (keyword_count[KEYWORD_FLOAT] == 1 && keyword_count[KEYWORD_IMAGINARY] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_FLOAT_IMAGINARY)
    else if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_IMAGINARY] == 1 && arithmetic_keyword_counts == 2)
        arithmetic_body(ARITHMETIC_TYPE_DOUBLE_IMAGINARY)
    else if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_LONG] == 1 && keyword_count[KEYWORD_IMAGINARY] == 1 &&
        arithmetic_keyword_counts == 3)
        arithmetic_body(ARITHMETIC_TYPE_LONG_DOUBLE_IMAGINARY)
    else
        free(arith_type_specifier);

    #undef local_dealloc_null_return
    #undef is_arithmetic_type_keyword

    *toks_ref = tok;
    return v;
}

syntax_component_t* maybe_parse_qualifier(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_SPECIFIER_QUALIFIER;
    s->sc2_type = SPECIFIER_QUALIFIER_QUALIFIER;
    lexer_token_t* tok = *toks_ref;
    if (tok->type != LEXER_TOKEN_KEYWORD)
        dealloc_null_return;
    if (tok->keyword_id == KEYWORD_CONST)
        s->sc2_qualifier_id = QUALIFIER_CONST;
    else if (tok->keyword_id == KEYWORD_VOLATILE)
        s->sc2_qualifier_id = QUALIFIER_VOLATILE;
    else if (tok->keyword_id == KEYWORD_RESTRICT)
        s->sc2_qualifier_id = QUALIFIER_RESTRICT;
    else
        dealloc_null_return;
    *toks_ref = tok->next;
    return s;
}

syntax_component_t* maybe_parse_declarator_extension(syntax_component_t* unit, syntax_component_t* super, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    if (!tok) return NULL;
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_DECLARATOR;
    if (tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '[')
    {
        safe_lexer_token_next(tok);
        s->sc3_type = DECLARATOR_ARRAY;
        s->sc3_array_static = false;
        if (tok->type == LEXER_TOKEN_KEYWORD && tok->keyword_id == KEYWORD_STATIC)
        {
            s->sc3_array_static = true;
            safe_lexer_token_next(tok);
        }
        s->sc3_array_qualifiers = vector_init();
        s->sc3_array_expression = NULL;
        s->sc3_array_subdeclarator = super;
        for (;;)
        {
            syntax_component_t* qualifier = maybe_parse_qualifier(unit, &tok);
            if (!qualifier)
                break;
            vector_add(s->sc3_array_qualifiers, qualifier);
        }
        if (tok->type == LEXER_TOKEN_KEYWORD && tok->keyword_id == KEYWORD_STATIC)
        {
            if (s->sc3_array_static)
                dealloc_null_return;
            s->sc3_array_static = true;
            safe_lexer_token_next(tok);
        }
        if (tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '*')
        {
            if (s->sc3_array_static)
                dealloc_null_return;
            safe_lexer_token_next(tok);
        }
        else
            s->sc3_array_expression = maybe_parse_expression(unit, &tok);
        if (tok->type != LEXER_TOKEN_OPERATOR)
            dealloc_null_return;
        if (tok->operator_id != ']')
            dealloc_null_return;
        tok = tok->next;
    }
    else if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '(')
    {
        safe_lexer_token_next(tok);
        s->sc3_type = DECLARATOR_FUNCTION;
        bool old_style = tok->type == LEXER_TOKEN_IDENTIFIER || (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == ')');
        s->sc3_function_subdeclarator = super;
        if (old_style)
            s->sc3_function_identifiers = vector_init();
        else
            s->sc3_function_parameters = vector_init();
        while (tok && (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != ')'))
        {
            if (old_style)
            {
                if (tok->type != LEXER_TOKEN_IDENTIFIER)
                    dealloc_null_return;
                vector_add(s->sc3_function_identifiers, tok->string_value);
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
            }
            else
            {
                syntax_component_t* decl = maybe_parse_parameter_declaration(unit, &tok);
                if (!decl)
                    dealloc_null_return;
                if (decl->sc15_ellipsis && (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != ')'))
                    dealloc_null_return;
                vector_add(s->sc3_function_parameters, decl);
            }
            if (tok->type != LEXER_TOKEN_SEPARATOR)
                dealloc_null_return;
            if (tok->separator_id == ',')
                safe_lexer_token_next(tok)
            else if (tok->separator_id != ')')
                dealloc_null_return;
        }
        tok = tok->next;
    }
    else
        dealloc_null_return;
    syntax_component_t* sub = maybe_parse_declarator_extension(unit, s, &tok);
    *toks_ref = tok;
    return sub ? sub : s;
}

/*
    GRAMMAR

    declarator:
        identifier declarator-extend
        "(" declarator ")"
        "*" [qualifiers] declarator

    declarator-extend:
        "[" ["static"] [qualifiers] expression "]" declarator-extend
        "[" [qualifiers] ["static"] expression "]" declarator-extend
        "[" [qualifiers] "*" "]" declarator-extend
        "(" parameters-or-identifiers ")" declarator-extend
        ""
*/
syntax_component_t* maybe_parse_declarator(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_DECLARATOR;
    lexer_token_t* tok = *toks_ref;
    if (tok->type == LEXER_TOKEN_IDENTIFIER)
    {
        s->sc3_type = DECLARATOR_IDENTIFIER;
        s->sc3_identifier = tok->string_value;
        tok = tok->next;
    }
    else if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '(')
    {
        safe_lexer_token_next(tok);
        s->sc3_type = DECLARATOR_NEST;
        syntax_component_t* nested_declarator = maybe_parse_declarator(unit, &tok);
        if (!nested_declarator)
            dealloc_null_return;
        s->sc3_nested_declarator = nested_declarator;
        if (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != ')')
            dealloc_null_return;
        tok = tok->next;
    }
    else if (tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '*')
    {
        safe_lexer_token_next(tok);
        s->sc3_type = DECLARATOR_POINTER;
        s->sc3_pointer_qualifiers = vector_init();
        for (;;)
        {
            syntax_component_t* qualifier = maybe_parse_qualifier(unit, &tok);
            if (!qualifier)
                break;
            vector_add(s->sc3_pointer_qualifiers, qualifier);
        }
        syntax_component_t* subdeclarator = maybe_parse_declarator(unit, &tok);
        s->sc3_pointer_subdeclarator = subdeclarator;
    }
    else
        dealloc_null_return;
    syntax_component_t* ext = maybe_parse_declarator_extension(unit, s, &tok);
    if (ext) s = ext;
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_designator(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_DESIGNATOR;
    s->sc8_array_designator_expression = NULL;
    s->sc8_struct_designator_identifier = NULL;
    lexer_token_t* tok = *toks_ref;
    if (tok->type != LEXER_TOKEN_OPERATOR)
        dealloc_null_return;
    if (tok->operator_id == '.')
    {
        safe_lexer_token_next(tok);
        if (tok->type != LEXER_TOKEN_IDENTIFIER)
            dealloc_null_return;
        s->sc8_struct_designator_identifier = tok->string_value;
        safe_lexer_token_next(tok);
    }
    else if (tok->operator_id == '[')
    {
        safe_lexer_token_next(tok);
        syntax_component_t* expr = maybe_parse_expression(unit, &tok);
        if (!expr)
            dealloc_null_return;
        s->sc8_array_designator_expression = expr;
        if (tok->type != LEXER_TOKEN_OPERATOR)
            dealloc_null_return;
        if (tok->operator_id != ']')
            dealloc_null_return;
        safe_lexer_token_next(tok);
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_initializer(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_INITIALIZER;
    lexer_token_t* tok = *toks_ref;
    if (tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '=')
        safe_lexer_token_next(tok);
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '{') // initializer list
    {
        safe_lexer_token_next(tok);
        s->sc4_type = INITIALIZER_LIST;
        s->sc4_initializer_list = vector_init();
        for (;;)
        {
            syntax_component_t* initializer = maybe_parse_initializer(unit, &tok);
            if (!initializer)
            {
                initializer = calloc(1, sizeof *initializer);
                initializer->sc4_type = INITIALIZER_DESIGNATION;
                initializer->sc4_designator_list = vector_init();
                for (;;)
                {
                    syntax_component_t* designator = maybe_parse_designator(unit, &tok);
                    if (!designator)
                        break;
                    vector_add(initializer->sc4_designator_list, designator);
                }
                if (tok->type != LEXER_TOKEN_OPERATOR)
                    dealloc_null_return;
                if (tok->operator_id != '=')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* subinitializer = maybe_parse_initializer(unit, &tok);
                if (!subinitializer)
                    dealloc_null_return;
                initializer->sc4_subinitializer = subinitializer;
            }
            vector_add(s->sc4_initializer_list, initializer);
            if (tok->type == LEXER_TOKEN_OPERATOR && tok->separator_id == ',')
                safe_lexer_token_next(tok)
            else if (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != '}')
                dealloc_null_return
            else
                break;
        }
        tok = tok->next;
    }
    else // expression
    {
        s->sc4_type = INITIALIZER_EXPRESSION;
        syntax_component_t* expr = maybe_parse_expression(unit, &tok);
        if (!expr)
            dealloc_null_return;
        s->sc4_expression = expr;
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_declaration(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_DECLARATION;
    lexer_token_t* tok = *toks_ref;
    s->sc1_specifiers_qualifiers = maybe_parse_specifiers_qualifiers(unit, &tok);
    if (!s->sc1_specifiers_qualifiers)
        dealloc_null_return;
    s->sc1_declarators = vector_init();
    s->sc1_initializers = vector_init();

    // do some checking here to ensure there is at least one type specifier
    bool found_type_specifier = false;
    for (unsigned i = 0; i < s->sc1_specifiers_qualifiers->size; ++i)
    {
        syntax_component_t* spec_qual = vector_get(s->sc1_specifiers_qualifiers, i);
        unsigned type = spec_qual->sc2_type;
        if (type == SPECIFIER_QUALIFIER_VOID ||
            type == SPECIFIER_QUALIFIER_ARITHMETIC_TYPE ||
            type == SPECIFIER_QUALIFIER_TYPEDEF ||
            type == SPECIFIER_QUALIFIER_STRUCT ||
            type == SPECIFIER_QUALIFIER_UNION ||
            type == SPECIFIER_QUALIFIER_ENUM)
        {
            found_type_specifier = true;
            break;
        }
    }
    if (!found_type_specifier)
        dealloc_null_return;
    for (;;)
    {
        syntax_component_t* declarator = maybe_parse_declarator(unit, &tok);
        if (!declarator)
            break;
        vector_add(s->sc1_declarators, declarator);
        syntax_component_t* initializer = maybe_parse_initializer(unit, &tok);
        vector_add(s->sc1_initializers, initializer);
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            dealloc_null_return;
        if (tok->separator_id == ',')
            safe_lexer_token_next(tok)
        else if (tok->separator_id == ';')
            break;
        else
            dealloc_null_return;
    }
    if (tok->type != LEXER_TOKEN_SEPARATOR)
        dealloc_null_return;
    if (tok->separator_id != ';')
        dealloc_null_return;
    *toks_ref = tok->next;
    return s;
}

syntax_component_t* maybe_parse_parameter_declaration(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_PARAMETER_DECLARATION;
    s->sc15_ellipsis = false;
    lexer_token_t* tok = *toks_ref;
    null_protection(tok);
    if (tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '.' * '.' * '.') // ellipsis
    {
        s->sc15_ellipsis = true;
        *toks_ref = tok->next;
        return s;
    }
    s->sc15_specifiers_qualifiers = maybe_parse_specifiers_qualifiers(unit, &tok);
    if (!s->sc15_specifiers_qualifiers)
        dealloc_null_return;
    s->sc15_declarator = maybe_parse_declarator(unit, &tok);
    // TODO there's probably some abstract declarator stuff i have to do here but i cba rn lol
    *toks_ref = tok;
    return s;
}

static bool c(unsigned lhs, unsigned len, ...)
{
    va_list args;
    va_start(args, len);
    for (unsigned i = 0; i < len; ++i)
        if (lhs == va_arg(args, unsigned))
        {
            va_end(args);
            return true;
        }
    va_end(args);
    return false;
}

syntax_component_t* maybe_parse_primary_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    if (!tok) return NULL;
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_EXPRESSION;
    s->sc11_type = EXPRESSION_PRIMARY;
    if (tok->type == LEXER_TOKEN_IDENTIFIER)
    {
        s->sc11_primary_type = EXPRESSION_PRIMARY_IDENTIFIER;
        s->sc11_primary_identifier = tok->string_value;
    }
    else if (tok->type == LEXER_TOKEN_INTEGER_CONSTANT)
    {
        s->sc11_primary_type = EXPRESSION_PRIMARY_INTEGER_CONSTANT;
        s->sc11_primary_integer_constant = 0; // TODO
    }
    else if (tok->type == LEXER_TOKEN_CHARACTER_CONSTANT)
    {
        s->sc11_primary_type = EXPRESSION_PRIMARY_INTEGER_CONSTANT;
        s->sc11_primary_integer_constant = 0; // TODO
    }
    else if (tok->type == LEXER_TOKEN_FLOATING_CONSTANT)
    {
        s->sc11_primary_type = EXPRESSION_PRIMARY_FLOATING_CONSTANT;
        s->sc11_primary_floating_constant = 0.0L; // TODO
    }
    else if (tok->type == LEXER_TOKEN_STRING_CONSTANT)
    {
        s->sc11_primary_type = EXPRESSION_PRIMARY_STRING_LITERAL;
        s->sc11_primary_string_literal = tok->string_value; // TODO
    }
    else if (tok->type == LEXER_TOKEN_SEPARATOR)
    {
        if (tok->separator_id != '(')
            dealloc_null_return;
        safe_lexer_token_next(tok);
        syntax_component_t* expr = maybe_parse_expression(unit, &tok);
        if (!expr)
            dealloc_null_return;
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            dealloc_null_return;
        if (tok->separator_id != ')')
            dealloc_null_return;
        s->sc11_primary_type = EXPRESSION_PRIMARY_NEST;
        s->sc11_primary_nested_expression = expr;
    }
    else
        dealloc_null_return;
    *toks_ref = tok->next;
    return s;
}

syntax_component_t* maybe_parse_postfix_expression_extension(syntax_component_t* unit, syntax_component_t* super, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    if (!tok) return NULL;
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_EXPRESSION;
    s->sc11_type = EXPRESSION_POSTFIX;
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '(')
    {
        safe_lexer_token_next(tok);
        s->sc11_postfix_type = EXPRESSION_POSTFIX_FUNCTION_CALL;
        s->sc11_postfix_argument_list = vector_init();
        for (;;)
        {
            syntax_component_t* expr = maybe_parse_assignment_expression(unit, &tok);
            if (!expr)
                break;
            vector_add(s->sc11_postfix_argument_list, expr);
            if (tok->type != LEXER_TOKEN_SEPARATOR)
                dealloc_null_return;
            if (tok->separator_id == ',')
                safe_lexer_token_next(tok)
            else if (tok->separator_id != ')')
                dealloc_null_return
            else
                break;
        }
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            dealloc_null_return;
        if (tok->separator_id != ')')
            dealloc_null_return;
        tok = tok->next;
    }
    else if (tok->type == LEXER_TOKEN_OPERATOR)
    {
        if (tok->operator_id == '[')
        {
            safe_lexer_token_next(tok);
            s->sc11_postfix_type = EXPRESSION_POSTFIX_SUBSCRIPT;
            s->sc11_postfix_subscript_expression = maybe_parse_expression(unit, &tok);
            if (!s->sc11_postfix_subscript_expression)
                dealloc_null_return;
            if (tok->type != LEXER_TOKEN_OPERATOR)
                dealloc_null_return;
            if (tok->operator_id != ']')
                dealloc_null_return;
            tok = tok->next;
        }
        else if (tok->operator_id == '.' || tok->operator_id == '-' * '>')
        {
            safe_lexer_token_next(tok);
            s->sc11_postfix_type = tok->operator_id == '.' ? EXPRESSION_POSTFIX_MEMBER : EXPRESSION_POSTFIX_PTR_MEMBER;
            if (tok->type != LEXER_TOKEN_IDENTIFIER)
                dealloc_null_return;
            s->sc11_postfix_member_identifier = tok->string_value;
            tok = tok->next;
        }
        else if (tok->operator_id == '+' * '+' || tok->operator_id == '-' * '-')
        {
            s->sc11_postfix_type = tok->operator_id == '+' * '+' ? EXPRESSION_POSTFIX_INCREMENT : EXPRESSION_POSTFIX_DECREMENT;
            tok = tok->next;
        }
        else
            dealloc_null_return;
    }
    else
        dealloc_null_return;
    syntax_component_t* sub = maybe_parse_declarator_extension(unit, s, &tok);
    *toks_ref = tok;
    return sub ? sub : s;
}

syntax_component_t* maybe_parse_postfix_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* s = maybe_parse_primary_expression(unit, &tok);
    if (!s)
    {
        if (!tok) return NULL;
        s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_EXPRESSION;
        s->sc11_type = EXPRESSION_POSTFIX;
        s->sc11_postfix_type = EXPRESSION_POSTFIX_COMPOUND_LITERAL;
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            dealloc_null_return;
        if (tok->separator_id != '(')
            dealloc_null_return;
        safe_lexer_token_next(tok);
        syntax_component_t* type = maybe_parse_type_name(unit, &tok);
        if (!type)
            dealloc_null_return;
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            dealloc_null_return;
        if (tok->separator_id != ')')
            dealloc_null_return;
        safe_lexer_token_next(tok);
        syntax_component_t* init_list = maybe_parse_initializer(unit, &tok);
        if (!init_list)
            dealloc_null_return;
        if (!init_list->sc4_type != INITIALIZER_LIST)
            dealloc_null_return;
        s->sc11_postfix_type_name = type;
        s->sc11_postfix_initializer_list = init_list;
    }
    syntax_component_t* ext = maybe_parse_postfix_expression_extension(unit, s, &tok);
    *toks_ref = tok;
    return ext ? ext : s;
}

syntax_component_t* maybe_parse_unary_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* s = maybe_parse_postfix_expression(unit, &tok);
    if (!s)
    {
        s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_EXPRESSION;
        s->sc11_type = EXPRESSION_UNARY;
        if (tok->type == LEXER_TOKEN_KEYWORD && tok->keyword_id == KEYWORD_SIZEOF)
        {
            s->sc11_operator_id = (unsigned) 'sizeof';
            safe_lexer_token_next(tok);
            if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '(')
            {
                safe_lexer_token_next(tok);
                syntax_component_t* type = maybe_parse_type_name(unit, &tok);
                if (!type)
                    dealloc_null_return;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ')')
                    dealloc_null_return;
                s->sc11_unary_type = type;
                tok = tok->next;
            }
            else
            {
                syntax_component_t* nested = maybe_parse_unary_expression(unit, &tok);
                if (!nested)
                    dealloc_null_return;
                s->sc11_unary_nested_expression = nested;
            }
        }
        else if (tok->type == LEXER_TOKEN_OPERATOR)
        {
            s->sc11_operator_id = tok->operator_id;
            if (tok->operator_id == '+' * '+' || tok->operator_id == '-' * '-')
            {
                safe_lexer_token_next(tok);
                syntax_component_t* nested = maybe_parse_unary_expression(unit, &tok);
                if (!nested)
                    dealloc_null_return;
                s->sc11_unary_nested_expression = nested;
            }
            else if (is_unary_operator_token(tok))
            {
                safe_lexer_token_next(tok);
                syntax_component_t* cast = maybe_parse_cast_expression(unit, &tok);
                if (!cast)
                    dealloc_null_return;
                s->sc11_unary_cast_expression = cast;
            }
            else
                dealloc_null_return;
        }
        else
            dealloc_null_return;
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_cast_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* s = maybe_parse_unary_expression(unit, &tok);
    if (!s)
    {
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            return NULL;
        if (tok->separator_id != '(')
            return NULL;
        safe_lexer_token_next(tok);
        s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_EXPRESSION;
        s->sc11_type = EXPRESSION_CAST;
        s->sc11_operator_id = '(' * ')';
        s->sc11_cast_type = maybe_parse_type_name(unit, &tok);
        if (!s->sc11_cast_type)
            dealloc_null_return;
        if (tok->type != LEXER_TOKEN_SEPARATOR)
            dealloc_null_return;
        if (tok->separator_id != ')')
            dealloc_null_return;
        safe_lexer_token_next(tok);
        syntax_component_t* nested = maybe_parse_cast_expression(unit, &tok);
        if (!nested)
            dealloc_null_return;
        s->sc11_cast_nested_expression = nested;
    }
    *toks_ref = tok;
    return s;
}

#define generate_maybe_parse_binary_operator_expression(name, t, subfunc, l, r, ...) \
    syntax_component_t* maybe_parse_##name##_expression_extension(syntax_component_t* unit, syntax_component_t* super, lexer_token_t** toks_ref) \
    { \
        lexer_token_t* tok = *toks_ref; \
        if (!tok) return NULL; \
        syntax_component_t* s = calloc(1, sizeof *s); \
        s->type = SYNTAX_COMPONENT_EXPRESSION; \
        s->sc11_type = t; \
        unsigned oid = tok->operator_id; \
        if (tok->type == LEXER_TOKEN_OPERATOR && c(oid, numargs(__VA_ARGS__), ## __VA_ARGS__)) \
        { \
            s->sc11_operator_id = oid; \
            safe_lexer_token_next(tok); \
            syntax_component_t* right = (subfunc)(unit, &tok); \
            if (!right) \
                dealloc_null_return; \
            s->l = super; \
            s->r = right; \
        } \
        else \
            dealloc_null_return; \
        syntax_component_t* sub = maybe_parse_logical_or_expression_extension(unit, s, &tok); \
        *toks_ref = tok; \
        return sub ? sub : s; \
    } \
    syntax_component_t* maybe_parse_##name##_expression(syntax_component_t* unit, lexer_token_t** toks_ref) \
    { \
        lexer_token_t* tok = *toks_ref; \
        syntax_component_t* base = (subfunc)(unit, &tok); \
        if (!base) \
            return NULL; \
        syntax_component_t* ext = maybe_parse_##name##_expression_extension(unit, base, &tok); \
        if (ext) base = ext; \
        *toks_ref = tok; \
        return base; \
    }

generate_maybe_parse_binary_operator_expression(multiplicative,
    EXPRESSION_MULTIPLICATIVE,
    maybe_parse_cast_expression,
    sc11_multiplicative_cast_expression,
    sc11_multiplicative_nested_expression,
    '*',
    '/',
    '%'
)

generate_maybe_parse_binary_operator_expression(additive,
    EXPRESSION_ADDITIVE,
    maybe_parse_multiplicative_expression,
    sc11_additive_multiplicative_expression,
    sc11_additive_nested_expression,
    '+',
    '-'
)

generate_maybe_parse_binary_operator_expression(shift,
    EXPRESSION_SHIFT,
    maybe_parse_additive_expression,
    sc11_shift_additive_expression,
    sc11_shift_nested_expression,
    '<' * '<',
    '>' * '>'
)

generate_maybe_parse_binary_operator_expression(relational,
    EXPRESSION_RELATIONAL,
    maybe_parse_shift_expression,
    sc11_relational_shift_expression,
    sc11_relational_nested_expression,
    '<',
    '>',
    '<' * '=',
    '>' * '='
)

generate_maybe_parse_binary_operator_expression(equality,
    EXPRESSION_EQUALITY,
    maybe_parse_relational_expression,
    sc11_equality_relational_expression,
    sc11_equality_nested_expression,
    '=' * '=',
    '!' * '='
)

generate_maybe_parse_binary_operator_expression(and,
    EXPRESSION_AND,
    maybe_parse_equality_expression,
    sc11_and_equality_expression,
    sc11_and_nested_expression,
    '&'
)

generate_maybe_parse_binary_operator_expression(xor,
    EXPRESSION_XOR,
    maybe_parse_and_expression,
    sc11_xor_and_expression,
    sc11_xor_nested_expression,
    '^'
)

generate_maybe_parse_binary_operator_expression(or,
    EXPRESSION_OR,
    maybe_parse_xor_expression,
    sc11_or_xor_expression,
    sc11_or_nested_expression,
    '|'
)

generate_maybe_parse_binary_operator_expression(logical_and,
    EXPRESSION_LOGICAL_AND,
    maybe_parse_or_expression,
    sc11_land_or_expression,
    sc11_land_nested_expression,
    '&' * '&'
)

generate_maybe_parse_binary_operator_expression(logical_or,
    EXPRESSION_LOGICAL_OR,
    maybe_parse_logical_and_expression,
    sc11_lor_land_expression,
    sc11_lor_nested_expression,
    '|' * '|'
)

/*
syntax_component_t* maybe_parse_logical_or_expression_extension(syntax_component_t* unit, syntax_component_t* super, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    if (!tok) return NULL;
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_EXPRESSION;
    s->sc11_type = EXPRESSION_LOGICAL_OR;
    s->sc11_operator_id = '|' * '|';
    unsigned oid = tok->operator_id;
    if (tok->type == LEXER_TOKEN_OPERATOR && oid == '|' * '|')
    {
        safe_lexer_token_next(tok);
        syntax_component_t* right = maybe_parse_logical_and_expression(unit, &tok);
        if (!right)
            dealloc_null_return;
        s->sc11_lor_land_expression = super;
        s->sc11_lor_nested_expression = right;
    }
    else
        dealloc_null_return;
    syntax_component_t* sub = maybe_parse_logical_or_expression_extension(unit, s, &tok);
    *toks_ref = tok;
    return sub ? sub : s;
}

syntax_component_t* maybe_parse_logical_or_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* base = maybe_parse_logical_and_expression(unit, &tok);
    if (!base)
        return NULL;
    syntax_component_t* ext = maybe_parse_logical_or_expression_extension(unit, base, &tok);
    if (ext) base = ext;
    *toks_ref = tok;
    return base;
}
*/

// 5 * 3 + 5

syntax_component_t* maybe_parse_conditional_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* s = maybe_parse_logical_or_expression(unit, &tok);
    if (!s)
        return NULL;
    if (tok && tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '?')
    {
        safe_lexer_token_next(tok);
        syntax_component_t* super = calloc(1, sizeof *super);
        super->type = SYNTAX_COMPONENT_EXPRESSION;
        super->sc11_type = EXPRESSION_CONDITIONAL;
        super->sc11_operator_id = '?';
        syntax_component_t* then = maybe_parse_expression(unit, &tok);
        if (!then)
            dealloc_null_return;
        if (!tok)
            dealloc_null_return;
        if (tok->type != LEXER_TOKEN_OPERATOR)
            dealloc_null_return;
        if (tok->operator_id != ':')
            dealloc_null_return;
        safe_lexer_token_next(tok);
        syntax_component_t* els = maybe_parse_conditional_expression(unit, &tok);
        if (!els)
            dealloc_null_return;
        super->sc11_conditional_lor_expression = s;
        super->sc11_conditional_then_expression = then;
        super->sc11_conditional_nested_expression = els;
        s = super;
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_assignment_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    syntax_component_t* s = maybe_parse_conditional_expression(unit, &tok);
    if (!s)
    {
        s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_EXPRESSION;
        s->sc11_type = EXPRESSION_ASSIGNMENT;
        syntax_component_t* unary = maybe_parse_unary_expression(unit, &tok);
        if (!unary)
            return NULL;
        s->sc11_assignment_unary_expression = unary;
        if (!is_assignment_operator_token(tok))
        {
            free(unary);
            return NULL;
        }
        s->sc11_operator_id = tok->operator_id;
        safe_lexer_token_next(tok);
        syntax_component_t* nested = maybe_parse_assignment_expression(unit, &tok);
        if (!nested)
        {
            free(unary);
            return NULL;
        }
        s->sc11_assignment_nested_expression = nested;
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_EXPRESSION;
    s->sc11_type = EXPRESSION_ASSIGNMENT_LIST;
    s->sc11_assignment_list = vector_init();
    lexer_token_t* tok = *toks_ref;
    // TODO
    for (;;)
    {
        syntax_component_t* expr = maybe_parse_assignment_expression(unit, &tok);
        if (!expr)
            dealloc_null_return;
        vector_add(s->sc11_assignment_list, expr);
        if (tok && tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == ',')
            continue;
        break;
    }
    *toks_ref = tok;
    return s;
}

syntax_component_t* maybe_parse_statement(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    lexer_token_t* tok = *toks_ref;
    if (!tok) return NULL;
    if (tok->type == LEXER_TOKEN_IDENTIFIER && tok->next && tok->next->type == LEXER_TOKEN_OPERATOR && tok->next->operator_id == ':')
    {
        syntax_component_t* s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_STATEMENT;
        s->sc10_type = STATEMENT_LABELED;
        s->sc10_labeled_type = STATEMENT_LABELED_IDENTIFIER;
        s->sc10_labeled_identifier = tok->string_value;
        *toks_ref = tok->next->next;
        return s;
    }
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '{')
    {
        syntax_component_t* s = calloc(1, sizeof *s);
        s->type = SYNTAX_COMPONENT_STATEMENT;
        s->sc10_type = STATEMENT_COMPOUND;
        s->sc10_compound_block_items = vector_init();
        safe_lexer_token_next(tok);
        while (tok && (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != '}'))
        {
            syntax_component_t* item = maybe_parse_statement(unit, &tok);
            if (!item)
            {
                item = maybe_parse_declaration(unit, &tok);
                if (!item)
                    dealloc_null_return;
            }
            vector_add(s->sc10_compound_block_items, item);
        }
        if (!tok)
            dealloc_null_return;
        *toks_ref = tok->next;
        return s;
    }
    if (tok->type == LEXER_TOKEN_KEYWORD)
    {
        switch (tok->keyword_id)
        {
            case KEYWORD_CASE:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_LABELED;
                s->sc10_labeled_type = STATEMENT_LABELED_CASE;
                safe_lexer_token_next(tok);
                s->sc10_labeled_case_const_expression = maybe_parse_expression(unit, &tok);
                if (!s->sc10_labeled_case_const_expression)
                    dealloc_null_return;
                if (tok->type != LEXER_TOKEN_OPERATOR)
                    dealloc_null_return;
                if (tok->operator_id != ':')
                    dealloc_null_return;
                *toks_ref = tok->next;
                return s;
            }
            case KEYWORD_DEFAULT:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_LABELED;
                s->sc10_labeled_type = STATEMENT_LABELED_DEFAULT;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_OPERATOR)
                    dealloc_null_return;
                if (tok->operator_id != ':')
                    dealloc_null_return;
                *toks_ref = tok->next;
                return s;
            }
            case KEYWORD_IF:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_SELECTION;
                s->sc10_selection_type = STATEMENT_SELECTION_IF;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != '(')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* expr = maybe_parse_expression(unit, &tok);
                if (!expr)
                    dealloc_null_return;
                s->sc10_selection_condition = expr;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ')')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* stmt = maybe_parse_statement(unit, &tok);
                if (!stmt)
                    dealloc_null_return;
                if (tok->type == LEXER_TOKEN_KEYWORD && tok->keyword_id == KEYWORD_ELSE)
                {
                    s->sc10_selection_type = STATEMENT_SELECTION_IF_ELSE;
                    safe_lexer_token_next(tok);
                    syntax_component_t* else_stmt = maybe_parse_statement(unit, &tok);
                    if (!else_stmt)
                        dealloc_null_return;
                    s->sc10_selection_if_else_then = stmt;
                    s->sc10_selection_if_else_else = else_stmt;
                }
                else
                    s->sc10_selection_then = stmt;
                *toks_ref = tok;
                return s;
            }
            case KEYWORD_SWITCH:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_SELECTION;
                s->sc10_selection_type = STATEMENT_SELECTION_SWITCH;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != '(')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* expr = maybe_parse_expression(unit, &tok);
                if (!expr)
                    dealloc_null_return;
                s->sc10_selection_condition = expr;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ')')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* stmt = maybe_parse_statement(unit, &tok);
                if (!stmt)
                    dealloc_null_return;
                s->sc10_selection_switch = stmt;
                *toks_ref = tok;
                return s;
            }
            case KEYWORD_WHILE:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_ITERATION;
                s->sc10_iteration_type = STATEMENT_ITERATION_WHILE;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != '(')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* expr = maybe_parse_expression(unit, &tok);
                if (!expr)
                    dealloc_null_return;
                s->sc10_iteration_while_condition = expr;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ')')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* stmt = maybe_parse_statement(unit, &tok);
                if (!stmt)
                    dealloc_null_return;
                s->sc10_iteration_while_action = stmt;
                *toks_ref = tok;
                return s;
            }
            case KEYWORD_DO:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_ITERATION;
                s->sc10_iteration_type = STATEMENT_ITERATION_DO_WHILE;
                safe_lexer_token_next(tok);
                syntax_component_t* stmt = maybe_parse_statement(unit, &tok);
                if (!stmt)
                    dealloc_null_return;
                s->sc10_iteration_do_while_action = stmt;
                if (tok->type != LEXER_TOKEN_KEYWORD)
                    dealloc_null_return;
                if (tok->keyword_id != KEYWORD_WHILE)
                    dealloc_null_return;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != '(')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* expr = maybe_parse_expression(unit, &tok);
                if (!expr)
                    dealloc_null_return;
                s->sc10_iteration_do_while_condition = expr;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ')')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ';')
                    dealloc_null_return;
                *toks_ref = tok->next;
                return s;
            }
            case KEYWORD_FOR:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_ITERATION;
                s->sc10_iteration_type = STATEMENT_ITERATION_FOR_DECLARATION;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != '(')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* init = maybe_parse_declaration(unit, &tok);
                if (!init)
                {
                    init = maybe_parse_expression(unit, &tok);
                    if (init)
                        s->sc10_iteration_type = STATEMENT_ITERATION_FOR_EXPRESSION;
                }
                s->sc10_iteration_for_init = init;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ';')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                s->sc10_iteration_for_condition = maybe_parse_expression(unit, &tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ';')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                s->sc10_iteration_for_update = maybe_parse_expression(unit, &tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ')')
                    dealloc_null_return;
                safe_lexer_token_next(tok);
                syntax_component_t* stmt = maybe_parse_statement(unit, &tok);
                if (!stmt)
                    dealloc_null_return;
                s->sc10_iteration_for_action = stmt;
                *toks_ref = tok;
                return s;
            }
            case KEYWORD_BREAK:
            case KEYWORD_CONTINUE:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_JUMP;
                s->sc10_jump_type = tok->type == KEYWORD_BREAK ? STATEMENT_JUMP_BREAK : STATEMENT_JUMP_CONTINUE;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ';')
                    dealloc_null_return;
                *toks_ref = tok->next;
                return s;
            }
            case KEYWORD_GOTO:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_JUMP;
                s->sc10_jump_type = STATEMENT_JUMP_GOTO;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_IDENTIFIER)
                    dealloc_null_return;
                s->sc10_jump_goto_identifier = tok->string_value;
                safe_lexer_token_next(tok);
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ';')
                    dealloc_null_return;
                *toks_ref = tok->next;
                return s;
            }
            case KEYWORD_RETURN:
            {
                syntax_component_t* s = calloc(1, sizeof *s);
                s->type = SYNTAX_COMPONENT_STATEMENT;
                s->sc10_type = STATEMENT_JUMP;
                s->sc10_jump_type = STATEMENT_JUMP_RETURN;
                s->sc10_jump_return_expression = NULL;
                safe_lexer_token_next(tok);
                if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == ';')
                {
                    *toks_ref = tok->next;
                    return s;
                }
                s->sc10_jump_return_expression = maybe_parse_expression(unit, &tok);
                if (!s->sc10_jump_return_expression)
                    dealloc_null_return;
                if (tok->type != LEXER_TOKEN_SEPARATOR)
                    dealloc_null_return;
                if (tok->separator_id != ';')
                    dealloc_null_return;
                *toks_ref = tok->next;
                return s;
            }
        }
    }
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_STATEMENT;
    s->sc10_type = STATEMENT_EXPRESSION;
    s->sc10_expression = maybe_parse_expression(unit, &tok);
    if (tok->type != LEXER_TOKEN_SEPARATOR)
        dealloc_null_return;
    if (tok->separator_id != ';')
        dealloc_null_return;
    *toks_ref = tok->next;
    return s;
}

syntax_component_t* maybe_parse_function_definition(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_FUNCTION_DEFINITION;
    lexer_token_t* tok = *toks_ref;

    vector_t* specs_quals = maybe_parse_specifiers_qualifiers(unit, &tok);
    if (!specs_quals)
        dealloc_null_return;
    s->sc9_function_specifiers_qualifiers = specs_quals;

    // do some checking here to ensure there is at least one type specifier
    bool found_type_specifier = false;
    for (unsigned i = 0; i < s->sc9_function_specifiers_qualifiers->size; ++i)
    {
        syntax_component_t* spec_qual = vector_get(s->sc9_function_specifiers_qualifiers, i);
        unsigned type = spec_qual->sc2_type;
        if (type == SPECIFIER_QUALIFIER_VOID ||
            type == SPECIFIER_QUALIFIER_ARITHMETIC_TYPE ||
            type == SPECIFIER_QUALIFIER_TYPEDEF ||
            type == SPECIFIER_QUALIFIER_STRUCT ||
            type == SPECIFIER_QUALIFIER_UNION ||
            type == SPECIFIER_QUALIFIER_ENUM)
        {
            found_type_specifier = true;
            break;
        }
    }
    if (!found_type_specifier)
        dealloc_null_return;

    syntax_component_t* declarator = maybe_parse_declarator(unit, &tok);
    if (declarator->sc3_type != DECLARATOR_FUNCTION) // declarator must be a function declarator (obviously)
        dealloc_null_return;
    s->sc9_function_declarator = declarator;
    syntax_component_t* compound_stmt = maybe_parse_statement(unit, &tok);
    if (!compound_stmt)
        dealloc_null_return;
    if (compound_stmt->sc10_type != STATEMENT_COMPOUND) // function body should be a compound statement
        dealloc_null_return;
    s->sc9_function_body = compound_stmt;
    *toks_ref = tok;
    return s;
}

syntax_component_t* parse(lexer_token_t* toks)
{
    syntax_component_t* unit = calloc(1, sizeof *unit);
    unit->type = SYNTAX_COMPONENT_TRANSLATION_UNIT;
    unit->sc0_external_declarations = vector_init();
    while (toks)
    {
        syntax_component_t* declaration = maybe_parse_declaration(unit, &toks);
        if (declaration)
        {
            vector_add(unit->sc0_external_declarations, declaration);
            continue;
        }
        syntax_component_t* function_definition = maybe_parse_function_definition(unit, &toks);
        if (function_definition)
        {
            vector_add(unit->sc0_external_declarations, function_definition);
            continue;
        }
        parse_terminate(toks, "unknown syntax encountered");
    }
    return unit;
}