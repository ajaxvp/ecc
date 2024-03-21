#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "cc.h"

#define dealloc_null_return { free(s); return NULL; }
#define null_protection(x) if (!(x)) dealloc_null_return
#define safe_lexer_token_next(tok) { tok = tok->next; if (!tok) return NULL; }
#define parse_terminate(tok, fmt, ...) terminate("(line %d, col. %d) " fmt, (tok)->row, (tok)->col, ## __VA_ARGS__)

// here are some helper functions i figured would be useful at some point

// crazy amount of work to find btw
syntax_component_t* find_typedef(syntax_component_t* unit, char* identifier)
{
    for (unsigned i = 0; i < unit->sc0_external_declarations->size; ++i)
    {
        syntax_component_t* decl = vector_get(unit->sc0_external_declarations, i);
        if (decl->type != SYNTAX_COMPONENT_DECLARATION)
            continue;
        bool found_typedef_specifier = false;
        for (unsigned j = 0; j < decl->sc1_specifiers_qualifiers->size; ++j)
        {
            syntax_component_t* specifier = vector_get(decl->sc1_specifiers_qualifiers, j);
            if (specifier->sc2_type != SPECIFIER_QUALIFIER_STORAGE_CLASS)
                continue;
            if (specifier->sc2_storage_class_id == STORAGE_CLASS_TYPEDEF)
            {
                found_typedef_specifier = true;
                break;
            }
        }
        if (!found_typedef_specifier)
            continue;
        for (unsigned j = 0; j < decl->sc1_declarators->size; ++j)
        {
            syntax_component_t* declarator = vector_get(decl->sc1_declarators, j);
            if (declarator->sc3_type == DECLARATOR_IDENTIFIER &&
                !strcmp(declarator->sc3_identifier, identifier))
                return decl;
        }
    }
    return NULL;
}

bool has_specifier_qualifier(syntax_component_t* declaration, unsigned specifier_qualifier_type)
{
    for (unsigned i = 0; i < declaration->sc1_specifiers_qualifiers->size; ++i)
    {
        syntax_component_t* spec_qual = vector_get(declaration->sc1_specifiers_qualifiers, i);
        if (spec_qual->sc2_type == specifier_qualifier_type)
            return true;
    }
    return false;
}

bool has_declarator_type(syntax_component_t* declaration, unsigned declarator_type)
{
    for (unsigned i = 0; i < declaration->sc1_declarators->size; ++i)
    {
        syntax_component_t* declarator = vector_get(declaration->sc1_declarators, i);
        if (declarator->sc3_type == declarator_type)
            return true;
    }
    return false;
}

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

