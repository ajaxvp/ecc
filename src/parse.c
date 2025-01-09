#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "cc.h"

#define MAX_ERROR_LEN 4096

#define parse_errorf(row, col, msg) \
    if (row) \
        errorf("[%d:%d] %s\n", row, col, msg); \
    else \
        errorf("[?:?] %s\n", msg);

/*

how the parse_<object> functions work:

the caller passes a reference to a variable to be updated with the position in the token queue
after the call. the token it is initialized with should be the starting position of the token.

the caller passes in a request code, which varies depending on what the caller needs.

CHECK:
    the caller is looking to see if the object is present at the current position.
    the variable passed in "tokens" will not be modified, it will return with ONLY the status code.
    NULL will always be the return value if the request code is CHECK.

    status code:
        FOUND if the object is present
        NOT FOUND if it is not
        ABORT will never occur

OPTIONAL:
    the caller is looking to see if the object is present at the current position.
    if it is, then the variable passed in "tokens" will be updated to the position AFTER the object.
    if its not, the variable will remain the same.
    the object will be the official return value of the function.

    status code:
        FOUND if the object is present
        NOT FOUND if it is not
        ABORT will never occur

EXPECTED:
    the caller is expecting the object is be present at the current position.
    if it is, then the variable passed in "tokens" will be updated to the position AFTER the object.
    if its not, ABORT will be passed in the status code and the return value will be NULL
    the return value, if not ABORTed, will be the object found

    status code:
        FOUND if the object is present
        NOT FOUND will never occur
        ABORT if it is not found, and an error will be printed

*/

// updates the status code, bringing tokens up to speed with token if necessary
#define update_status(x) if (stat) *stat = (x); if ((x) == FOUND) *tokens = token;

#define fail_status update_status(req == EXPECTED ? ABORT : NOT_FOUND)

// creates token
#define init_parse \
    lexer_token_t* token = *tokens;

#define init_syn(t) \
    syntax_component_t* syn = calloc(1, sizeof *syn); \
    syn->type = (t); \
    if (token) syn->row = token->row, syn->col = token->col;

// updates status with fail for the given request and prepares error for printing if needed
// does NOT free anything. you should do this!
#define fail_parse(token, fmt, ...) \
    fail_status; \
    { \
        char buffer[MAX_ERROR_LEN]; \
        snprintf(buffer, MAX_ERROR_LEN, fmt, ## __VA_ARGS__); \
        syntax_component_t* err = calloc(1, sizeof *err); \
        err->type = SC_ERROR; \
        err->err_message = strdup(buffer); \
        err->err_depth = depth; \
        err->row = (token) ? (token)->row : 0; \
        err->col = (token) ? (token)->col : 0; \
        vector_add(tlu->tlu_errors, err); \
    }

// does NOT deallocate syntax elements
#define advance_token \
    if (!token) \
    { \
        fail_parse(token, "unexpected end of file"); \
        return NULL; \
    } \
    token = token->next;

#define next_depth (depth + 1)

// same as link_to_parent, but allows you specify a parent that is not the default 'syn'
#define link_to_specific_parent(s, p) if (s) (s)->parent = (p)

// for filling in the 'parent' field for the child, should be used after a parse_<syntax element> call
#define link_to_parent(s) link_to_specific_parent(s, syn)

// for linking each element of a vector to its parent
#define link_vector_to_parent(v) if (v) { VECTOR_FOR(syntax_component_t*, s, (v)) link_to_specific_parent(s, syn); }

typedef enum parse_status_code
{
    UNKNOWN_STATUS = 1,
    FOUND,
    NOT_FOUND,
    ABORT
} parse_status_code_t;

typedef enum parse_request_code
{
    UNKNOWN_REQUEST = 1,
    CHECK,
    OPTIONAL,
    EXPECTED
} parse_request_code_t;

syntax_component_t* parse_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth);

bool is_keyword(lexer_token_t* token, unsigned keyword_id)
{
    if (!token)
        return false;
    return token->type == LEXER_TOKEN_KEYWORD && token->keyword_id == keyword_id;
}

