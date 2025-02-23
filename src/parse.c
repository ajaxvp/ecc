#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>

#include "ecc.h"

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

RULES TO FOLLOW:
    - a parent calling to parse a child MUST have itself linked to its parent before proceeding
    - if a parse function is called with CHECK, it should do the least possible work in order
      to determine whether an object is present or not.

*/

// updates the status code, bringing tokens up to speed with token if necessary
#define update_status(x) if (stat) *stat = (x); if ((x) == FOUND) *tokens = token;

#define fail_status update_status(req == EXPECTED ? ABORT : NOT_FOUND)

// creates token
#define init_parse \
    token_t* token = *tokens;

#define init_syn(t) \
    syntax_component_t* syn = calloc(1, sizeof *syn); \
    syn->type = (t); \
    if (token) syn->row = token->row, syn->col = token->col; \
    syn->parent = parent;

#define try_parse(type, name, ...) \
    parse_status_code_t name##_stat = UNKNOWN_STATUS; \
    syntax_component_t* name = parse_##type(&token, OPTIONAL, &name##_stat, tlu, next_depth, parent, ## __VA_ARGS__); \
    if (name##_stat == FOUND) \
    { \
        update_status(FOUND); \
        return name; \
    }

// updates status with fail for the given request and prepares error for printing if needed
// does NOT free anything. you should do this!
#define fail_parse(token, fmt, ...) \
    fail_status; \
    if (req == EXPECTED) \
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

typedef syntax_component_t* (*parse_function_t)(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);

syntax_component_t* parse_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_type_specifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_type_qualifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
vector_t* parse_parameter_type_list(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, bool* ellipsis);
syntax_component_t* parse_abstract_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_unary_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_cast_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_conditional_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, syntax_component_type_t type);
syntax_component_t* parse_assignment_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_compound_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent);
syntax_component_t* parse_initializer_list(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, bool permit_trailing_comma);

bool is_keyword(token_t* token, c_keyword_t keyword)
{
    if (!token)
        return false;
    return token->type == T_KEYWORD && token->keyword == keyword;
}

bool is_punctuator(token_t* token, punctuator_type_t punctuator)
{
    if (!token)
        return false;
    return token->type == T_PUNCTUATOR && token->punctuator == punctuator;
}

bool has_identifier(token_t* token, char* id)
{
    if (!token)
        return false;
    return token->type == T_IDENTIFIER && !strcmp(token->identifier, id);
}

syntax_component_t* parse_stub(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    fail_parse(token, "this syntax element cannot be parsed yet");
    return NULL;
}