syntax_component_t* maybe_parse_specifier_qualifier(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_SPECIFIER_QUALIFIER;
    null_protection(*toks_ref);

    unsigned keyword_count[KEYWORDS_LEN];
    unsigned non_zero_keyword_counts = 0;

    for (lexer_token_t* tok = *toks_ref; tok; tok = tok->next)
    {
        if (tok->type == LEXER_TOKEN_KEYWORD)
        {
            // update keyword count and update non-zero keyword counts if necessary
            if (!keyword_count[tok->keyword_id])
                ++non_zero_keyword_counts;
            ++(keyword_count[tok->keyword_id]);

            if (keyword_count[KEYWORD_VOID] == 1 && non_zero_keyword_counts == 1)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_VOID;
                *toks_ref = tok->next;
                return s;
            }

            #define arithmetic_body(x) \
            { \
                s->sc2_type = SPECIFIER_QUALIFIER_ARITHMETIC_TYPE; \
                s->sc2_arithmetic_type_id = (x); \
                *toks_ref = tok->next; \
                return s; \
            }

            if (keyword_count[KEYWORD_BOOL] == 1 && non_zero_keyword_counts == 1)
                arithmetic_body(ARITHMETIC_TYPE_BOOL);
            if (keyword_count[KEYWORD_CHAR] == 1 && non_zero_keyword_counts == 1)
                arithmetic_body(ARITHMETIC_TYPE_CHAR);
            if (keyword_count[KEYWORD_CHAR] == 1 && keyword_count[KEYWORD_SIGNED] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_SIGNED_CHAR);
            if (keyword_count[KEYWORD_CHAR] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_CHAR);
            if (keyword_count[KEYWORD_SHORT] == 1 && (non_zero_keyword_counts == 1 ||
                (keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 2) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && non_zero_keyword_counts == 2) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 3)))
                arithmetic_body(ARITHMETIC_TYPE_SHORT_INT);
            if (keyword_count[KEYWORD_SHORT] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && (non_zero_keyword_counts == 2 ||
                (keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 3)))
                arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_SHORT_INT);
            if ((keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 1) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && non_zero_keyword_counts == 1) ||
                (keyword_count[KEYWORD_INT] == 1 && keyword_count[KEYWORD_SIGNED] == 1 && non_zero_keyword_counts == 2))
                arithmetic_body(ARITHMETIC_TYPE_INT);
            if ((keyword_count[KEYWORD_UNSIGNED] == 1 && non_zero_keyword_counts == 1) ||
                (keyword_count[KEYWORD_INT] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && non_zero_keyword_counts == 2))
                arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_INT);
            if (keyword_count[KEYWORD_LONG] == 1 && (non_zero_keyword_counts == 1 ||
                (keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 2) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && non_zero_keyword_counts == 2) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 3)))
                arithmetic_body(ARITHMETIC_TYPE_LONG_INT);
            if (keyword_count[KEYWORD_LONG] == 1 && keyword_count[KEYWORD_UNSIGNED] == 1 && (non_zero_keyword_counts == 2 ||
                (keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 3)))
                arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_LONG_INT);
            if (keyword_count[KEYWORD_LONG] == 2 && (non_zero_keyword_counts == 2 ||
                (keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 3) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && non_zero_keyword_counts == 3) ||
                (keyword_count[KEYWORD_SIGNED] == 1 && keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 4)))
                arithmetic_body(ARITHMETIC_TYPE_LONG_LONG_INT);
            if (keyword_count[KEYWORD_LONG] == 2 && keyword_count[KEYWORD_UNSIGNED] == 1 && (non_zero_keyword_counts == 3 ||
                (keyword_count[KEYWORD_INT] == 1 && non_zero_keyword_counts == 4)))
                arithmetic_body(ARITHMETIC_TYPE_UNSIGNED_LONG_LONG_INT);
            if (keyword_count[KEYWORD_FLOAT] == 1 && non_zero_keyword_counts == 1)
                arithmetic_body(ARITHMETIC_TYPE_FLOAT);
            if (keyword_count[KEYWORD_DOUBLE] == 1 && non_zero_keyword_counts == 1)
                arithmetic_body(ARITHMETIC_TYPE_DOUBLE);
            if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_LONG] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_LONG_DOUBLE);
            if (keyword_count[KEYWORD_FLOAT] == 1 && keyword_count[KEYWORD_COMPLEX] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_FLOAT_COMPLEX);
            if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_COMPLEX] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_DOUBLE_COMPLEX);
            if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_LONG] == 1 && keyword_count[KEYWORD_COMPLEX] == 1 &&
                non_zero_keyword_counts == 3)
                arithmetic_body(ARITHMETIC_TYPE_LONG_DOUBLE_COMPLEX);
            if (keyword_count[KEYWORD_FLOAT] == 1 && keyword_count[KEYWORD_IMAGINARY] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_FLOAT_IMAGINARY);
            if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_IMAGINARY] == 1 && non_zero_keyword_counts == 2)
                arithmetic_body(ARITHMETIC_TYPE_DOUBLE_IMAGINARY);
            if (keyword_count[KEYWORD_DOUBLE] == 1 && keyword_count[KEYWORD_LONG] == 1 && keyword_count[KEYWORD_IMAGINARY] == 1 &&
                non_zero_keyword_counts == 3)
                arithmetic_body(ARITHMETIC_TYPE_LONG_DOUBLE_IMAGINARY);
            #undef arithmetic_body

            #define storage_class_body(x) \
            { \
                s->sc2_type = SPECIFIER_QUALIFIER_STORAGE_CLASS; \
                s->sc2_storage_class_id = (x); \
                *toks_ref = tok->next; \
                return s; \
            }

            if (keyword_count[KEYWORD_TYPEDEF] == 1 && non_zero_keyword_counts == 1)
                storage_class_body(STORAGE_CLASS_TYPEDEF);
            if (keyword_count[KEYWORD_AUTO] == 1 && non_zero_keyword_counts == 1)
                storage_class_body(STORAGE_CLASS_AUTO);
            if (keyword_count[KEYWORD_REGISTER] == 1 && non_zero_keyword_counts == 1)
                storage_class_body(STORAGE_CLASS_REGISTER);
            if (keyword_count[KEYWORD_STATIC] == 1 && non_zero_keyword_counts == 1)
                storage_class_body(STORAGE_CLASS_STATIC);
            if (keyword_count[KEYWORD_EXTERN] == 1 && non_zero_keyword_counts == 1)
                storage_class_body(STORAGE_CLASS_EXTERN);
            #undef storage_class_body

            #define qualifier_body(x) \
            { \
                s->sc2_type = SPECIFIER_QUALIFIER_QUALIFIER; \
                s->sc2_qualifier_id = (x); \
                *toks_ref = tok->next; \
                return s; \
            }

            if (keyword_count[KEYWORD_CONST] == 1 && non_zero_keyword_counts == 1)
                qualifier_body(QUALIFIER_CONST);
            if (keyword_count[KEYWORD_VOLATILE] == 1 && non_zero_keyword_counts == 1)
                qualifier_body(QUALIFIER_VOLATILE);
            if (keyword_count[KEYWORD_RESTRICT] == 1 && non_zero_keyword_counts == 1)
                qualifier_body(QUALIFIER_RESTRICT);
            #undef qualifier_body

            if (keyword_count[KEYWORD_INLINE] == 1 && non_zero_keyword_counts == 1)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_FUNCTION;
                s->sc2_function_id = FUNCTION_SPECIFIER_INLINE;
                *toks_ref = tok->next;
                return s;
            }

            if (keyword_count[KEYWORD_STRUCT] == 1 && non_zero_keyword_counts == 1)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_STRUCT;
                s->sc2_struct = maybe_parse_struct_union(unit, &tok);
                if (!s->sc2_struct)
                    return NULL;
                return s;
            }

            if (keyword_count[KEYWORD_UNION] == 1 && non_zero_keyword_counts == 1)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_UNION;
                s->sc2_union = maybe_parse_struct_union(unit, &tok);
                if (!s->sc2_union)
                    return NULL;
                return s;
            }

            if (keyword_count[KEYWORD_ENUM] == 1 && non_zero_keyword_counts == 1)
            {
                s->sc2_type = SPECIFIER_QUALIFIER_ENUM;
                s->sc2_enum = maybe_parse_enum(unit, &tok);
                if (!s->sc2_enum)
                    return NULL;
                return s;
            }
        }
        else if (tok->type == LEXER_TOKEN_IDENTIFIER)
        {
            syntax_component_t* maybe_typedef = find_typedef(unit, tok->string_value);
            null_protection(maybe_typedef);
            s->sc2_type = SPECIFIER_QUALIFIER_TYPEDEF;
            s->sc2_typedef = maybe_typedef;
            *toks_ref = tok->next;
            return s;
        }
        else
            break;
    }
    dealloc_null_return;
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