bool is_operator(lexer_token_t* token, unsigned operator_id)
{
    if (!token)
        return false;
    return token->type == LEXER_TOKEN_OPERATOR && token->operator_id == operator_id;
}

bool is_separator(lexer_token_t* token, unsigned separator_id)
{
    if (!token)
        return false;
    return token->type == LEXER_TOKEN_SEPARATOR && token->separator_id == separator_id;
}

bool has_identifier(lexer_token_t* token, char* id)
{
    if (!token)
        return false;
    return token->type == LEXER_TOKEN_IDENTIFIER && !strcmp(token->string_value, id);
}

syntax_component_t* parse_stub(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    fail_parse(token, "this syntax element cannot be parsed yet");
    return NULL;
}

syntax_component_t* parse_assignment_expression(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    return parse_stub(tokens, req, stat, tlu, depth);
}

// 6.4.2.1
syntax_component_t* parse_identifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_type_t type)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.4.2.1 (1)
        fail_parse(token, "expected identifier, got EOF");
        return NULL;
    }
    init_syn(type);
    if (token->type != LEXER_TOKEN_IDENTIFIER)
    {
        // ISO: 6.4.2.1 (1)
        fail_parse(token, "expected identifier");
        free_syntax(syn);
        return NULL;
    }

    syn->id = strdup(token->string_value);

    advance_token;
    update_status(FOUND);
    return syn;
}

// reminder: define body earlier so that symbols defined in the parameter list are in scope for the function
syntax_component_t* parse_function_definition(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    return parse_stub(tokens, req, stat, tlu, depth);
}

// 6.7.2.1
syntax_component_t* parse_struct_declaration(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_STRUCT_DECLARATION);
    return parse_stub(tokens, req, stat, tlu, depth);
}

// 6.7.2.1
syntax_component_t* parse_struct_or_union_specifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    if (!is_keyword(token, KEYWORD_STRUCT) && !is_keyword(token, KEYWORD_UNION))
    {
        // ISO: 6.7.2.1 (1)
        fail_parse(token, "expected 'struct' or 'union'");
        return NULL;
    }
    init_syn(SC_STRUCT_UNION_SPECIFIER);
    switch (token->keyword_id)
    {
        case KEYWORD_STRUCT: syn->sus_sou = SOU_STRUCT; break;
        case KEYWORD_UNION: syn->sus_sou = SOU_UNION; break;
    }
    advance_token;
    parse_status_code_t id_stat = UNKNOWN_STATUS;
    syn->sus_id = parse_identifier(&token, OPTIONAL, &id_stat, tlu, next_depth, SC_IDENTIFIER);
    if (!is_separator(token, '{'))
    {
        if (id_stat == NOT_FOUND)
        {
            // ISO: 6.7.2.1 (1)
            fail_parse(token, "identifier is required if no declaration list is provided");
            free_syntax(syn);
            return NULL;
        }
        update_status(FOUND);
        return syn;
    }
    advance_token;
    syn->sus_declarations = vector_init();
    if (id_stat == FOUND)
    {
        // add to symbol table //
        symbol_t* sy = symbol_init(syn->sus_id);
        symbol_table_add(tlu->tlu_st, syn->sus_id->id, sy);
    }
    for (;;)
    {
        parse_status_code_t sdecl_stat = UNKNOWN_STATUS;
        syntax_component_t* sdecl = parse_struct_declaration(&token, OPTIONAL, &sdecl_stat, tlu, next_depth);
        if (sdecl_stat == NOT_FOUND)
            break;
        vector_add(syn->sus_declarations, sdecl);
    }
    if (!syn->sus_declarations->size)
    {
        // ISO: 6.7.2.1 (1)
        fail_parse(token, "struct is empty");
        free_syntax(syn);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_enum_specifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    return parse_stub(tokens, req, stat, tlu, depth);
}