// 6.4.2.1
syntax_component_t* parse_identifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.4.2.1 (1)
        fail_parse(token, "expected identifier, got EOF");
        return NULL;
    }
    init_syn(SC_IDENTIFIER);
    if (token->type != T_IDENTIFIER)
    {
        // ISO: 6.4.2.1 (1)
        fail_parse(token, "expected identifier");
        free_syntax(syn, tlu);
        return NULL;
    }

    syn->id = strdup(token->identifier);

    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_struct_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_STRUCT_DECLARATOR);
    parse_status_code_t declr_stat = UNKNOWN_STATUS;
    syn->sdeclr_declarator = parse_declarator(&token, OPTIONAL, &declr_stat, tlu, next_depth, syn);
    if (is_punctuator(token, P_COLON))
    {
        advance_token;
        parse_status_code_t ce_stat = UNKNOWN_STATUS;
        syn->sdeclr_bits_expression = parse_conditional_expression(&token, EXPECTED, &ce_stat, tlu, next_depth, syn, SC_CONSTANT_EXPRESSION);
        if (ce_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        update_status(FOUND);
        return syn;
    }
    if (declr_stat == NOT_FOUND)
    {
        fail_parse(token, "declarator required for struct declarator if it does not have a bitfield");
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

// 6.7.2.1
syntax_component_t* parse_struct_declaration(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_STRUCT_DECLARATION);
    syn->sdecl_specifier_qualifier_list = vector_init();
    for (;;)
    {
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth, syn);
        if (tq_stat == FOUND)
        {
            vector_add(syn->sdecl_specifier_qualifier_list, tq);
            continue;
        }
        parse_status_code_t ts_stat = UNKNOWN_STATUS;
        syntax_component_t* ts = parse_type_specifier(&token, OPTIONAL, &ts_stat, tlu, next_depth, syn);
        if (ts_stat == FOUND)
        {
            vector_add(syn->sdecl_specifier_qualifier_list, ts);
            continue;
        }
        break;
    }
    if (!syn->sdecl_specifier_qualifier_list->size)
    {
        // ISO: 6.7.2.1 (1)
        fail_parse(token, "expected at least one type specifier or qualifier for struct declaration");
        free_syntax(syn, tlu);
        return NULL;
    }
    syn->sdecl_declarators = vector_init();
    for (;;)
    {
        parse_status_code_t sdeclr_stat = UNKNOWN_STATUS;
        syntax_component_t* sdeclr = parse_struct_declarator(&token, EXPECTED, &sdeclr_stat, tlu, next_depth, syn);
        if (sdeclr_stat == ABORT)
        {
            // ISO: 6.7.2.1 (1)
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        vector_add(syn->sdecl_declarators, sdeclr);
        if (!is_punctuator(token, P_COMMA))
            break;
        advance_token;
    }
    if (!is_punctuator(token, P_SEMICOLON))
    {
        // ISO: 6.7.2.1 (1)
        fail_parse(token, "expected semicolon at end of struct declaration");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.2.1
syntax_component_t* parse_struct_or_union_specifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!is_keyword(token, KW_STRUCT) && !is_keyword(token, KW_UNION))
    {
        // ISO: 6.7.2.1 (1)
        fail_parse(token, "expected 'struct' or 'union'");
        return NULL;
    }
    init_syn(SC_STRUCT_UNION_SPECIFIER);
    switch (token->keyword)
    {
        case KW_STRUCT: syn->sus_sou = SOU_STRUCT; break;
        case KW_UNION: syn->sus_sou = SOU_UNION; break;
        default: break;
    }
    advance_token;
    parse_status_code_t id_stat = UNKNOWN_STATUS;
    syn->sus_id = parse_identifier(&token, OPTIONAL, &id_stat, tlu, next_depth, syn);
    if (!is_punctuator(token, P_LEFT_BRACE))
    {
        if (id_stat == NOT_FOUND)
        {
            // ISO: 6.7.2.1 (1)
            fail_parse(token, "identifier is required if no declaration list is provided");
            free_syntax(syn, tlu);
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
        syntax_component_t* sdecl = parse_struct_declaration(&token, OPTIONAL, &sdecl_stat, tlu, next_depth, syn);
        if (sdecl_stat == NOT_FOUND)
            break;
        vector_add(syn->sus_declarations, sdecl);
    }
    if (!is_punctuator(token, P_RIGHT_BRACE))
    {
        fail_parse(token, "expected right brace for end of declaration list for struct");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!syn->sus_declarations->size)
    {
        // ISO: 6.7.2.1 (1)
        fail_parse(token, "struct is empty");
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_enumerator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_ENUMERATOR);
    parse_status_code_t id_stat = UNKNOWN_STATUS;
    syn->enumr_constant = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, syn);
    if (id_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    // add to symbol table //
    symbol_t* sy = symbol_init(syn->enumr_constant);
    symbol_table_add(tlu->tlu_st, syn->enumr_constant->id, sy);
    if (!is_punctuator(token, P_ASSIGNMENT))
    {
        update_status(FOUND);
        return syn;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->enumr_expression = parse_conditional_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, syn, SC_CONSTANT_EXPRESSION);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_enum_specifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!is_keyword(token, KW_ENUM))
    {
        // ISO: 6.7.2.2 (1)
        fail_parse(token, "expected 'enum'");
        return NULL;
    }
    init_syn(SC_ENUM_SPECIFIER);
    advance_token;
    parse_status_code_t id_stat = UNKNOWN_STATUS;
    syn->enums_id = parse_identifier(&token, OPTIONAL, &id_stat, tlu, next_depth, syn);
    if (!is_punctuator(token, P_LEFT_BRACE))
    {
        if (id_stat == NOT_FOUND)
        {
            // ISO: 6.7.2.2 (1)
            fail_parse(token, "identifier is required if no enumerator list is provided");
            free_syntax(syn, tlu);
            return NULL;
        }
        update_status(FOUND);
        return syn;
    }
    advance_token;
    syn->enums_enumerators = vector_init();
    if (id_stat == FOUND)
    {
        // add to symbol table //
        symbol_t* sy = symbol_init(syn->enums_id);
        symbol_table_add(tlu->tlu_st, syn->enums_id->id, sy);
    }
    for (;;)
    {
        parse_status_code_t enumr_stat = UNKNOWN_STATUS;
        syntax_component_t* enumr = parse_enumerator(&token, EXPECTED, &enumr_stat, tlu, next_depth, syn);
        if (enumr_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        vector_add(syn->enums_enumerators, enumr);
        if (is_punctuator(token, P_COMMA))
        {
            advance_token;
        }
        if (is_punctuator(token, P_RIGHT_BRACE))
            break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.1
// to be resolved: none in this function
syntax_component_t* parse_storage_class_specifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.1 (1)
        fail_parse(token, "expected storage class specifier, got EOF");
        return NULL;
    }
    init_syn(SC_STORAGE_CLASS_SPECIFIER);
    bool is = token->type == T_KEYWORD &&
        (token->keyword == KW_AUTO ||
        token->keyword == KW_TYPEDEF ||
        token->keyword == KW_REGISTER ||
        token->keyword == KW_STATIC ||
        token->keyword == KW_EXTERN);
    if (!is)
    {
        // ISO: 6.7.1 (1)
        fail_parse(token, "expected storage class specifier");
        free_syntax(syn, tlu);
        return NULL;
    }
    switch (token->keyword)
    {
        case KW_AUTO: syn->scs = SCS_AUTO; break;
        case KW_TYPEDEF: syn->scs = SCS_TYPEDEF; break;
        case KW_REGISTER: syn->scs = SCS_REGISTER; break;
        case KW_STATIC: syn->scs = SCS_STATIC; break;
        case KW_EXTERN: syn->scs = SCS_EXTERN; break;
        default: break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.2
// to be resolved: 6.7.2 (2), 6.7.2 (3), 6.7.2 (4), 6.7.2 (5)
syntax_component_t* parse_type_specifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.2 (1)
        fail_parse(token, "expected type specifier, got EOF");
        return NULL;
    }
    init_syn(SC_UNKNOWN);
    bool is = token->type == T_KEYWORD &&
        (token->keyword == KW_VOID ||
        token->keyword == KW_CHAR ||
        token->keyword == KW_SHORT ||
        token->keyword == KW_INT ||
        token->keyword == KW_LONG ||
        token->keyword == KW_FLOAT ||
        token->keyword == KW_DOUBLE ||
        token->keyword == KW_SIGNED ||
        token->keyword == KW_UNSIGNED ||
        token->keyword == KW_BOOL ||
        token->keyword == KW_COMPLEX ||
        token->keyword == KW_IMAGINARY);
    if (is)
    {
        syn->type = SC_BASIC_TYPE_SPECIFIER;
        switch (token->keyword)
        {
            case KW_VOID: syn->bts = BTS_VOID; break;
            case KW_CHAR: syn->bts = BTS_CHAR; break;
            case KW_SHORT: syn->bts = BTS_SHORT; break;
            case KW_INT: syn->bts = BTS_INT; break;
            case KW_LONG: syn->bts = BTS_LONG; break;
            case KW_FLOAT: syn->bts = BTS_FLOAT; break;
            case KW_DOUBLE: syn->bts = BTS_DOUBLE; break;
            case KW_SIGNED: syn->bts = BTS_SIGNED; break;
            case KW_UNSIGNED: syn->bts = BTS_UNSIGNED; break;
            case KW_BOOL: syn->bts = BTS_BOOL; break;
            case KW_COMPLEX: syn->bts = BTS_COMPLEX; break;
            case KW_IMAGINARY: syn->bts = BTS_IMAGINARY; break;
            default: break;
        }
        link_to_specific_parent(syn, parent);
        advance_token;
        update_status(FOUND);
        return syn;
    }
    free_syntax(syn, tlu);
    parse_status_code_t sus_stat = UNKNOWN_STATUS;
    syntax_component_t* sus = parse_struct_or_union_specifier(&token, OPTIONAL, &sus_stat, tlu, next_depth, parent);
    if (sus_stat == FOUND)
    {
        update_status(FOUND);
        return sus;
    }
    parse_status_code_t es_stat = UNKNOWN_STATUS;
    syntax_component_t* es = parse_enum_specifier(&token, OPTIONAL, &es_stat, tlu, next_depth, parent);
    if (es_stat == FOUND)
    {
        update_status(FOUND);
        return es;
    }
    parse_status_code_t td_stat = UNKNOWN_STATUS;
    syntax_component_t* td = parse_identifier(&token, OPTIONAL, &td_stat, tlu, next_depth, parent);
    if (td_stat == FOUND)
    {
        c_namespace_t ns = get_basic_namespace(NSC_ORDINARY);
        symbol_t* sy;
        if (!(sy = symbol_table_lookup(tlu->tlu_st, td, &ns)))
        {
            free_syntax(td, tlu);
            fail_parse(token, "could not find the given typedef name");
            return NULL;
        }
        if (!syntax_is_typedef_name(sy->declarer))
        {
            free_syntax(td, tlu);
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
syntax_component_t* parse_type_qualifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.3 (1)
        fail_parse(token, "expected type qualifier, got EOF");
        return NULL;
    }
    init_syn(SC_TYPE_QUALIFIER);
    bool is = token->type == T_KEYWORD &&
        (token->keyword == KW_CONST ||
        token->keyword == KW_RESTRICT ||
        token->keyword == KW_VOLATILE);
    if (!is)
    {
        // ISO: 6.7.3 (1)
        fail_parse(token, "expected type qualifier");
        free_syntax(syn, tlu);
        return NULL;
    }
    switch (token->keyword)
    {
        case KW_CONST: syn->tq = TQ_CONST; break;
        case KW_RESTRICT: syn->tq = TQ_RESTRICT; break;
        case KW_VOLATILE: syn->tq = TQ_VOLATILE; break;
        default: break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.4
// to be resolved: 6.7.4 (2), 6.7.4 (3), 6.7.4 (4), 6.7.4 (5), 6.7.4 (6)
syntax_component_t* parse_function_specifier(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.4 (1)
        fail_parse(token, "expected function specifier, got EOF");
        return NULL;
    }
    init_syn(SC_FUNCTION_SPECIFIER);
    bool is = token->type == T_KEYWORD &&
        (token->keyword == KW_INLINE);
    if (!is)
    {
        // ISO: 6.7.4 (1)
        fail_parse(token, "expected function specifier");
        free_syntax(syn, tlu);
        return NULL;
    }
    switch (token->keyword)
    {
        case KW_INLINE: syn->fs = FS_INLINE; break;
        default: break;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7
// to be resolved: 
vector_t* parse_declaration_specifiers(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    vector_t* declaration_specifiers = vector_init();
    for (;;)
    {
        parse_status_code_t scs_stat = UNKNOWN_STATUS;
        syntax_component_t* scs = parse_storage_class_specifier(&token, OPTIONAL, &scs_stat, tlu, next_depth, parent);
        if (scs_stat == FOUND)
        {
            vector_add(declaration_specifiers, scs);
            continue;
        }
        parse_status_code_t ts_stat = UNKNOWN_STATUS;
        syntax_component_t* ts = parse_type_specifier(&token, OPTIONAL, &ts_stat, tlu, next_depth, parent);
        if (ts_stat == FOUND)
        {
            vector_add(declaration_specifiers, ts);
            continue;
        }
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth, parent);
        if (tq_stat == FOUND)
        {
            vector_add(declaration_specifiers, tq);
            continue;
        }
        parse_status_code_t fs_stat = UNKNOWN_STATUS;
        syntax_component_t* fs = parse_function_specifier(&token, OPTIONAL, &fs_stat, tlu, next_depth, parent);
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

vector_t* parse_type_qualifier_list(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    vector_t* tqlist = vector_init();
    for (;;)
    {
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth, parent);
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
syntax_component_t* parse_pointer(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        // ISO: 6.7.5 (1)
        fail_parse(token, "expected pointer, got EOF");
        return NULL;
    }
    init_syn(SC_POINTER);
    if (!is_punctuator(token, P_ASTERISK))
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
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth, syn);
        if (tq_stat == NOT_FOUND)
            break;
        vector_add(syn->ptr_type_qualifiers, tq);
    }
    update_status(FOUND);
    return syn;
}

// does not parse optional direct abstract declarator
syntax_component_t* parse_partial_abstract_function_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_ABSTRACT_FUNCTION_DECLARATOR);

    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for abstract function declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;

    parse_status_code_t ptlist_stat = UNKNOWN_STATUS;
    syn->abfdeclr_parameter_declarations = parse_parameter_type_list(&token, OPTIONAL, &ptlist_stat, tlu, next_depth, syn, &syn->abfdeclr_ellipsis);
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for abstract function declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;

    update_status(FOUND);
    return syn;
}

// does not parse optional direct abstract declarator
syntax_component_t* parse_partial_abstract_array_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_ABSTRACT_ARRAY_DECLARATOR);

    if (!is_punctuator(token, P_LEFT_BRACKET))
    {
        fail_parse(token, "expected left bracket for abstract array declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (is_punctuator(token, P_ASTERISK))
    {
        syn->abadeclr_unspecified_size = true;
        advance_token;
    }
    else
        syn->abadeclr_length_expression = parse_assignment_expression(&token, OPTIONAL, NULL, tlu, next_depth, syn);
    if (!is_punctuator(token, P_RIGHT_BRACKET))
    {
        fail_parse(token, "expected right bracket for abstract array declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// 6.7.6
syntax_component_t* parse_direct_abstract_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    // nested declarator
    syntax_component_t* left = NULL;
    if (is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        advance_token;
        parse_status_code_t abdeclr_stat = UNKNOWN_STATUS;
        left = parse_abstract_declarator(&token, EXPECTED, &abdeclr_stat, tlu, next_depth, parent);
        if (abdeclr_stat == ABORT)
        {
            fail_parse(token, "expected abstract declarator");
            return NULL;
        }
        if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
        {
            fail_parse(token, "expected right parenthesis");
            free_syntax(left, tlu);
            return NULL;
        }
        advance_token;
    }
    for (;;)
    {
        parse_status_code_t abfdeclr_stat = UNKNOWN_STATUS;
        syntax_component_t* abfdeclr = parse_partial_abstract_function_declarator(&token, OPTIONAL, &abfdeclr_stat, tlu, next_depth, parent);
        if (abfdeclr_stat == FOUND)
        {
            if (left)
            {
                link_to_specific_parent(left, abfdeclr);
                abfdeclr->abfdeclr_direct = left;
            }
            left = abfdeclr;
            continue;
        }
        parse_status_code_t abadeclr_stat = UNKNOWN_STATUS;
        syntax_component_t* abadeclr = parse_partial_abstract_array_declarator(&token, OPTIONAL, &abadeclr_stat, tlu, next_depth, parent);
        if (abadeclr_stat == FOUND)
        {
            if (left)
            {
                link_to_specific_parent(left, abadeclr);
                abadeclr->abadeclr_direct = left;
            }
            left = abadeclr;
            continue;
        }
        break;
    }
    update_status(FOUND);
    return left;
}

// 6.7.6
syntax_component_t* parse_abstract_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_ABSTRACT_DECLARATOR);
    syn->abdeclr_pointers = vector_init();
    for (;;)
    {
        parse_status_code_t ptr_stat = UNKNOWN_STATUS;
        syntax_component_t* ptr = parse_pointer(&token, OPTIONAL, &ptr_stat, tlu, next_depth, syn);
        if (ptr_stat == NOT_FOUND)
            break;
        vector_add(syn->abdeclr_pointers, ptr);
    }
    parse_status_code_t dabdeclr_stat = UNKNOWN_STATUS;
    syn->abdeclr_direct = parse_direct_abstract_declarator(&token, OPTIONAL, &dabdeclr_stat, tlu, next_depth, syn);
    if (!syn->abdeclr_pointers->size && dabdeclr_stat == NOT_FOUND)
    {
        // ISO: 6.7.6 (1)
        fail_parse(token, "expected a pointer or a declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!syn->abdeclr_pointers->size) // no pointers
    {
        syntax_component_t* direct = syn->abdeclr_direct;
        syn->abdeclr_direct = NULL;
        free_syntax(syn, tlu);
        update_status(FOUND);
        return direct;
    }
    update_status(FOUND);
    return syn;
}

// 6.7.6
syntax_component_t* parse_type_name(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_TYPE_NAME);
    syn->tn_specifier_qualifier_list = vector_init();
    for (;;)
    {
        parse_status_code_t tq_stat = UNKNOWN_STATUS;
        syntax_component_t* tq = parse_type_qualifier(&token, OPTIONAL, &tq_stat, tlu, next_depth, syn);
        if (tq_stat == FOUND)
        {
            vector_add(syn->sdecl_specifier_qualifier_list, tq);
            continue;
        }
        parse_status_code_t ts_stat = UNKNOWN_STATUS;
        syntax_component_t* ts = parse_type_specifier(&token, OPTIONAL, &ts_stat, tlu, next_depth, syn);
        if (ts_stat == FOUND)
        {
            vector_add(syn->sdecl_specifier_qualifier_list, ts);
            continue;
        }
        break;
    }
    if (!syn->tn_specifier_qualifier_list->size)
    {
        // ISO: 6.7.6 (1)
        fail_parse(token, "expected at least one type specifier or qualifier for struct declaration");
        free_syntax(syn, tlu);
        return NULL;
    }
    parse_status_code_t adeclr_stat = UNKNOWN_STATUS;
    syn->tn_declarator = parse_abstract_declarator(&token, OPTIONAL, &adeclr_stat, tlu, next_depth, syn);
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_parameter_declaration(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_PARAMETER_DECLARATION);
    parse_status_code_t declspecs_stat = UNKNOWN_STATUS;
    syn->pdecl_declaration_specifiers = parse_declaration_specifiers(&token, EXPECTED, &declspecs_stat, tlu, next_depth, syn);
    if (declspecs_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    parse_status_code_t declr_stat = UNKNOWN_STATUS;
    syn->pdecl_declr = parse_declarator(&token, OPTIONAL, &declr_stat, tlu, next_depth, syn);
    if (declr_stat == FOUND)
    {
        update_status(FOUND);
        return syn;
    }
    declr_stat = UNKNOWN_STATUS;
    syn->pdecl_declr = parse_abstract_declarator(&token, OPTIONAL, &declr_stat, tlu, next_depth, syn);
    if (declr_stat == NOT_FOUND)
        syn->pdecl_declr = NULL;
    update_status(FOUND);
    return syn;
}

vector_t* parse_parameter_type_list(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, bool* ellipsis)
{
    init_parse;
    vector_t* ptlist = vector_init();
    *ellipsis = false;
    for (;;)
    {
        parse_status_code_t pdecl_stat = UNKNOWN_STATUS;
        syntax_component_t* pdecl = parse_parameter_declaration(&token, EXPECTED, &pdecl_stat, tlu, next_depth, parent);
        if (pdecl_stat == ABORT)
        {
            fail_status;
            deep_free_syntax_vector(ptlist, s);
            return NULL;
        }
        vector_add(ptlist, pdecl);
        if (!is_punctuator(token, P_COMMA))
            break;
        advance_token;
        if (is_punctuator(token, P_ELLIPSIS)) // ellipsis
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
syntax_component_t* parse_partial_array_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_ARRAY_DECLARATOR);

    if (!is_punctuator(token, P_LEFT_BRACKET))
    {
        fail_parse(token, "expected left bracket for array declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (is_keyword(token, KW_STATIC))
    {
        syn->adeclr_is_static = true;
        advance_token;
    }
    parse_status_code_t tqlist_stat = UNKNOWN_STATUS;
    syn->adeclr_type_qualifiers = parse_type_qualifier_list(&token, OPTIONAL, &tqlist_stat, tlu, next_depth, syn);
    if (is_punctuator(token, P_ASTERISK))
    {
        if (syn->adeclr_is_static)
        {
            fail_parse(token, "array with unspecified size must not have static in the declarator");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
        syn->adeclr_unspecified_size = true;
        if (!is_punctuator(token, P_RIGHT_BRACKET))
        {
            fail_parse(token, "expected right bracket for array declarator");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
        return syn;
    }
    if (is_keyword(token, KW_STATIC))
    {
        // form '[' type-qualifier-list 'static' assignment-expression ']' requires a type qualifier list
        if (tqlist_stat == NOT_FOUND)
        {
            fail_parse(token, "expected at least one type qualifier");
            free_syntax(syn, tlu);
            return NULL;
        }
        syn->adeclr_is_static = true;
        advance_token;
    }
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->adeclr_length_expression = parse_assignment_expression(&token, syn->adeclr_is_static ? EXPECTED : OPTIONAL, &expr_stat, tlu, next_depth, syn);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_BRACKET))
    {
        fail_parse(token, "expected right bracket for array declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// doesn't parse the direct declarator
syntax_component_t* parse_partial_function_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_FUNCTION_DECLARATOR);

    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for function declarator");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;

    parse_status_code_t ptlist_stat = UNKNOWN_STATUS;
    syn->fdeclr_parameter_declarations = parse_parameter_type_list(&token, OPTIONAL, &ptlist_stat, tlu, next_depth, syn, &syn->fdeclr_ellipsis);
    if (ptlist_stat == FOUND)
    {
        if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
        {
            fail_parse(token, "expected right parenthesis for function declarator");
            free_syntax(syn, tlu);
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
        syntax_component_t* id = parse_identifier(&token, OPTIONAL, &id_stat, tlu, next_depth, syn);
        // TODO: consider adding to symbol table?
        if (id_stat == NOT_FOUND)
            break;
        vector_add(syn->fdeclr_knr_identifiers, id);
        if (!is_punctuator(token, P_COMMA))
            break;
        advance_token;
    }

    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for function declarator");
        free_syntax(syn, tlu);
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
//     '(' parameter-type-list P_RIGHT_PARENTHESIS
//     '(' [identifier-list] P_RIGHT_PARENTHESIS

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
syntax_component_t* parse_direct_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    // nested declarator
    syntax_component_t* left = NULL;
    if (is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        advance_token;
        parse_status_code_t declr_stat = UNKNOWN_STATUS;
        left = parse_declarator(&token, EXPECTED, &declr_stat, tlu, next_depth, parent);
        if (declr_stat == ABORT)
        {
            fail_parse(token, "expected declarator");
            return NULL;
        }
        if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
        {
            fail_parse(token, "expected right parenthesis");
            free_syntax(left, tlu);
            return NULL;
        }
        advance_token;
    }
    else
    {
        parse_status_code_t id_stat = UNKNOWN_STATUS;
        left = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, parent);
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
        syntax_component_t* fdeclr = parse_partial_function_declarator(&token, OPTIONAL, &fdeclr_stat, tlu, next_depth, parent);
        if (fdeclr_stat == FOUND)
        {
            link_to_specific_parent(left, fdeclr);
            fdeclr->fdeclr_direct = left;
            left = fdeclr;
            continue;
        }
        parse_status_code_t adeclr_stat = UNKNOWN_STATUS;
        syntax_component_t* adeclr = parse_partial_array_declarator(&token, OPTIONAL, &adeclr_stat, tlu, next_depth, parent);
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
syntax_component_t* parse_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_DECLARATOR);
    syn->declr_pointers = vector_init();
    for (;;)
    {
        parse_status_code_t ptr_stat = UNKNOWN_STATUS;
        syntax_component_t* ptr = parse_pointer(&token, OPTIONAL, &ptr_stat, tlu, next_depth, syn);
        if (ptr_stat == NOT_FOUND)
            break;
        vector_add(syn->declr_pointers, ptr);
    }
    parse_status_code_t dir_stat = UNKNOWN_STATUS;
    syn->declr_direct = parse_direct_declarator(&token, EXPECTED, &dir_stat, tlu, next_depth, syn);
    if (dir_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!syn->declr_pointers->size) // no pointers
    {
        syntax_component_t* direct = syn->declr_direct;
        link_to_specific_parent(direct, parent);
        syn->declr_direct = NULL;
        free_syntax(syn, tlu);
        update_status(FOUND);
        return direct;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_initializer(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_parse(assignment_expression, aexpr)
    if (!is_punctuator(token, P_LEFT_BRACE))
    {
        fail_parse(token, "expected left brace for initializer list");
        return NULL;
    }
    advance_token;
    parse_status_code_t inlist_stat = UNKNOWN_STATUS;
    syntax_component_t* inlist = parse_initializer_list(&token, EXPECTED, &inlist_stat, tlu, next_depth, parent, true);
    if (inlist_stat == ABORT)
    {
        fail_status;
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_BRACE))
    {
        fail_parse(token, "expected right brace for initializer list");
        free_syntax(inlist, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return inlist;
}

// 6.7
// to be resolved: 
syntax_component_t* parse_init_declarator(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_INIT_DECLARATOR);
    parse_status_code_t declr_stat = UNKNOWN_STATUS;
    syn->ideclr_declarator = parse_declarator(&token, EXPECTED, &declr_stat, tlu, next_depth, syn);
    if (declr_stat == ABORT)
    {
        // ISO: 6.7 (1)
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (is_punctuator(token, P_ASSIGNMENT))
    {
        advance_token;
        parse_status_code_t init_stat = UNKNOWN_STATUS;
        syn->ideclr_initializer = parse_initializer(&token, EXPECTED, &init_stat, tlu, next_depth, syn);
        if (init_stat == ABORT)
        {
            // ISO: 6.7 (1)
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
    }
    update_status(FOUND);
    return syn;
}

// 6.7
// to be resolved: 6.7.1 (5), 6.7.1 (6)
syntax_component_t* parse_declaration(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_DECLARATION);
    parse_status_code_t declspec_stat = UNKNOWN_STATUS;
    syn->decl_declaration_specifiers = parse_declaration_specifiers(&token, EXPECTED, &declspec_stat, tlu, next_depth, syn);
    link_vector_to_parent(syn->decl_declaration_specifiers);
    if (declspec_stat == ABORT)
    {
        // ISO: 6.7 (1)
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    syn->decl_init_declarators = vector_init();
    if (is_punctuator(token, P_SEMICOLON))
    {
        advance_token;
        update_status(FOUND);
        return syn;
    }
    for (;;)
    {
        parse_status_code_t initdecl_stat = UNKNOWN_STATUS;
        syntax_component_t* initdecl = parse_init_declarator(&token, OPTIONAL, &initdecl_stat, tlu, next_depth, syn);
        if (initdecl_stat == NOT_FOUND)
            break;
        vector_add(syn->decl_init_declarators, initdecl);
        if (is_punctuator(token, P_COMMA))
        {
            advance_token;
        }
        else
            break;
    }
    if (!is_punctuator(token, P_SEMICOLON))
    {
        // ISO: 6.7 (1)
        fail_parse(token, "expected semicolon at the end of a declaration");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

#define try_expression(type, name) \
    parse_status_code_t name##_stat = UNKNOWN_STATUS; \
    syntax_component_t* name = parse_##type##_expression(&token, OPTIONAL, &name##_stat, tlu, next_depth, parent); \
    if (name##_stat == FOUND) \
    { \
        update_status(FOUND); \
        return name; \
    }

syntax_component_t* parse_floating_constant(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        fail_parse(token, "expected floating constant, got EOF");
        return NULL;
    }
    if (token->type != T_FLOATING_CONSTANT)
    {
        fail_parse(token, "expected floating constant");
        return NULL;
    }
    init_syn(SC_FLOATING_CONSTANT);
    syn->floc = token->floating_constant.value;
    syn->ctype = make_basic_type(token->floating_constant.class);
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_character_constant(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        fail_parse(token, "expected character constant, got EOF");
        return NULL;
    }
    if (token->type != T_CHARACTER_CONSTANT)
    {
        fail_parse(token, "expected character constant");
        return NULL;
    }
    init_syn(SC_CHARACTER_CONSTANT);
    syn->charc_value = token->character_constant.value;
    syn->charc_wide = token->character_constant.wide;
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_integer_constant(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        fail_parse(token, "expected integer constant, got EOF");
        return NULL;
    }
    if (token->type != T_INTEGER_CONSTANT)
    {
        fail_parse(token, "expected integer constant");
        return NULL;
    }
    init_syn(SC_INTEGER_CONSTANT);
    syn->intc = token->integer_constant.value;
    syn->ctype = make_basic_type(token->integer_constant.class);
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_string_literal(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!token)
    {
        fail_parse(token, "expected string literal, got EOF");
        return NULL;
    }
    if (token->type != T_STRING_LITERAL)
    {
        fail_parse(token, "expected string literal");
        return NULL;
    }
    init_syn(SC_STRING_LITERAL);
    if (token->string_literal.value_reg)
        syn->strl_reg = strdup(token->string_literal.value_reg);
    if (token->string_literal.value_wide)
        syn->strl_wide = strdup_wide(token->string_literal.value_wide);
    syn->strl_length = calloc(1, sizeof *syn->strl_length);
    syn->strl_length->type = SC_INTEGER_CONSTANT;
    syn->strl_length->intc = syn->strl_reg ? strlen(syn->strl_reg) : wcslen(syn->strl_wide);
    syn->ctype = make_basic_type(CTC_ARRAY);
    syn->ctype->array.length_expression = syn->strl_length;
    syn->ctype->array.unspecified_size = false;
    syn->ctype->derived_from = make_basic_type(CTC_CHAR);
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_primary_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_parse(identifier, id)
    try_parse(floating_constant, fcon)
    try_parse(character_constant, ccon)
    try_parse(integer_constant, icon)
    try_parse(string_literal, strl)
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected primary expression");
        return NULL;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syntax_component_t* expr = parse_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, parent);
    if (expr_stat == ABORT)
    {
        fail_status;
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis to end nested expression");
        free_syntax(expr, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return expr;
}

syntax_component_t* parse_designation(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_DESIGNATION);
    syn->desig_designators = vector_init();
    for (;;)
    {
        if (is_punctuator(token, P_LEFT_BRACKET))
        {
            advance_token;
            parse_status_code_t cexpr_stat = UNKNOWN_STATUS;
            syntax_component_t* cexpr = parse_conditional_expression(&token, EXPECTED, &cexpr_stat, tlu, next_depth, syn, SC_CONSTANT_EXPRESSION);
            if (cexpr_stat == ABORT)
            {
                fail_status;
                free_syntax(syn, tlu);
                return NULL;
            }
            vector_add(syn->desig_designators, cexpr);
            if (!is_punctuator(token, P_RIGHT_BRACKET))
            {
                fail_parse(token, "expected right bracket for end of designator");
                free_syntax(syn, tlu);
                return NULL;
            }
            advance_token;
            continue;
        }
        if (is_punctuator(token, P_PERIOD))
        {
            advance_token;
            parse_status_code_t id_stat = UNKNOWN_STATUS;
            syntax_component_t* id = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, syn);
            if (id_stat == ABORT)
            {
                fail_status;
                free_syntax(syn, tlu);
                return NULL;
            }
            vector_add(syn->desig_designators, id);
            continue;
        }
        break;
    }
    if (!is_punctuator(token, P_ASSIGNMENT))
    {
        fail_parse(token, "expected assignment operator after designator list");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_initializer_list(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, bool permit_trailing_comma)
{
    init_parse;
    init_syn(SC_INITIALIZER_LIST);
    syn->inlist_designations = vector_init();
    syn->inlist_initializers = vector_init();
    for (;;)
    {
        parse_status_code_t desig_stat = UNKNOWN_STATUS;
        syntax_component_t* desig = parse_designation(&token, OPTIONAL, &desig_stat, tlu, next_depth, syn);
        parse_status_code_t init_stat = UNKNOWN_STATUS;
        syntax_component_t* init = parse_initializer(&token, EXPECTED, &init_stat, tlu, next_depth, syn);
        if (init_stat == ABORT)
        {
            if (permit_trailing_comma && syn->inlist_initializers)
            {
                update_status(FOUND);
                return syn;
            }
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        vector_add(syn->inlist_designations, desig);
        vector_add(syn->inlist_initializers, init);
        if (!is_punctuator(token, P_COMMA))
            break;
        advance_token;
    }
    update_status(FOUND);
    return syn;
}

// does not parse the postfix expression
syntax_component_t* parse_partial_subscript_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!is_punctuator(token, P_LEFT_BRACKET))
    {
        fail_parse(token, "expected left bracket for subscript expression");
        return NULL;
    }
    advance_token;
    init_syn(SC_SUBSCRIPT_EXPRESSION);
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->subsexpr_index_expression = parse_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, syn);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_BRACKET))
    {
        fail_parse(token, "expected right bracket for subscript expression");
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// does not parse the postfix expression
syntax_component_t* parse_partial_function_call_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for function call expression");
        return NULL;
    }
    advance_token;
    init_syn(SC_FUNCTION_CALL_EXPRESSION);
    syn->fcallexpr_args = vector_init();
    for (;;)
    {
        parse_status_code_t expr_stat = UNKNOWN_STATUS;
        syntax_component_t* expr = parse_assignment_expression(&token, OPTIONAL, &expr_stat, tlu, next_depth, syn);
        if (expr_stat == NOT_FOUND)
            break;
        vector_add(syn->fcallexpr_args, expr);
        if (!is_punctuator(token, P_COMMA))
            break;
        advance_token;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for function call expression");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// does not parse the postfix expression
syntax_component_t* parse_partial_member_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, syntax_component_type_t type, unsigned oid)
{
    init_parse;
    if (!is_punctuator(token, oid))
    {
        fail_parse(token, "expected member access operator");
        return NULL;
    }
    advance_token;
    init_syn(type);
    parse_status_code_t id_stat = UNKNOWN_STATUS;
    syn->memexpr_id = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, syn);
    if (id_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

// does not parse the postfix expression
syntax_component_t* parse_partial_postfix_incdec_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, syntax_component_type_t type, unsigned oid)
{
    init_parse;
    if (!is_punctuator(token, oid))
    {
        fail_parse(token, "expected increment/decrement operator");
        return NULL;
    }
    init_syn(type);
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_postfix_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    parse_status_code_t pe_stat = UNKNOWN_STATUS;
    syntax_component_t* left = parse_primary_expression(&token, OPTIONAL, &pe_stat, tlu, next_depth, parent);
    if (pe_stat == NOT_FOUND)
    {
        init_syn(SC_COMPOUND_LITERAL);
        if (!is_punctuator(token, P_LEFT_PARENTHESIS))
        {
            fail_parse(token, "expected left parenthesis for compound literal type");
            return NULL;
        }
        advance_token;
        parse_status_code_t tn_stat = UNKNOWN_STATUS;
        syn->cl_type_name = parse_type_name(&token, EXPECTED, &tn_stat, tlu, next_depth, syn);
        if (tn_stat == ABORT)
        {
            fail_status;
            return NULL;
        }
        if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
        {
            fail_parse(token, "expected right parenthesis for compound literal type");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
        if (!is_punctuator(token, P_LEFT_BRACE))
        {
            fail_parse(token, "expected left brace for compound literal initializer list");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
        parse_status_code_t inlist_stat = UNKNOWN_STATUS;
        syn->cl_inlist = parse_initializer_list(&token, EXPECTED, &inlist_stat, tlu, next_depth, syn, true);
        if (inlist_stat == ABORT)
        {
            fail_status;
            return NULL;
        }
        if (!is_punctuator(token, P_RIGHT_BRACE))
        {
            fail_parse(token, "expected right brace for compound literal initializer list");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
        left = syn;
    }
    for (;;)
    {
        parse_status_code_t subsexpr_stat = UNKNOWN_STATUS;
        syntax_component_t* subsexpr = parse_partial_subscript_expression(&token, OPTIONAL, &subsexpr_stat, tlu, next_depth, parent);
        if (subsexpr_stat == FOUND)
        {
            link_to_specific_parent(left, subsexpr);
            subsexpr->subsexpr_expression = left;
            left = subsexpr;
            continue;
        }
        parse_status_code_t fcall_stat = UNKNOWN_STATUS;
        syntax_component_t* fcall = parse_partial_function_call_expression(&token, OPTIONAL, &fcall_stat, tlu, next_depth, parent);
        if (fcall_stat == FOUND)
        {
            link_to_specific_parent(left, fcall);
            fcall->fcallexpr_expression = left;
            left = fcall;
            continue;
        }
        parse_status_code_t mem_stat = UNKNOWN_STATUS;
        syntax_component_t* mem = parse_partial_member_expression(&token, OPTIONAL, &mem_stat, tlu, next_depth, parent, SC_MEMBER_EXPRESSION, P_PERIOD);
        if (mem_stat == FOUND)
        {
            link_to_specific_parent(left, mem);
            mem->memexpr_expression = left;
            left = mem;
            continue;
        }
        parse_status_code_t dmem_stat = UNKNOWN_STATUS;
        syntax_component_t* dmem = parse_partial_member_expression(&token, OPTIONAL, &dmem_stat, tlu, next_depth, parent, SC_DEREFERENCE_MEMBER_EXPRESSION, P_DEREFERENCE_MEMBER);
        if (dmem_stat == FOUND)
        {
            link_to_specific_parent(left, dmem);
            dmem->memexpr_expression = left;
            left = dmem;
            continue;
        }
        parse_status_code_t inc_stat = UNKNOWN_STATUS;
        syntax_component_t* inc = parse_partial_postfix_incdec_expression(&token, OPTIONAL, &inc_stat, tlu, next_depth, parent, SC_POSTFIX_INCREMENT_EXPRESSION, P_INCREMENT);
        if (inc_stat == FOUND)
        {
            link_to_specific_parent(left, inc);
            inc->uexpr_operand = left;
            left = inc;
            continue;
        }
        parse_status_code_t dec_stat = UNKNOWN_STATUS;
        syntax_component_t* dec = parse_partial_postfix_incdec_expression(&token, OPTIONAL, &dec_stat, tlu, next_depth, parent, SC_POSTFIX_DECREMENT_EXPRESSION, P_DECREMENT);
        if (dec_stat == FOUND)
        {
            link_to_specific_parent(left, dec);
            dec->uexpr_operand = left;
            left = dec;
            continue;
        }
        break;
    }
    update_status(FOUND);
    return left;
}

syntax_component_t* parse_left_unary_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, syntax_component_type_t type, unsigned oid, parse_function_t subparser)
{
    init_parse;
    if (!is_punctuator(token, oid))
    {
        fail_parse(token, "expected operator for unary expression");
        return NULL;
    }
    init_syn(type);
    advance_token;
    parse_status_code_t s_stat = UNKNOWN_STATUS;
    syn->uexpr_operand = subparser(&token, EXPECTED, &s_stat, tlu, next_depth, syn);
    if (s_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_prefix_increment_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_PREFIX_INCREMENT_EXPRESSION, P_INCREMENT, parse_unary_expression);
}

syntax_component_t* parse_prefix_decrement_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_PREFIX_DECREMENT_EXPRESSION, P_DECREMENT, parse_unary_expression);
}

syntax_component_t* parse_reference_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_REFERENCE_EXPRESSION, P_AND, parse_cast_expression);
}

syntax_component_t* parse_dereference_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_DEREFERENCE_EXPRESSION, P_ASTERISK, parse_cast_expression);
}

syntax_component_t* parse_plus_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_PLUS_EXPRESSION, P_PLUS, parse_cast_expression);
}

syntax_component_t* parse_minus_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_MINUS_EXPRESSION, P_MINUS, parse_cast_expression);
}

syntax_component_t* parse_complement_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_COMPLEMENT_EXPRESSION, P_TILDE, parse_cast_expression);
}

syntax_component_t* parse_not_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    return parse_left_unary_expression(tokens, req, stat, tlu, depth, parent, SC_NOT_EXPRESSION, P_EXCLAMATION_POINT, parse_cast_expression);
}

syntax_component_t* parse_sizeof_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!is_keyword(token, KW_SIZEOF))
    {
        fail_parse(token, "expected sizeof operator in sizeof expression");
        return NULL;
    }
    init_syn(SC_SIZEOF_EXPRESSION);
    advance_token;
    parse_status_code_t s_stat = UNKNOWN_STATUS;
    syn->uexpr_operand = parse_unary_expression(&token, EXPECTED, &s_stat, tlu, next_depth, syn);
    if (s_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_sizeof_type_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    if (!is_keyword(token, KW_SIZEOF))
    {
        fail_parse(token, "expected sizeof operator in sizeof expression");
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for sizeof operator");
        return NULL;
    }
    advance_token;
    init_syn(SC_SIZEOF_TYPE_EXPRESSION);
    parse_status_code_t tn_stat = UNKNOWN_STATUS;
    syn->uexpr_operand = parse_type_name(&token, EXPECTED, &tn_stat, tlu, next_depth, syn);
    if (tn_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for sizeof operator");
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_unary_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_expression(postfix, pfexpr)
    try_expression(prefix_increment, piexpr)
    try_expression(prefix_decrement, pdexpr)
    try_expression(reference, drexpr)
    try_expression(dereference, rexpr)
    try_expression(plus, pexpr)
    try_expression(minus, mexpr)
    try_expression(complement, cexpr)
    try_expression(not, nexpr)
    try_expression(sizeof, soexpr)
    try_expression(sizeof_type, sotexpr)
    fail_parse(token, "expected unary expression");
    return NULL;
}

syntax_component_t* parse_cast_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    parse_status_code_t uexpr_stat = UNKNOWN_STATUS;
    syntax_component_t* uexpr = parse_unary_expression(&token, OPTIONAL, &uexpr_stat, tlu, next_depth, parent);
    if (uexpr_stat == FOUND)
    {
        update_status(FOUND);
        return uexpr;
    }
    init_syn(SC_CAST_EXPRESSION);
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for cast");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t tn_stat = UNKNOWN_STATUS;
    syn->caexpr_type_name = parse_type_name(&token, EXPECTED, &tn_stat, tlu, next_depth, syn);
    if (tn_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for cast");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t cexpr_stat = UNKNOWN_STATUS;
    syn->caexpr_operand = parse_cast_expression(&token, EXPECTED, &cexpr_stat, tlu, next_depth, syn);
    if (cexpr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

/*

logical-or-expression :=
    logical-and-expression logical-or-expression'

logical-or-expression' :=
    || logical-and-expression logical-or-expression'
    e

*/

// operator_name: the operator(s) to define a parse function for
// suboperator_name: the type of operator which has expression which composes these expressions
// otype1-4: the syntax element types corresponding to the operators to handle here
// op1-4: the operators corresponding to otype1-4
#define define_basic_parse_expression(operator_name, suboperator_name, otype1, otype2, otype3, otype4, op1, op2, op3, op4) \
    syntax_component_t* parse_##operator_name##_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent) \
    { \
        init_parse; \
        parse_status_code_t lhs_stat = UNKNOWN_STATUS; \
        syntax_component_t* lhs = parse_##suboperator_name##_expression(&token, EXPECTED, &lhs_stat, tlu, next_depth, parent); \
        if (lhs_stat == ABORT) \
        { \
            fail_status; \
            return NULL; \
        } \
        for (;;) \
        { \
            if (!is_punctuator(token, (op1)) && \
                !is_punctuator(token, (op2)) && \
                !is_punctuator(token, (op3)) && \
                !is_punctuator(token, (op4))) \
                break; \
            punctuator_type_t op = token->punctuator; \
            advance_token; \
            parse_status_code_t rhs_stat = UNKNOWN_STATUS; \
            syntax_component_t* rhs = parse_##suboperator_name##_expression(&token, EXPECTED, &rhs_stat, tlu, next_depth, parent); \
            if (rhs_stat == ABORT) \
            { \
                fail_status; \
                free_syntax(lhs, tlu); \
                return NULL; \
            } \
            syntax_component_type_t otype = SC_UNKNOWN; \
            if (op == (op1)) otype = (otype1); \
            if (op == (op2)) otype = (otype2); \
            if (op == (op3)) otype = (otype3); \
            if (op == (op4)) otype = (otype4); \
            init_syn(otype); \
            syn->bexpr_lhs = lhs; \
            link_to_specific_parent(syn->bexpr_lhs, syn); \
            syn->bexpr_rhs = rhs; \
            link_to_specific_parent(syn->bexpr_rhs, syn); \
            lhs = syn; \
        } \
        update_status(FOUND); \
        return lhs; \
    }

define_basic_parse_expression(
    multiplicative,
    cast,
    SC_MULTIPLICATION_EXPRESSION, SC_DIVISION_EXPRESSION, SC_MODULAR_EXPRESSION, SC_UNKNOWN,
    P_ASTERISK, P_SLASH, P_PERCENT, 0
)
define_basic_parse_expression(
    additive,
    multiplicative,
    SC_ADDITION_EXPRESSION, SC_SUBTRACTION_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN,
    P_PLUS, P_MINUS, 0, 0
)
define_basic_parse_expression(
    shift,
    additive,
    SC_BITWISE_LEFT_EXPRESSION, SC_BITWISE_RIGHT_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN,
    P_LEFT_SHIFT, P_RIGHT_SHIFT, 0, 0
)
define_basic_parse_expression(
    relational,
    shift,
    SC_LESS_EXPRESSION, SC_GREATER_EXPRESSION, SC_LESS_EQUAL_EXPRESSION, SC_GREATER_EQUAL_EXPRESSION,
    P_LESS, P_GREATER, P_LESS_EQUAL, P_GREATER_EQUAL
)
define_basic_parse_expression(
    equality,
    relational,
    SC_EQUALITY_EXPRESSION, SC_INEQUALITY_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN,
    P_EQUAL, P_INEQUAL, 0, 0
);
define_basic_parse_expression(
    bitwise_and,
    equality,
    SC_BITWISE_AND_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN, SC_UNKNOWN,
    P_AND, 0, 0, 0
)
define_basic_parse_expression(
    bitwise_xor,
    bitwise_and,
    SC_BITWISE_XOR_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN, SC_UNKNOWN,
    P_CARET, 0, 0, 0
)
define_basic_parse_expression(
    bitwise_or,
    bitwise_xor, 
    SC_BITWISE_OR_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN, SC_UNKNOWN,
    P_PIPE, 0, 0, 0
)
define_basic_parse_expression(
    logical_and,
    bitwise_or,
    SC_LOGICAL_AND_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN, SC_UNKNOWN,
    P_LOGICAL_AND, 0, 0, 0
)
define_basic_parse_expression(
    logical_or,
    logical_and,
    SC_LOGICAL_OR_EXPRESSION, SC_UNKNOWN, SC_UNKNOWN, SC_UNKNOWN,
    P_LOGICAL_OR, 0, 0, 0
)

syntax_component_t* parse_conditional_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent, syntax_component_type_t type)
{
    init_parse;
    init_syn(type);

    parse_status_code_t loexpr_stat = UNKNOWN_STATUS;
    syn->cexpr_condition = parse_logical_or_expression(&token, EXPECTED, &loexpr_stat, tlu, next_depth, syn);
    if (loexpr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (is_punctuator(token, P_QUESTION_MARK))
    {
        advance_token;
        parse_status_code_t if_stat = UNKNOWN_STATUS;
        syn->cexpr_if = parse_expression(&token, EXPECTED, &if_stat, tlu, next_depth, syn);
        if (if_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        if (!is_punctuator(token, P_COLON))
        {
            fail_parse(token, "expected colon for separating resulting expressions in conditional");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
        parse_status_code_t else_stat = UNKNOWN_STATUS;
        syn->cexpr_else = parse_conditional_expression(&token, EXPECTED, &else_stat, tlu, next_depth, syn, SC_CONDITIONAL_EXPRESSION);
        if (else_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
    }
    else
    {
        syntax_component_t* loexpr = syn->cexpr_condition;
        link_to_specific_parent(loexpr, parent);
        syn->cexpr_condition = NULL;
        free_syntax(syn, tlu);
        syn = loexpr;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_actual_assignment_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_UNKNOWN);
    parse_status_code_t uexpr_stat = UNKNOWN_STATUS;
    syn->bexpr_lhs = parse_unary_expression(&token, EXPECTED, &uexpr_stat, tlu, next_depth, syn);
    if (uexpr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!token || token->type != T_PUNCTUATOR)
    {
        fail_parse(token, "expected an assignment operator for assignment expression");
        free_syntax(syn, tlu);
        return NULL;
    }
    switch (token->punctuator)
    {
        case P_ASSIGNMENT: syn->type = SC_ASSIGNMENT_EXPRESSION; break;
        case P_MULTIPLY_ASSIGNMENT: syn->type = SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION; break;
        case P_DIVIDE_ASSIGNMENT: syn->type = SC_DIVISION_ASSIGNMENT_EXPRESSION; break;
        case P_MODULO_ASSIGNMENT: syn->type = SC_MODULAR_ASSIGNMENT_EXPRESSION; break;
        case P_ADD_ASSIGNMENT: syn->type = SC_ADDITION_ASSIGNMENT_EXPRESSION; break;
        case P_SUB_ASSIGNMENT: syn->type = SC_SUBTRACTION_ASSIGNMENT_EXPRESSION; break;
        case P_SHIFT_LEFT_ASSIGNMENT: syn->type = SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION; break;
        case P_SHIFT_RIGHT_ASSIGNMENT: syn->type = SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION; break;
        case P_BITWISE_AND_ASSIGNMENT: syn->type = SC_BITWISE_AND_ASSIGNMENT_EXPRESSION; break;
        case P_BITWISE_XOR_ASSIGNMENT: syn->type = SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION; break;
        case P_BITWISE_OR_ASSIGNMENT: syn->type = SC_BITWISE_OR_ASSIGNMENT_EXPRESSION; break;
        default:
        {
            fail_parse(token, "expected an assignment operator for assignment expression");
            free_syntax(syn, tlu);
            return NULL;
        }
    }
    advance_token;
    parse_status_code_t aexpr_stat = UNKNOWN_STATUS;
    syn->bexpr_rhs = parse_assignment_expression(&token, EXPECTED, &aexpr_stat, tlu, next_depth, syn);
    if (aexpr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_assignment_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_expression(actual_assignment, aexo);
    try_parse(conditional_expression, cexpr, SC_CONDITIONAL_EXPRESSION);
    fail_parse(token, "expected expression");
    return NULL;
}

syntax_component_t* parse_expression(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_EXPRESSION);
    syn->expr_expressions = vector_init();
    for (;;)
    {
        parse_status_code_t aexpr_stat = UNKNOWN_STATUS;
        syntax_component_t* aexpr = parse_assignment_expression(&token, EXPECTED, &aexpr_stat, tlu, next_depth, syn);
        if (aexpr_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        vector_add(syn->expr_expressions, aexpr);
        if (!is_punctuator(token, P_COMMA))
            break;
        advance_token;
    }
    update_status(FOUND);
    return syn;
}

#undef try_expression

#define try_statement(type, name) \
    parse_status_code_t name##_stat = UNKNOWN_STATUS; \
    syntax_component_t* name = parse_##type##_statement(&token, OPTIONAL, &name##_stat, tlu, next_depth, parent); \
    if (name##_stat == FOUND) \
    { \
        update_status(FOUND); \
        return name; \
    }

syntax_component_t* parse_expression_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_EXPRESSION_STATEMENT);
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->estmt_expression = parse_expression(&token, OPTIONAL, &expr_stat, tlu, next_depth, syn);
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon for expression statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_goto_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_GOTO_STATEMENT);
    if (!is_keyword(token, KW_GOTO))
    {
        fail_parse(token, "expected 'goto' for goto statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t id_stat = UNKNOWN_STATUS;
    syn->gtstmt_label_id = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, syn);
    if (id_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon for goto statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_continue_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_CONTINUE_STATEMENT);
    if (!is_keyword(token, KW_CONTINUE))
    {
        fail_parse(token, "expected 'continue' for continue statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon for continue statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_break_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_BREAK_STATEMENT);
    if (!is_keyword(token, KW_BREAK))
    {
        fail_parse(token, "expected 'break' for break statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon for break statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_return_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_RETURN_STATEMENT);
    if (!is_keyword(token, KW_RETURN))
    {
        fail_parse(token, "expected 'return' for return statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->retstmt_expression = parse_expression(&token, OPTIONAL, &expr_stat, tlu, next_depth, syn);
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon for return statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_jump_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_statement(goto, gstmt)
    try_statement(continue, cstmt)
    try_statement(break, bstmt)
    try_statement(return, rstmt)
    fail_parse(token, "expected jump statement");
    return NULL;
}

syntax_component_t* parse_for_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_FOR_STATEMENT);
    if (!is_keyword(token, KW_FOR))
    {
        fail_parse(token, "expected 'for' for for statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for for statement header");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t init_stat = UNKNOWN_STATUS;
    syn->forstmt_init = parse_declaration(&token, OPTIONAL, &init_stat, tlu, next_depth, syn);
    if (init_stat == NOT_FOUND)
    {
        init_stat = UNKNOWN_STATUS;
        syn->forstmt_init = parse_expression(&token, OPTIONAL, &init_stat, tlu, next_depth, syn);
        if (!is_punctuator(token, P_SEMICOLON))
        {
            fail_parse(token, "expected semicolon after initializing clause in for statement");
            free_syntax(syn, tlu);
            return NULL;
        }
        advance_token;
    }
    parse_status_code_t cond_stat = UNKNOWN_STATUS;
    syn->forstmt_condition = parse_expression(&token, OPTIONAL, &cond_stat, tlu, next_depth, syn);
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon after condition expression in for statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t post_stat = UNKNOWN_STATUS;
    syn->forstmt_post = parse_expression(&token, OPTIONAL, &post_stat, tlu, next_depth, syn);
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for for statement header");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t body_stat = UNKNOWN_STATUS;
    syn->forstmt_body = parse_statement(&token, EXPECTED, &body_stat, tlu, next_depth, syn);
    if (body_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_do_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_DO_STATEMENT);
    if (!is_keyword(token, KW_DO))
    {
        fail_parse(token, "expected 'do' for do-while statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t body_stat = UNKNOWN_STATUS;
    syn->dostmt_body = parse_statement(&token, EXPECTED, &body_stat, tlu, next_depth, syn);
    if (body_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_keyword(token, KW_WHILE))
    {
        fail_parse(token, "expected 'while' for do-while statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for do-while statement condition");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->dostmt_condition = parse_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, syn);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for do-while statement condition");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_SEMICOLON))
    {
        fail_parse(token, "expected semicolon for do-while statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_while_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_WHILE_STATEMENT);
    if (!is_keyword(token, KW_WHILE))
    {
        fail_parse(token, "expected 'while' for while statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for while statement condition");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->whstmt_condition = parse_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, syn);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for while statement condition");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t body_stat = UNKNOWN_STATUS;
    syn->whstmt_body = parse_statement(&token, EXPECTED, &body_stat, tlu, next_depth, syn);
    if (body_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_iteration_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_statement(for, fstmt)
    try_statement(do, dstmt)
    try_statement(while, wstmt)
    fail_parse(token, "expected iteration statement");
    return NULL;
}

syntax_component_t* parse_if_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_IF_STATEMENT);
    if (!is_keyword(token, KW_IF))
    {
        fail_parse(token, "expected 'if' for if statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for if statement condition");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->ifstmt_condition = parse_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, syn);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for if statement condition");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t body_stat = UNKNOWN_STATUS;
    syn->ifstmt_body = parse_statement(&token, EXPECTED, &body_stat, tlu, next_depth, syn);
    if (body_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (is_keyword(token, KW_ELSE))
    {
        advance_token;
        parse_status_code_t else_stat = UNKNOWN_STATUS;
        syn->ifstmt_else = parse_statement(&token, EXPECTED, &else_stat, tlu, next_depth, syn);
        if (else_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_switch_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_SWITCH_STATEMENT);
    if (!is_keyword(token, KW_SWITCH))
    {
        fail_parse(token, "expected 'switch' for switch statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    if (!is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        fail_parse(token, "expected left parenthesis for switch statement expression");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t expr_stat = UNKNOWN_STATUS;
    syn->swstmt_condition = parse_expression(&token, EXPECTED, &expr_stat, tlu, next_depth, syn);
    if (expr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
    {
        fail_parse(token, "expected right parenthesis for switch statement expression");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t body_stat = UNKNOWN_STATUS;
    syn->swstmt_body = parse_statement(&token, EXPECTED, &body_stat, tlu, next_depth, syn);
    if (body_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_selection_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_statement(if, istmt);
    try_statement(switch, stmt);
    fail_parse(token, "expected selection statement");
    return NULL;
}

syntax_component_t* parse_labeled_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_LABELED_STATEMENT);
    if (is_keyword(token, KW_CASE))
    {
        advance_token;
        parse_status_code_t cexpr_stat = UNKNOWN_STATUS;
        syn->lstmt_case_expression = parse_conditional_expression(&token, EXPECTED, &cexpr_stat, tlu, next_depth, syn, SC_CONSTANT_EXPRESSION);
        if (cexpr_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
    }
    else if (is_keyword(token, KW_DEFAULT))
    {
        advance_token;
    }
    else
    {
        parse_status_code_t id_stat = UNKNOWN_STATUS;
        syn->lstmt_id = parse_identifier(&token, EXPECTED, &id_stat, tlu, next_depth, syn);
        if (id_stat == ABORT)
        {
            fail_status;
            free_syntax(syn, tlu);
            return NULL;
        }
        // add to symbol table //
        symbol_t* sy = symbol_init(syn->lstmt_id);
        symbol_table_add(tlu->tlu_st, syn->lstmt_id->id, sy);
    }
    if (!is_punctuator(token, P_COLON))
    {
        fail_parse(token, "expected colon after label statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    parse_status_code_t stmt_stat = UNKNOWN_STATUS;
    syn->lstmt_stmt = parse_statement(&token, EXPECTED, &stmt_stat, tlu, next_depth, syn);
    if (stmt_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

syntax_component_t* parse_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    try_statement(labeled, lstmt)
    try_statement(expression, exstmt)
    try_statement(compound, cstmt)
    try_statement(selection, sstmt)
    try_statement(iteration, istmt)
    try_statement(jump, jstmt)
    fail_parse(token, "expected statement");
    return NULL;
}

#undef try_statement

syntax_component_t* parse_compound_statement(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_COMPOUND_STATEMENT);
    if (!is_punctuator(token, P_LEFT_BRACE))
    {
        fail_parse(token, "expected left brace for compound statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    syn->cstmt_block_items = vector_init();
    for (;;)
    {
        parse_status_code_t decl_stat = UNKNOWN_STATUS;
        syntax_component_t* decl = parse_declaration(&token, OPTIONAL, &decl_stat, tlu, next_depth, syn);
        if (decl_stat == FOUND)
        {
            vector_add(syn->cstmt_block_items, decl);
            continue;
        }
        parse_status_code_t stmt_stat = UNKNOWN_STATUS;
        syntax_component_t* stmt = parse_statement(&token, OPTIONAL, &stmt_stat, tlu, next_depth, syn);
        if (stmt_stat == FOUND)
        {
            vector_add(syn->cstmt_block_items, stmt);
            continue;
        }
        break;
    }
    if (!is_punctuator(token, P_RIGHT_BRACE))
    {
        fail_parse(token, "expected right brace for compound statement");
        free_syntax(syn, tlu);
        return NULL;
    }
    advance_token;
    update_status(FOUND);
    return syn;
}

// reminder: define body earlier so that symbols defined in the parameter list are in scope for the function
// update: i don't think this matters ^

// 6.9.1
syntax_component_t* parse_function_definition(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
{
    init_parse;
    init_syn(SC_FUNCTION_DEFINITION);
    parse_status_code_t declspecs_stat = UNKNOWN_STATUS;
    syn->fdef_declaration_specifiers = parse_declaration_specifiers(&token, EXPECTED, &declspecs_stat, tlu, next_depth, syn);
    link_vector_to_parent(syn->fdef_declaration_specifiers);
    if (declspecs_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    parse_status_code_t declr_stat = UNKNOWN_STATUS;
    syn->fdef_declarator = parse_declarator(&token, EXPECTED, &declr_stat, tlu, next_depth, syn);
    if (declr_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    syn->fdef_knr_declarations = vector_init();
    for (;;)
    {
        parse_status_code_t decl_stat = UNKNOWN_STATUS;
        syntax_component_t* knr_decl = parse_declaration(&token, OPTIONAL, &decl_stat, tlu, next_depth, syn);
        if (decl_stat == NOT_FOUND)
            break;
        vector_add(syn->fdef_knr_declarations, knr_decl);
    }
    parse_status_code_t cstmt_stat = UNKNOWN_STATUS;
    syn->fdef_body = parse_compound_statement(&token, EXPECTED, &cstmt_stat, tlu, next_depth, syn);
    // TODO: add __func__ symbol declaration at beginning of compound statement
    if (cstmt_stat == ABORT)
    {
        fail_status;
        free_syntax(syn, tlu);
        return NULL;
    }
    update_status(FOUND);
    return syn;
}

// 6.9
// to be resolved: 6.9 (3), 6.9 (4), 6.9 (5)
// exception: returns translation unit object even on fail
syntax_component_t* parse_translation_unit(token_t** tokens, parse_request_code_t req, parse_status_code_t* stat, syntax_component_t* tlu, int depth, syntax_component_t* parent)
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
        syntax_component_t* funcdef = parse_function_definition(&token, OPTIONAL, &funcdef_stat, tlu, next_depth, syn);
        if (funcdef_stat == FOUND)
        {
            vector_add(syn->tlu_external_declarations, funcdef);
            continue;
        }
        parse_status_code_t decl_stat = UNKNOWN_STATUS;
        syntax_component_t* decl = parse_declaration(&token, OPTIONAL, &decl_stat, tlu, next_depth, syn);
        if (decl_stat == FOUND)
        {
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
syntax_component_t* parse(token_t* tokens)
{
    parse_status_code_t tlu_stat = UNKNOWN_STATUS;
    syntax_component_t* tlu = parse_translation_unit(&tokens, EXPECTED, &tlu_stat, NULL, 1, NULL);
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
        free_syntax(tlu, tlu);
        tlu = NULL;
    }
    return tlu;
}