syntax_component_t* maybe_parse_declarator(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_DECLARATOR;
    lexer_token_t* tok = *toks_ref;
    if (tok->type == LEXER_TOKEN_IDENTIFIER)
    {
        s->sc3_type = DECLARATOR_IDENTIFIER;
        s->sc3_identifier = tok->string_value;
        *toks_ref = tok->next;
        return s;
    }
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '(')
    {
        safe_lexer_token_next(tok);
        s->sc3_type = DECLARATOR_NEST;
        syntax_component_t* nested_declarator = maybe_parse_declarator(unit, &tok);
        if (!nested_declarator)
            dealloc_null_return;
        s->sc3_nested_declarator = nested_declarator;
        if (tok->type != LEXER_TOKEN_SEPARATOR || tok->separator_id != ')')
            dealloc_null_return;
        *toks_ref = tok->next;
        return s;
    }
    if (tok->type == LEXER_TOKEN_OPERATOR && tok->operator_id == '*')
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
        if (!subdeclarator)
            dealloc_null_return;
        s->sc3_pointer_subdeclarator = subdeclarator;
        *toks_ref = tok;
        return s;
    }
    syntax_component_t* subdeclarator = maybe_parse_declarator(unit, &tok);
    if (!subdeclarator)
        dealloc_null_return;
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '(')
    {
        safe_lexer_token_next(tok);
        s->sc3_type = DECLARATOR_FUNCTION;
        bool old_style = tok->type == LEXER_TOKEN_IDENTIFIER;
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
                syntax_component_t* decl = maybe_parse_declaration(unit, &tok);
                if (!decl)
                    dealloc_null_return;
                vector_add(s->sc3_function_parameters, decl);
            }
            if (tok->separator_id == ',')
                safe_lexer_token_next(tok)
            else if (tok->separator_id != ')')
                dealloc_null_return;
        }
        *toks_ref = tok->next;
        return s;
    }
    else if (tok->type == LEXER_TOKEN_OPERATOR && tok->separator_id == '[')
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
        *toks_ref = tok->next;
        return s;
    }
    dealloc_null_return;
}