// 6.7.1
// to be resolved: none in this function
syntax_component_t* parse_storage_class_specifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.1 (1)
        fail_parse(token, "expected storage class specifier, got EOF");
        return NULL;
    }
    init_syn(SC_STORAGE_CLASS_SPECIFIER);
    bool is = token->type == LEXER_TOKEN_KEYWORD &&
        (token->keyword_id == KEYWORD_AUTO ||
        token->keyword_id == KEYWORD_TYPEDEF ||
        token->keyword_id == KEYWORD_REGISTER ||
        token->keyword_id == KEYWORD_STATIC ||
        token->keyword_id == KEYWORD_EXTERN);
    if (!is)
    {
        // ISO: 6.7.1 (1)
        fail_parse(token, "expected storage class specifier");
        free_syntax(syn);
        return NULL;
    }
    switch (token->keyword_id)
    {
        case KEYWORD_AUTO: syn->scs = SCS_AUTO; break;
        case KEYWORD_TYPEDEF: syn->scs = SCS_TYPEDEF; break;
        case KEYWORD_REGISTER: syn->scs = SCS_REGISTER; break;
        case KEYWORD_STATIC: syn->scs = SCS_STATIC; break;
        case KEYWORD_EXTERN: syn->scs = SCS_EXTERN; break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.2
// to be resolved: 6.7.2 (2), 6.7.2 (3), 6.7.2 (4), 6.7.2 (5)
syntax_component_t* parse_type_specifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.2 (1)
        fail_parse(token, "expected type specifier, got EOF");
        return NULL;
    }
    init_syn(SC_UNKNOWN);
    bool is = token->type == LEXER_TOKEN_KEYWORD &&
        (token->keyword_id == KEYWORD_VOID ||
        token->keyword_id == KEYWORD_CHAR ||
        token->keyword_id == KEYWORD_SHORT ||
        token->keyword_id == KEYWORD_INT ||
        token->keyword_id == KEYWORD_LONG ||
        token->keyword_id == KEYWORD_FLOAT ||
        token->keyword_id == KEYWORD_DOUBLE ||
        token->keyword_id == KEYWORD_SIGNED ||
        token->keyword_id == KEYWORD_UNSIGNED ||
        token->keyword_id == KEYWORD_BOOL ||
        token->keyword_id == KEYWORD_COMPLEX ||
        token->keyword_id == KEYWORD_IMAGINARY);
    if (is)
    {
        syn->type = SC_BASIC_TYPE_SPECIFIER;
        switch (token->keyword_id)
        {
            case KEYWORD_VOID: syn->bts = BTS_VOID; break;
            case KEYWORD_CHAR: syn->bts = BTS_CHAR; break;
            case KEYWORD_SHORT: syn->bts = BTS_SHORT; break;
            case KEYWORD_INT: syn->bts = BTS_INT; break;
            case KEYWORD_LONG: syn->bts = BTS_LONG; break;
            case KEYWORD_FLOAT: syn->bts = BTS_FLOAT; break;
            case KEYWORD_DOUBLE: syn->bts = BTS_DOUBLE; break;
            case KEYWORD_SIGNED: syn->bts = BTS_SIGNED; break;
            case KEYWORD_UNSIGNED: syn->bts = BTS_UNSIGNED; break;
            case KEYWORD_BOOL: syn->bts = BTS_BOOL; break;
            case KEYWORD_COMPLEX: syn->bts = BTS_COMPLEX; break;
            case KEYWORD_IMAGINARY: syn->bts = BTS_IMAGINARY; break;
        }
        advance_token;
        update_status(FOUND);
        return syn;
    }
    free_syntax(syn);
    parse_status_code_t sus_stat = UNKNOWN_STATUS;
    syntax_component_t* sus = parse_struct_or_union_specifier(&token, OPTIONAL, &sus_stat, tlu, next_depth);
    link_to_parent(sus);
    if (sus_stat == FOUND)
    {
        update_status(FOUND);
        return sus;
    }
    parse_status_code_t es_stat = UNKNOWN_STATUS;
    syntax_component_t* es = parse_enum_specifier(&token, OPTIONAL, &es_stat, tlu, next_depth);
    link_to_parent(es);
    if (es_stat == FOUND)
    {
        update_status(FOUND);
        return es;
    }
    parse_status_code_t td_stat = UNKNOWN_STATUS;
    syntax_component_t* td = parse_identifier(&token, OPTIONAL, &td_stat, tlu, next_depth, SC_TYPEDEF_NAME);
    link_to_parent(td);
    if (td_stat == FOUND)
    {
        if (!symbol_table_lookup(tlu->tlu_st, td))
        {
            free_syntax(td);
            fail_parse(token, "could not find the given typedef name");
            return NULL;
        }
        update_status(FOUND);
        return td;
    }
    // ISO: 6.7.2 (1)
    fail_parse(token, "expected type specifier");
    return NULL;
}

// 6.7.3
// to be resolved: 6.7.3 (2), 6.7.3 (3), 6.7.3 (4), 6.7.3 (5), 6.7.3 (6), 6.7.3 (7), 6.7.3 (8), 6.7.3 (9)
syntax_component_t* parse_type_qualifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.3 (1)
        fail_parse(token, "expected type qualifier, got EOF");
        return NULL;
    }
    init_syn(SC_TYPE_QUALIFIER);
    bool is = token->type == LEXER_TOKEN_KEYWORD &&
        (token->keyword_id == KEYWORD_CONST ||
        token->keyword_id == KEYWORD_RESTRICT ||
        token->keyword_id == KEYWORD_VOLATILE);
    if (!is)
    {
        // ISO: 6.7.3 (1)
        fail_parse(token, "expected type qualifier");
        free_syntax(syn);
        return NULL;
    }
    switch (token->keyword_id)
    {
        case KEYWORD_CONST: syn->tq = TQ_CONST; break;
        case KEYWORD_RESTRICT: syn->tq = TQ_RESTRICT; break;
        case KEYWORD_VOLATILE: syn->tq = TQ_VOLATILE; break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.4
// to be resolved: 6.7.4 (2), 6.7.4 (3), 6.7.4 (4), 6.7.4 (5), 6.7.4 (6)
syntax_component_t* parse_function_specifier(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.4 (1)
        fail_parse(token, "expected function specifier, got EOF");
        return NULL;
    }
    init_syn(SC_FUNCTION_SPECIFIER);
    bool is = token->type == LEXER_TOKEN_KEYWORD &&
        (token->keyword_id == KEYWORD_INLINE);
    if (!is)
    {
        // ISO: 6.7.4 (1)
        fail_parse(token, "expected function specifier");
        free_syntax(syn);
        return NULL;
    }
    switch (token->keyword_id)
    {
        case KEYWORD_INLINE: syn->fs = FS_INLINE; break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7
// to be resolved: 
vector_t* parse_declaration_specifiers(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    vector_t* declaration_specifiers = vector_init();
    bool has_scs = false;
    for (;;)
    {
        parse_status_code_t scs_stat = UNKNOWN_STATUS;
        syntax_component_t* scs = parse_storage_class_specifier(&token, OPTIONAL, &scs_stat, tlu, next_depth);
        if (scs_stat == FOUND)
        {
            if (has_scs)
            {
                // ISO: 6.7.1 (2)
                fail_parse(token, "only one storage class specifier allowed in declaration");
                VECTOR_FOR(syntax_component_t*, s, declaration_specifiers)
                    free_syntax(s);
                vector_delete(declaration_specifiers);
                return NULL;
            }
            vector_add(declaration_specifiers, scs);
            has_scs = true;
            continue;
        }
        parse_status_code_t ts_stat = UNKNOWN_STATUS;
        syntax_component_t* ts = parse_type_specifier(&token, OPTIONAL, &ts_stat, tlu, next_depth);
        if (ts_stat == FOUND)
        {
            vector_add(declaration_specifiers, ts);
            continue;
        }
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth);
        if (tq_stat == FOUND)
        {
            vector_add(declaration_specifiers, tq);
            continue;
        }
        parse_status_code_t fs_stat = UNKNOWN_STATUS;
        syntax_component_t* fs = parse_function_specifier(&token, OPTIONAL, &fs_stat, tlu, next_depth);
        if (fs_stat == FOUND)
        {
            vector_add(declaration_specifiers, fs);
            continue;
        }
        break;
    }
    if (!declaration_specifiers->size)
    {
        // ISO: 6.7 (1)
        fail_parse(token, "there must be at least one declaration specifier");
        vector_delete(declaration_specifiers);
        return NULL;
    }
    update_status(FOUND);
    return declaration_specifiers;
}

vector_t* parse_type_qualifier_list(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    vector_t* tqlist = vector_init();
    for (;;)
    {
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth);
        if (tq_stat == NOT_FOUND)
            break;
        vector_add(tqlist, tq);
    }
    if (!tqlist->size)
    {
        fail_parse(token, "expected one or more type qualifiers");
        deep_free_syntax_vector(tqlist, s);
        return NULL;
    }
    update_status(FOUND);
    return tqlist;
}