syntax_component_t* maybe_parse_initializer_list(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_INITIALIZER;
    lexer_token_t* tok = *toks_ref;

    dealloc_null_return;
}

syntax_component_t* maybe_parse_initializer(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_INITIALIZER;
    lexer_token_t* tok = *toks_ref;
    if (tok->type != LEXER_TOKEN_OPERATOR)
        dealloc_null_return;
    if (tok->operator_id != '=')
        dealloc_null_return;
    safe_lexer_token_next(tok);
    if (tok->type == LEXER_TOKEN_SEPARATOR && tok->separator_id == '{') // initializer list
    {
        s->sc4_type = INITIALIZER_LIST;
        // TODO initializer list stuff
    }
    else // expression
    {
        s->sc4_type = INITIALIZER_EXPRESSION;
        syntax_component_t* expr = maybe_parse_expression(unit, &tok);
        if (!expr)
            dealloc_null_return;
        s->sc4_expression = expr;
    }
    if (tok->type != LEXER_TOKEN_SEPARATOR)
        dealloc_null_return;
    if (tok->separator_id != ';')
        dealloc_null_return;
    *toks_ref = tok->next;
    return s;
}

syntax_component_t* maybe_parse_declaration(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_DECLARATION;
    lexer_token_t* tok = *toks_ref;
    // TODO
    dealloc_null_return;
}

syntax_component_t* maybe_parse_expression(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_EXPRESSION;
    lexer_token_t* tok = *toks_ref;
    // TODO
    dealloc_null_return;
}

syntax_component_t* maybe_parse_statement(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_STATEMENT;
    lexer_token_t* tok = *toks_ref;
    // TODO
    dealloc_null_return;
}

syntax_component_t* maybe_parse_function_definition(syntax_component_t* unit, lexer_token_t** toks_ref)
{
    syntax_component_t* s = calloc(1, sizeof *s);
    s->type = SYNTAX_COMPONENT_FUNCTION_DEFINITION;
    lexer_token_t* tok = *toks_ref;

    syntax_component_t* decl = maybe_parse_declaration(unit, &tok);
    if (!decl)
        dealloc_null_return;
    if (decl->sc1_declarators->size != 1) // function definition may only have one declarator
        dealloc_null_return;
    syntax_component_t* declarator = vector_get(decl->sc1_declarators, 0);
    if (declarator->sc3_type != DECLARATOR_FUNCTION) // declarator must be a function declarator (obviously)
        dealloc_null_return;
    s->sc9_function_declaration = decl;
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