// 6.7.5
// to be resolved: 
syntax_component_t* parse_pointer(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.5 (1)
        fail_parse(token, "expected pointer, got EOF");
        return NULL;
    }
    init_syn(SC_POINTER);
    if (!is_operator(token, '*'))
    {
        // ISO: 6.7.5 (1)
        fail_parse(token, "expected pointer");
        return NULL;
    }
    advance_token;
    syn->ptr_type_qualifiers = vector_init();
    for (;;)
    {
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth);
        link_to_parent(tq);
        if (tq_stat == NOT_FOUND)
            break;
        vector_add(syn->ptr_type_qualifiers, tq);
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_abstract_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    return parse_stub(tokens, req, stat, tlu, depth);
}

syntax_component_t* parse_parameter_declaration(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_PARAMETER_DECLARATION);
    parse_status_code_t declspecs_stat = UNKNOWN_STATUS;
    syn->pdecl_declaration_specifiers = parse_declaration_specifiers(&token, EXPECTED, &declspecs_stat, tlu, next_depth);
    link_vector_to_parent(syn->pdecl_declaration_specifiers);
    if (declspecs_stat == ABORT)
    {
        fail_status;
        free_syntax(syn);
        return NULL;
    }
    parse_status_code_t declr_stat = UNKNOWN_STATUS;
    syn->pdecl_declr = parse_declarator(&token, OPTIONAL, &declr_stat, tlu, next_depth);
    link_to_parent(syn->pdecl_declr);
    if (declr_stat == FOUND)
    {
        update_status(FOUND);
        return syn;
    }
    declr_stat = UNKNOWN_STATUS;
    syn->pdecl_declr = parse_abstract_declarator(&token, OPTIONAL, &declr_stat, tlu, next_depth);
    link_to_parent(syn->pdecl_declr);
    if (declr_stat == NOT_FOUND)
        syn->pdecl_declr = NULL;
    update_status(FOUND);
    return syn;
}

vector_t* parse_parameter_type_list(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, bool* ellipsis)
{
    init_parse;
    vector_t* ptlist = vector_init();
    *ellipsis = false;
    for (;;)
    {
        parse_status_code_t pdecl_stat = UNKNOWN_STATUS;
        syntax_component_t* pdecl = parse_parameter_declaration(&token, EXPECTED, &pdecl_stat, tlu, next_depth);
        if (pdecl_stat == ABORT)
        {
            fail_status;
            deep_free_syntax_vector(ptlist, s);
            return NULL;
        }
        vector_add(ptlist, pdecl);
        if (!is_separator(token, ','))
            break;
        advance_token;
        if (is_operator(token, '.' * '.' * '.')) // ellipsis
        {
            *ellipsis = true;
            advance_token;
            break;
        }
    }
    update_status(FOUND);
    return ptlist;
}

// partial-array-declarator :=
//     '[' [type-qualifier-list] [assignment-expression] ']'
//     '[' 'static' [type-qualifier-list] assignment-expression ']'
//     '[' type-qualifier-list 'static' assignment-expression ']'
//     '[' [type-qualifier-list] '*' ']'

// doesn't parse the direct declarator
syntax_component_t* parse_partial_array_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_ARRAY_DECLARATOR);

    if (!is_operator(token, '['))
    {
        fail_parse(token, "expected left bracket for array declarator");
        free_syntax(syn);
        return NULL;
    }
    advance_token;
    if (is_keyword(token, KEYWORD_STATIC))
    {
        syn->adeclr_is_static = true;
        advance_token;
    }
    parse_status_code_t tqlist_stat = UNKNOWN_STATUS;
    syn->adeclr_type_qualifiers = parse_type_qualifier_list(&token, OPTIONAL, &tqlist_stat, tlu, next_depth);
    link_vector_to_parent(syn->adeclr_type_qualifiers);
    if (is_operator(token, '*'))
    {
        if (syn->adeclr_is_static)
        {
            fail_parse(token, "array with unspecified size must not have static in the declarator");
            free_syntax(syn);
            return NULL;
        }
        advance_token;
        syn->adeclr_unspecified_size = true;
        if (!is_operator(token, ']'))
        {
            fail_parse(token, "expected right bracket for array declarator");
            free_syntax(syn);
            return NULL;
        }
        advance_token;
        return syn;
    }
    if (is_keyword(token, KEYWORD_STATIC))
    {
        // form '[' type-qualifier-list 'static' assignment-expression ']' requires a type qualifier list
        if (tqlist_stat == NOT_FOUND)
        {
            fail_parse(token, "expected at least one type qualifier");
            free_syntax(syn);
            return NULL;
        }
        syn->adeclr_is_static = true;
        advance_token;
    }
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->adeclr_length_expression = parse_assignment_expression(&token, syn->adeclr_is_static ? EXPECTED : OPTIONAL, &expr_stat, tlu, next_depth);
    link_to_parent(syn->adeclr_length_expression);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn);
        return NULL;
    }
    if (!is_operator(token, ']'))
    {
        fail_parse(token, "expected right bracket for array declarator");
        free_syntax(syn);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// doesn't parse the direct declarator
syntax_component_t* parse_partial_function_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_FUNCTION_DECLARATOR);

    if (!is_separator(token, '('))
    {
        fail_parse(token, "expected left parenthesis for function declarator");
        free_syntax(syn);
        return NULL;
    }
    advance_token;

    parse_status_code_t ptlist_stat = UNKNOWN_STATUS;
    syn->fdeclr_parameter_declarations = parse_parameter_type_list(&token, OPTIONAL, &ptlist_stat, tlu, next_depth, &syn->fdeclr_ellipsis);
    link_vector_to_parent(syn->fdeclr_parameter_declarations);
    if (ptlist_stat == FOUND)
    {
        if (!is_separator(token, ')'))
        {
            fail_parse(token, "expected right parenthesis for function declarator");
            free_syntax(syn);
            return NULL;
        }
        advance_token;
        update_status(FOUND);
        return syn;
    }
    syn->fdeclr_knr_identifiers = vector_init();
    for (;;)
    {
        parse_status_code_t id_stat = UNKNOWN_STATUS;
        syntax_component_t* id = parse_identifier(&token, OPTIONAL, &id_stat, tlu, next_depth, SC_IDENTIFIER);
        // TODO: consider adding to symbol table?
        link_to_parent(id);
        if (id_stat == NOT_FOUND)
            break;
        vector_add(syn->fdeclr_knr_identifiers, id);
        if (!is_separator(token, ','))
            break;
        advance_token;
    }

    if (!is_separator(token, ')'))
    {
        fail_parse(token, "expected right parenthesis for function declarator");
        free_syntax(syn);
        return NULL;
    }
    advance_token;

    update_status(FOUND);
    return syn;
}

// partial-array-declarator :=
//     '[' [type-qualifier-list] [assignment-expression] ']'
//     '[' 'static' [type-qualifier-list] assignment-expression ']'
//     '[' type-qualifier-list 'static' assignment-expression ']'
//     '[' [type-qualifier-list] '*' ']'

// partial-function-declarator :=
//     '(' parameter-type-list ')'
//     '(' [identifier-list] ')'

// direct-declarator :=
//     identifier direct-declarator'
//     '(' declarator ')' direct-declarator'

// direct-declarator' :=
//     partial-array-declarator direct-declarator'
//     partial-function-declarator direct-declarator'
//     e

// SC_DIRECT_DECLARATOR = SC_IDENTIFIER | SC_DECLARATOR | SC_ARRAY_DECLARATOR | SC_FUNCTION_DECLARATOR
// 6.7.5
// to be resolved: 
syntax_component_t* parse_direct_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    // nested declarator
    syntax_component_t* left = NULL;
    if (is_separator(token, '('))
    {
        advance_token;
        parse_status_code_t declr_stat = UNKNOWN_STATUS;
        left = parse_declarator(&token, EXPECTED, &declr_stat, tlu, next_depth);
        if (declr_stat == ABORT)
        {
            fail_parse(token, "expected declarator");
            return NULL;
        }
        if (!is_separator(token, ')'))
        {
            fail_parse(token, "expected right parenthesis");
            free_syntax(left);
            return NULL;
        }
        advance_token;
    }
    else
    {
        parse_status_code_t id_stat = UNKNOWN_STATUS;
        left = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, SC_IDENTIFIER);
        if (id_stat == ABORT)
        {
            fail_parse(token, "expected identifier");
            return NULL;
        }
        // add to symbol table //
        symbol_t* sy = symbol_init(left);
        symbol_table_add(tlu->tlu_st, left->id, sy);
    }
    for (;;)
    {
        parse_status_code_t fdeclr_stat = UNKNOWN_STATUS;
        syntax_component_t* fdeclr = parse_partial_function_declarator(&token, OPTIONAL, &fdeclr_stat, tlu, next_depth);
        if (fdeclr_stat == FOUND)
        {
            link_to_specific_parent(left, fdeclr);
            fdeclr->fdeclr_direct = left;
            left = fdeclr;
            continue;
        }
        parse_status_code_t adeclr_stat = UNKNOWN_STATUS;
        syntax_component_t* adeclr = parse_partial_array_declarator(&token, OPTIONAL, &adeclr_stat, tlu, next_depth);
        if (adeclr_stat == FOUND)
        {
            link_to_specific_parent(left, adeclr);
            adeclr->adeclr_direct = left;
            left = adeclr;
            continue;
        }
        break;
    }
    update_status(FOUND);
    return left;
}

// 6.7.5
// to be resolved: 
syntax_component_t* parse_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_DECLARATOR);
    syn->declr_pointers = vector_init();
    for (;;)
    {
        parse_status_code_t ptr_stat = UNKNOWN_STATUS;
        syntax_component_t* ptr = parse_pointer(&token, OPTIONAL, &ptr_stat, tlu, next_depth);
        link_to_parent(ptr);
        if (ptr_stat == NOT_FOUND)
            break;
        vector_add(syn->declr_pointers, ptr);
    }
    parse_status_code_t dir_stat = UNKNOWN_STATUS;
    syn->declr_direct = parse_direct_declarator(&token, EXPECTED, &dir_stat, tlu, next_depth);
    link_to_parent(syn->declr_direct);
    if (dir_stat == ABORT)
    {
        fail_status;
        free_syntax(syn);
        return NULL;
    }
    if (!syn->declr_pointers->size) // no pointers
    {
        syntax_component_t* direct = syn->declr_direct;
        syn->declr_direct = NULL;
        free_syntax(syn);
        update_status(FOUND);
        return direct;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_initializer(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    return parse_stub(tokens, req, stat, tlu, depth);
}

// 6.7
// to be resolved: 
syntax_component_t* parse_init_declarator(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_INIT_DECLARATOR);
    parse_status_code_t declr_stat = UNKNOWN_STATUS;
    syn->ideclr_declarator = parse_declarator(&token, EXPECTED, &declr_stat, tlu, next_depth);
    link_to_parent(syn->ideclr_declarator);
    if (declr_stat == ABORT)
    {
        // ISO: 6.7 (1)
        fail_status;
        free_syntax(syn);
        return NULL;
    }
    if (is_operator(token, '='))
    {
        advance_token;
        parse_status_code_t init_stat = UNKNOWN_STATUS;
        syn->ideclr_initializer = parse_initializer(&token, EXPECTED, &init_stat, tlu, next_depth);
        link_to_parent(syn->ideclr_initializer);
        if (init_stat == ABORT)
        {
            // ISO: 6.7 (1)
            fail_status;
            free_syntax(syn);
            return NULL;
        }
    }
    update_status(FOUND);
    return syn;
}

// 6.7
// to be resolved: 6.7.1 (5), 6.7.1 (6)
syntax_component_t* parse_declaration(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_DECLARATION);
    parse_status_code_t declspec_stat = UNKNOWN_STATUS;
    syn->decl_declaration_specifiers = parse_declaration_specifiers(&token, EXPECTED, &declspec_stat, tlu, next_depth);
    link_vector_to_parent(syn->decl_declaration_specifiers);
    if (declspec_stat == ABORT)
    {
        // ISO: 6.7 (1)
        fail_status;
        free_syntax(syn);
        return NULL;
    }
    syn->decl_init_declarators = vector_init();
    if (is_separator(token, ';'))
    {
        advance_token;
        update_status(FOUND);
        return syn;
    }
    for (;;)
    {
        parse_status_code_t initdecl_stat = UNKNOWN_STATUS;
        syntax_component_t* initdecl = parse_init_declarator(&token, OPTIONAL, &initdecl_stat, tlu, next_depth);
        link_to_parent(initdecl);
        if (initdecl_stat == NOT_FOUND)
            break;
        vector_add(syn->decl_init_declarators, initdecl);
        if (is_separator(token, ','))
        {
            advance_token;
        }
        else
            break;
    }
    if (!is_separator(token, ';'))
    {
        // ISO: 6.7 (1)
        fail_parse(token, "expected semicolon at the end of a declaration");
        free_syntax(syn);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.9
// to be resolved: 6.9 (3), 6.9 (4), 6.9 (5)
// exception: returns translation unit object even on fail
syntax_component_t* parse_translation_unit(lexer_token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth)
{
    init_parse;
    init_syn(SC_TRANSLATION_UNIT);
    tlu = syn;
    syn->tlu_external_declarations = vector_init();
    syn->tlu_errors = vector_init();
    syn->tlu_st = symbol_table_init();
    while (token)
    {
        parse_status_code_t funcdef_stat = UNKNOWN_STATUS;
        syntax_component_t* funcdef = parse_function_definition(&token, OPTIONAL, &funcdef_stat, tlu, next_depth);
        link_to_parent(funcdef);
        if (funcdef_stat == FOUND)
        {
            vector_add(syn->tlu_external_declarations, funcdef);
            continue;
        }
        parse_status_code_t decl_stat = UNKNOWN_STATUS;
        syntax_component_t* decl = parse_declaration(&token, OPTIONAL, &decl_stat, tlu, next_depth);
        link_to_parent(decl);
        if (decl_stat == FOUND)
        {
            if (syntax_has_specifier(decl->decl_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER, SCS_AUTO) ||
                syntax_has_specifier(decl->decl_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER, SCS_REGISTER))
            {
                // ISO: 6.9 (2)
                fail_parse(decl, "declaration in global scope may not include storage-class specifiers 'auto' or 'register'");
                free_syntax(decl);
                return syn;
            }
            vector_add(syn->tlu_external_declarations, decl);
            continue;
        }
        // ISO: 6.9 (1)
        fail_parse(token, "translation unit cannot be empty");
        return syn;
    }
    update_status(FOUND);
    return syn;
}

// parse a sequence of tokens into a tree.
// takes a linked list of tokens
syntax_component_t* parse(lexer_token_t* tokens)
{
    parse_status_code_t tlu_stat = UNKNOWN_STATUS;
    syntax_component_t* tlu = parse_translation_unit(&tokens, EXPECTED, &tlu_stat, NULL, 1);
    // if it failed, find the deepest error (i.e., the one to give the best description of what went wrong) and print it
    if (tlu_stat == ABORT)
    {
        syntax_component_t* err = NULL;
        VECTOR_FOR(syntax_component_t*, s, tlu->tlu_errors)
            if (!err || s->err_depth > err->err_depth)
                err = s;
        if (err)
        {
            parse_errorf(err->row, err->col, err->err_message);
        }
        free_syntax(tlu);
        tlu = NULL;
    }
    return tlu;
}