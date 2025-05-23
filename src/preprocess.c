#include <stdlib.h>
#include <string.h>

#include "ecc.h"

typedef struct preprocessing_state
{
    preprocessing_table_t* table;
    preprocessing_token_t* tokens;
    preprocessing_settings_t* settings;
} preprocessing_state_t;

typedef enum pp_status_code
{
    UNKNOWN_STATUS = 1,
    FOUND,
    NOT_FOUND,
    ABORT
} pp_status_code_t;

#define init_preprocess(ty) preprocessing_token_t* token = *tokens; \
    preprocessing_component_t* comp = calloc(1, sizeof *comp); \
    comp->type = ty; \
    comp->start = token;

// goes to the next non-whitespace token (since
// technically whitespace are not preprocessing tokens)
#define advance_token do token = (token ? token->next : NULL); while (token != NULL && !is_pp_token(token));

// goes to the next element of the token list no matter what it is
#define advance_token_list token = (token ? token->next : NULL)

#define fail(token, fmt, ...) ((token) ? snerrorf(state->settings->error, MAX_ERROR_LENGTH, "[%s:%d:%d] " fmt "\n", get_file_name(state->settings->filepath, false), (token)->row, (token)->col, ## __VA_ARGS__) : snerrorf(state->settings->error, MAX_ERROR_LENGTH, "[%s] " fmt "\n", get_file_name(state->settings->filepath, false), ## __VA_ARGS__), NULL)
#define fallthrough_fail NULL

#define found (*tokens = comp->end = token, comp)

preprocessing_table_t* preprocessing_table_init(void)
{
    preprocessing_table_t* t = calloc(1, sizeof *t);
    t->key = calloc(50, sizeof(char*));
    memset(t->key, 0, 50 * sizeof(char*));
    t->v_repl_list = calloc(50, sizeof(preprocessing_token_t*));
    memset(t->v_repl_list, 0, 50 * sizeof(preprocessing_token_t*));
    t->v_id_list = calloc(50, sizeof(vector_t*));
    memset(t->v_id_list, 0, 50 * sizeof(vector_t*));
    t->size = 0;
    t->capacity = 50;
    return t;
}

preprocessing_table_t* preprocessing_table_resize(preprocessing_table_t* t)
{
    unsigned old_capacity = t->capacity;
    t->key = realloc(t->key, (t->capacity = (unsigned) (t->capacity * 1.5)) * sizeof(char*));
    memset(t->key + old_capacity, 0, (t->capacity - old_capacity) * sizeof(char*));
    t->v_repl_list = realloc(t->v_repl_list, t->capacity * sizeof(preprocessing_token_t*));
    memset(t->v_repl_list + old_capacity, 0, (t->capacity - old_capacity) * sizeof(preprocessing_token_t*));
    t->v_id_list = realloc(t->v_id_list, t->capacity * sizeof(vector_t*));
    memset(t->v_id_list + old_capacity, 0, (t->capacity - old_capacity) * sizeof(vector_t*));
    return t;
}

// the function will copy params passed in, i.e., the key and the token list
preprocessing_token_t* preprocessing_table_add(preprocessing_table_t* t, char* k, preprocessing_token_t* token, preprocessing_token_t* end, vector_t* id_list)
{
    if (!t) return NULL;
    if (t->size >= t->capacity)
        (void) preprocessing_table_resize(t);
    unsigned long index = hash(k) % t->capacity;
    preprocessing_token_t* v = NULL;
    for (unsigned long i = index;;)
    {
        if (t->key[i] == NULL)
        {
            t->key[i] = strdup(k);
            if (token)
                v = t->v_repl_list[i] = pp_token_copy_range(token, end);
            t->v_id_list[i] = vector_deep_copy(id_list, (void* (*)(void*)) strdup);
            ++(t->size);
            break;
        }
        i = (i + 1) % t->capacity;
        if (i == index) // for safety but this should never happen
            return NULL;
    }
    return v;
}

bool preprocessing_table_get_internal(preprocessing_table_t* t, char* k, int* i, preprocessing_token_t** token, vector_t** id_list)
{
    if (!t) return false;
    unsigned long index = hash(k) % t->capacity;
    unsigned long oidx = index;
    do
    {
        if (t->key[index] != NULL && !strcmp(t->key[index], k))
        {
            if (i) *i = index;
            if (token) *token = t->v_repl_list[index];
            if (id_list) *id_list = t->v_id_list[index];
            return true;
        }
        index = (index + 1 == t->capacity ? 0 : index + 1);
    }
    while (index != oidx);
    if (i) *i = -1;
    return false;
}

bool preprocessing_table_get(preprocessing_table_t* t, char* k, preprocessing_token_t** token, vector_t** id_list)
{
    return preprocessing_table_get_internal(t, k, NULL, token, id_list);
}

void preprocessing_table_remove(preprocessing_table_t* t, char* k)
{
    if (!t) return;
    unsigned long index = hash(k) % t->capacity;
    for (unsigned long i = index;;)
    {
        if (t->key[i] != NULL && !strcmp(t->key[i], k))
        {
            free(t->key[i]);
            pp_token_delete_all(t->v_repl_list[i]);
            vector_deep_delete(t->v_id_list[i], free);
            t->key[i] = NULL;
            t->v_repl_list[i] = NULL;
            t->v_id_list[i] = NULL;
            return;
        }
        i = (i + 1) % t->capacity;
        if (i == index)
            break;
    }
}

void preprocessing_table_delete(preprocessing_table_t* t)
{
    if (!t) return;
    for (unsigned i = 0; i < t->capacity; ++i)
    {
        free(t->key[i]);
        pp_token_delete_all(t->v_repl_list[i]);
        vector_deep_delete(t->v_id_list[i], free);
    }
    free(t->key);
    free(t->v_repl_list);
    free(t->v_id_list);
    free(t);
}

void preprocessing_table_print(preprocessing_table_t* t, int (*printer)(const char* fmt, ...))
{
    printer("preprocessing table:\n");
    for (unsigned i = 0; i < t->capacity; ++i)
    {
        if (!t->key[i])
            continue;
        printer(" \"%s\" -> \n", t->key[i]);
        if (t->v_id_list[i])
        {
            printer("  parameter list:\n");
            for (unsigned j = 0; j < t->v_id_list[i]->size; ++j)
                printer("   %s\n", vector_get(t->v_id_list[i], j));
        }
        printer("  token sequence:\n");
        for (preprocessing_token_t* token = t->v_repl_list[i]; token; token = token->next)
        {
            printer("   ");
            pp_token_print(token, printer);
            printer("\n");
        }
    }
}

void state_delete(preprocessing_state_t* state)
{
    if (!state) return;
    preprocessing_table_delete(state->table);
    free(state);
}

static bool is_punctuator(preprocessing_token_t* token, punctuator_type_t type)
{
    if (!token) return false;
    if (token->type != PPT_PUNCTUATOR) return false;
    return token->punctuator == type;
}

static bool is_identifier(preprocessing_token_t* token, char* identifier)
{
    if (!token) return false;
    if (token->type != PPT_IDENTIFIER) return false;
    return !strcmp(token->identifier, identifier);
}

static bool is_pp_type(preprocessing_token_t* token, preprocessor_token_type_t type)
{
    return token && token->type == type;
}

static bool is_pp_token(preprocessing_token_t* token)
{
    return token && token->type != PPT_WHITESPACE;
}

static bool is_whitespace(preprocessing_token_t* token)
{
    return token && token->type == PPT_WHITESPACE;
}

static bool is_whitespace_containing_newline(preprocessing_token_t* token)
{
    return is_whitespace(token) && contains_char(token->whitespace, '\n');
}

// inserting next
// inserting token next

static preprocessing_token_t* insert_token_after(preprocessing_token_t* token, preprocessing_token_t* inserting)
{
    if (!token || !inserting) return NULL;
    preprocessing_token_t* next = inserting->next;
    inserting->next = token;
    token->prev = inserting;
    token->next = next;
    if (next)
        next->prev = token;
    return token;
}

// a b c
// inserting a b c next

static preprocessing_token_t* insert_token_sequence_after(preprocessing_token_t* tokens, preprocessing_token_t* inserting)
{
    if (!tokens || !inserting) return NULL;
    preprocessing_token_t* next = inserting->next;
    inserting->next = tokens;
    tokens->prev = inserting;

    preprocessing_token_t* last = tokens;
    for (; last->next; last = last->next);

    last->next = next;
    if (next)
        next->prev = last;
    return tokens;
}

// prev token next
// prev next

// returns next token, if any
static preprocessing_token_t* remove_token(preprocessing_token_t* token)
{
    if (!token) return NULL;
    preprocessing_token_t* prev = token->prev;
    preprocessing_token_t* next = token->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    pp_token_delete(token);
    return next;
}

static void pp_token_delete_all_prev(preprocessing_token_t* token)
{
    if (!token) return;
    pp_token_delete_all_prev(token->prev);
    pp_token_delete(token);
}

static void pp_token_delete_all_until(preprocessing_token_t* token, preprocessing_token_t* end)
{
    if (!token || token == end) return;
    pp_token_delete_all_until(token->next, end);
    pp_token_delete(token);
}

// delete every token from a token sequence, start-inclusive, end-exclusive
// returns end
static preprocessing_token_t* remove_token_sequence(preprocessing_token_t* start, preprocessing_token_t* end)
{
    if (!start && !end) return NULL;
    if (!start)
    {
        pp_token_delete_all_prev(end->prev);
        end->prev = NULL;
        return end;
    }
    if (!end)
    {
        if (start->prev)
            start->prev->next = NULL;
        pp_token_delete_all(start);
        return NULL;
    }
    // prev start tok1 tok2 ... tokN end
    if (start->prev)
        start->prev->next = end;
    end->prev = start->prev;
    pp_token_delete_all_until(start, end);
    return end;
}

// returns the token at the position at the end of the expansion, NULL if expansion failed
static preprocessing_token_t* expand(preprocessing_token_t* token, preprocessing_state_t* state, preprocessing_token_t** start)
{
    if (start) *start = token;
    if (!is_pp_type(token, PPT_IDENTIFIER))
        return token;
    preprocessing_token_t* repl = NULL;
    vector_t* params = NULL;
    preprocessing_table_get(state->table, token->identifier, &repl, &params);
    if (!repl)
        return token;
    if (params && !is_punctuator(token->next, P_LEFT_PARENTHESIS))
        return token;
    if (params)
        return fail(token, "invocations of function-like macros are not supported yet");
    preprocessing_token_t* inserting = token;
    for (; repl; repl = repl->next)
    {
        preprocessing_token_t* cp = pp_token_copy(repl);
        cp->row = token->row;
        cp->col = token->col;
        inserting = insert_token_after(cp, inserting);
        preprocessing_token_t* inner_start = NULL;
        if (!(inserting = expand(inserting, state, &inner_start)))
            return NULL;
        if (start && !*start)
            *start = inner_start;
    }
    remove_token(token);
    return inserting;
}

static bool can_start_control_line(preprocessing_state_t* state, preprocessing_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    // null directive
    if (is_whitespace_containing_newline(token->next))
        return true;
    
    advance_token;

    return is_identifier(token, "include") ||
        is_identifier(token, "define") ||
        is_identifier(token, "undef") ||
        is_identifier(token, "line") ||
        is_identifier(token, "error") ||
        is_identifier(token, "pragma");
}

static bool can_start_non_directive(preprocessing_state_t* state, preprocessing_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    advance_token;

    if (!is_pp_token(token))
        return false;
    
    if (is_identifier(token, "if") ||
        is_identifier(token, "ifdef") ||
        is_identifier(token, "ifndef") ||
        is_identifier(token, "elif") ||
        is_identifier(token, "else") ||
        is_identifier(token, "endif") ||
        is_identifier(token, "include") ||
        is_identifier(token, "define") ||
        is_identifier(token, "undef") ||
        is_identifier(token, "line") ||
        is_identifier(token, "error") ||
        is_identifier(token, "pragma"))
        return false;
    
    return true;
}

static bool can_start_text_line(preprocessing_state_t* state, preprocessing_token_t* token)
{
    if (is_punctuator(token, P_HASH))
        return false;
    for (; token && !is_whitespace_containing_newline(token); advance_token_list);
    return token;
}

static bool can_start_standard_directive(preprocessing_state_t* state, preprocessing_token_t* token, char* name)\
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    advance_token;

    return is_identifier(token, name);
}

static bool can_start_if_group(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_standard_directive(state, token, "if");
}

static bool can_start_ifdef_group(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_standard_directive(state, token, "ifdef");
}

static bool can_start_ifndef_group(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_standard_directive(state, token, "ifndef");
}

static bool can_start_if_section(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_if_group(state, token) ||
        can_start_ifdef_group(state, token) ||
        can_start_ifndef_group(state, token);
}

static bool can_start_group_part(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_control_line(state, token) ||
        can_start_if_section(state, token) ||
        can_start_non_directive(state, token) ||
        can_start_text_line(state, token);
}

static bool can_start_elif_group(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_standard_directive(state, token, "elif");
}

static bool can_start_else_group(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_standard_directive(state, token, "else");
}

static bool can_start_endif_line(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_standard_directive(state, token, "endif");
}

typedef enum preprocessing_component_type
{
    PPC_PP_FILE,
    PPC_IF_SECTION,
    PPC_TEXT_LINE,
    PPC_NON_DIRECTIVE,
    PPC_IF_GROUP,
    PPC_IFDEF_GROUP,
    PPC_IFNDEF_GROUP,
    PPC_ELIF_GROUP,
    PPC_ELSE_GROUP,
    PPC_ENDIF_LINE,
    PPC_INCLUDE_LINE,
    PPC_DEFINE_LINE,
    PPC_UNDEF_LINE,
    PPC_LINE_LINE,
    PPC_ERROR_LINE,
    PPC_PRAGMA_LINE,
    PPC_EMPTY_CONTROL_LINE,
    PPC_TOKEN_SEQUENCE
} preprocessing_component_type_t;

// PPC_GROUP_PART = PPC_IF_SECTION | PPC_CONTROL_LINE | PPC_TEXT_LINE | PPC_NON_DIRECTIVE
// PPC_CONTROL_LINE = PPC_INCLUDE_LINE | PPC_DEFINE_LINE | PPC_UNDEF_LINE | PPC_LINE_LINE | PPC_ERROR_LINE | PPC_PRAGMA_LINE | PPC_EMPTY_CONTROL_LINE

typedef struct preprocessing_component preprocessing_component_t;

typedef struct preprocessing_component
{
    preprocessing_component_type_t type;
    preprocessing_token_t* start;
    preprocessing_token_t* end;
    // for if-like, elif, and else groups, their beginning directives end much earlier than the groups themselves 
    preprocessing_token_t* directive_end;
    union
    {
        // PPC_PP_FILE
        vector_t* ppf_parts; // vector_t<preprocessing_component_t* (PPC_GROUP_PART)>
        // PPC_IF_SECTION
        struct
        {
            preprocessing_component_t* ifs_if_group; // PPC_IF_GROUP | PPC_IFDEF_GROUP | PPC_IFNDEF_GROUP
            vector_t* ifs_elif_groups; // vector_t<preprocessing_component_t* (PPC_ELIF_GROUP)>
            preprocessing_component_t* ifs_else_group; // PPC_ELSE_GROUP
            preprocessing_component_t* ifs_endif_line; // PPC_ENDIF_LINE
        };
        // PPC_IF_GROUP
        struct
        {
            syntax_component_t* ifg_condition; // PPC_CONDITIONAL_EXPRESSION (constant expression)
            vector_t* ifg_parts; // vector_t<preprocessing_component_t* (PPC_GROUP_PART)>
        };
        // PPC_IFDEF_GROUP
        struct
        {
            char* ifdg_id;
            vector_t* ifdg_parts; // vector_t<preprocessing_component_t* (PPC_GROUP_PART)>
        };
        // PPC_IFNDEF_GROUP
        struct
        {
            char* ifndg_id;
            vector_t* ifndg_parts; // vector_t<preprocessing_component_t* (PPC_GROUP_PART)>
        };
        // PPC_ELIF_GROUP
        struct
        {
            syntax_component_t* elifg_condition; // PPC_CONDITIONAL_EXPRESSION (constant expression)
            vector_t* elifg_parts; // vector_t<preprocessing_component_t* (PPC_GROUP_PART)>
        };
        // PPC_ELSE_GROUP
        vector_t* elseg_parts; // vector_t<preprocessing_component_t* (PPC_GROUP_PART)>
        // PPC_INCLUDE_LINE
        preprocessing_component_t* incl_sequence; // PPC_TOKEN_SEQUENCE
        // PPC_DEFINE_LINE
        struct
        {
            char* defl_id;
            vector_t* defl_params; // vector_t<char*>
            preprocessing_component_t* defl_replacement; // PPC_TOKEN_SEQUENCE
        };
        // PPC_UNDEF_LINE
        char* undefl_id;
        // PPC_LINE_LINE
        preprocessing_component_t* linel_sequence; // PPC_TOKEN_SEQUENCE
        // PPC_ERROR_LINE
        preprocessing_component_t* errl_sequence; // PPC_TOKEN_SEQUENCE
        // PPC_PRAGMA_LINE
        preprocessing_component_t* pragl_sequence; // PPC_TOKEN_SEQUENCE
    };
} preprocessing_component_t;

void pp_component_delete(preprocessing_component_t* comp)
{
    if (!comp) return;
    switch (comp->type)
    {
        case PPC_PP_FILE:
            vector_deep_delete(comp->ppf_parts, (void (*)(void*)) pp_component_delete);
            break;
        case PPC_IF_SECTION:
            pp_component_delete(comp->ifs_if_group);
            vector_deep_delete(comp->ifs_elif_groups, (void (*)(void*)) pp_component_delete);
            pp_component_delete(comp->ifs_else_group);
            pp_component_delete(comp->ifs_endif_line);
            break;
        case PPC_IF_GROUP:
            free_syntax(comp->ifg_condition, NULL);
            vector_deep_delete(comp->ifg_parts, (void (*)(void*)) pp_component_delete);
            break;
        case PPC_IFDEF_GROUP:
            free(comp->ifdg_id);
            vector_deep_delete(comp->ifdg_parts, (void (*)(void*)) pp_component_delete);
            break;
        case PPC_IFNDEF_GROUP:
            free(comp->ifndg_id);
            vector_deep_delete(comp->ifndg_parts, (void (*)(void*)) pp_component_delete);
            break;
        case PPC_ELIF_GROUP:
            free_syntax(comp->elifg_condition, NULL);
            vector_deep_delete(comp->elifg_parts, (void (*)(void*)) pp_component_delete);
            break;
        case PPC_ELSE_GROUP:
            vector_deep_delete(comp->elseg_parts, (void (*)(void*)) pp_component_delete);
            break;
        case PPC_INCLUDE_LINE:
            pp_component_delete(comp->incl_sequence);
            break;
        case PPC_DEFINE_LINE:
            free(comp->defl_id);
            vector_deep_delete(comp->defl_params, free);
            pp_component_delete(comp->defl_replacement);
            break;
        case PPC_UNDEF_LINE:
            free(comp->undefl_id);
            break;
        case PPC_LINE_LINE:
            pp_component_delete(comp->linel_sequence);
            break;
        case PPC_ERROR_LINE:
            pp_component_delete(comp->errl_sequence);
            break;
        case PPC_PRAGMA_LINE:
            pp_component_delete(comp->pragl_sequence);
            break;
        case PPC_TEXT_LINE:
        case PPC_ENDIF_LINE:
        case PPC_TOKEN_SEQUENCE:
        case PPC_NON_DIRECTIVE:
            // no free'ing to be done
            break;
        default:
            warnf("no free'ing operation defined for preprocessing component no. %d\n", comp->type);
            break;
    }
    free(comp);
}

// ps - print structure
// pf - print field
#define ps(fmt, ...) { for (unsigned i = 0; i < indent; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define pf(fmt, ...) { for (unsigned i = 0; i < indent + 1; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define next_indent (indent + 2)
#define sty(x) "[" x "]"

static void pp_component_print_indented(preprocessing_component_t* comp, unsigned indent, int (*printer)(const char* fmt, ...));

#define create_vector_printer(name, element, operation) \
    static void name(vector_t* v, unsigned indent, int (*printer)(const char* fmt, ...)) \
    { \
        if (!v) \
        { \
            ps("null\n"); \
            return; \
        } \
        if (!v->size) \
        { \
            ps("[\n\n"); \
            ps("]\n"); \
            return; \
        } \
        ps("[\n"); \
        for (unsigned i = 0; i < v->size; ++i) \
        { \
            element el = vector_get(v, i); \
            operation; \
        } \
        ps("]\n"); \
    }

create_vector_printer(print_vector_indented, preprocessing_component_t*, pp_component_print_indented(el, indent + 1, printer));
create_vector_printer(print_string_vector_indented, char*, pf("%s\n", el));

static void pp_component_print_indented(preprocessing_component_t* comp, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!comp)
    {
        ps("null\n");
        return;
    }
    switch (comp->type)
    {
        case PPC_PP_FILE:
            ps(sty("preprocessing file") "\n");
            pf("group:\n");
            print_vector_indented(comp->ppf_parts, next_indent, printer);
            break;
        case PPC_IF_SECTION:
            ps(sty("if section") "\n");
            pf("if group:\n");
            pp_component_print_indented(comp->ifs_if_group, next_indent, printer);
            pf("elif groups:\n");
            print_vector_indented(comp->ifs_elif_groups, next_indent, printer);
            pf("else group:\n");
            pp_component_print_indented(comp->ifs_else_group, next_indent, printer);
            pf("endif line:\n");
            pp_component_print_indented(comp->ifs_endif_line, next_indent, printer);
            break;
        case PPC_IF_GROUP:
            ps(sty("if group") "\n");
            pf("condition: [omitted]\n");
            pf("group:\n");
            print_vector_indented(comp->ifg_parts, next_indent, printer);
            break;
        case PPC_IFDEF_GROUP:
            ps(sty("ifdef group") "\n");
            pf("identifier: %s\n", comp->ifdg_id);
            pf("group:\n");
            print_vector_indented(comp->ifdg_parts, next_indent, printer);
            break;
        case PPC_IFNDEF_GROUP:
            ps(sty("ifndef group") "\n");
            pf("identifier: %s\n", comp->ifndg_id);
            pf("group:\n");
            print_vector_indented(comp->ifndg_parts, next_indent, printer);
            break;
        case PPC_ELIF_GROUP:
            ps(sty("elif group") "\n");
            pf("condition: [omitted]\n");
            pf("group:\n");
            print_vector_indented(comp->elifg_parts, next_indent, printer);
            break;
        case PPC_ELSE_GROUP:
            ps(sty("else group") "\n");
            pf("group:\n");
            print_vector_indented(comp->elseg_parts, next_indent, printer);
            break;
        case PPC_INCLUDE_LINE:
            ps(sty("include line") "\n");
            pf("token sequence:\n");
            pp_component_print_indented(comp->incl_sequence, next_indent, printer);
            break;
        case PPC_DEFINE_LINE:
            ps(sty("define line") "\n");
            pf("identifier: %s\n", comp->defl_id);
            pf("parameter list:\n");
            print_string_vector_indented(comp->defl_params, next_indent, printer);
            pf("replacement list:\n");
            pp_component_print_indented(comp->defl_replacement, next_indent, printer);
            break;
        case PPC_UNDEF_LINE:
            ps(sty("undef line") "\n");
            pf("identifier: %s\n", comp->undefl_id);
            break;
        case PPC_LINE_LINE:
            ps(sty("line line") "\n");
            pf("token sequence:\n");
            pp_component_print_indented(comp->linel_sequence, next_indent, printer);
            break;
        case PPC_ERROR_LINE:
            ps(sty("error line") "\n");
            pf("token sequence:\n");
            pp_component_print_indented(comp->errl_sequence, next_indent, printer);
            break;
        case PPC_PRAGMA_LINE:
            ps(sty("pragma line") "\n");
            pf("token sequence:\n");
            pp_component_print_indented(comp->pragl_sequence, next_indent, printer);
            break;
        case PPC_EMPTY_CONTROL_LINE:
            ps(sty("empty control line") "\n");
            break;
        case PPC_ENDIF_LINE:
            ps(sty("endif line") "\n");
            break;
        case PPC_TOKEN_SEQUENCE:
            ps(sty("token sequence") "\n");
            break;
        case PPC_TEXT_LINE:
            ps(sty("text line") "\n");
            break;
        case PPC_NON_DIRECTIVE:
            ps(sty("non-directive") "\n");
            break;
        default:
            ps(sty("unknown/unprintable preprocessing component") "\n");
            pf("type number: %d\n", comp->type);
            return;
    }

    if (comp->start)
    {
        pf("start: (%d, %d)\n", comp->start->row, comp->start->col);
    }
    else
        pf("start: (EOF)\n");

    if (comp->end)
    {
        pf("end: (%d, %d)\n", comp->end->row, comp->end->col);
    }
    else
        pf("end: (EOF)\n");
    
    switch (comp->type)
    {
        case PPC_IF_GROUP:
        case PPC_IFDEF_GROUP:
        case PPC_IFNDEF_GROUP:
        case PPC_ELIF_GROUP:
        case PPC_ELSE_GROUP:
            if (comp->directive_end)
            {
                pf("directive end: (%d, %d)\n", comp->directive_end->row, comp->directive_end->col);
            }
            else
                pf("directive end: (EOF)\n");
            break;
        default:
            break;
    }
}

#undef ps
#undef pf
#undef next_indent
#undef sty

void pp_component_print(preprocessing_component_t* comp, int (*printer)(const char* fmt, ...))
{
    pp_component_print_indented(comp, 0, printer);
}

preprocessing_component_t* pp_parse_group_part(preprocessing_token_t** tokens, preprocessing_state_t* state);

preprocessing_component_t* pp_parse_if_group(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    return fail(*tokens, "#if directives are not supported yet");
}

preprocessing_component_t* pp_parse_ifdef_group(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_IFDEF_GROUP);
    
    // skip '#' and 'ifdef' (both checked when checking if it can start an ifdef)
    advance_token;
    advance_token;

    if (!is_pp_type(token, PPT_IDENTIFIER))
    {
        pp_component_delete(comp);
        return fail(token, "expected identifier to check for being a macro");
    }

    comp->ifdg_id = strdup(token->identifier);

    advance_token_list;
    if (!is_whitespace_containing_newline(token))
    {
        pp_component_delete(comp);
        return fail(token, "newline expected at the end of #ifdef directive");
    }
    advance_token_list;
    comp->directive_end = token;

    while (can_start_group_part(state, token))
    {
        preprocessing_component_t* part = pp_parse_group_part(&token, state);
        if (!part)
        {
            pp_component_delete(comp);
            return fallthrough_fail;
        }
        if (!comp->ifdg_parts) comp->ifdg_parts = vector_init();
        vector_add(comp->ifdg_parts, part);
    }

    return found;
}

preprocessing_component_t* pp_parse_ifndef_group(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_IFNDEF_GROUP);
    
    // skip '#' and 'ifndef' (both checked when checking if it can start an ifndef)
    advance_token;
    advance_token;

    if (!is_pp_type(token, PPT_IDENTIFIER))
    {
        pp_component_delete(comp);
        return fail(token, "expected identifier to check for being a macro");
    }

    comp->ifndg_id = strdup(token->identifier);

    advance_token_list;
    if (!is_whitespace_containing_newline(token))
    {
        pp_component_delete(comp);
        return fail(token, "newline expected at the end of #ifndef directive");
    }
    advance_token_list;
    comp->directive_end = token;

    while (can_start_group_part(state, token))
    {
        preprocessing_component_t* part = pp_parse_group_part(&token, state);
        if (!part)
        {
            pp_component_delete(comp);
            return fallthrough_fail;
        }
        if (!comp->ifndg_parts) comp->ifndg_parts = vector_init();
        vector_add(comp->ifndg_parts, part);
    }

    return found;
}

preprocessing_component_t* pp_parse_elif_group(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    return fail(*tokens, "#elif directives are not supported yet");
}

preprocessing_component_t* pp_parse_else_group(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_ELSE_GROUP);
    
    // skip '#' and 'else' (both checked when checking if it can start an else)
    advance_token;
    advance_token_list;

    if (!is_whitespace_containing_newline(token))
    {
        pp_component_delete(comp);
        return fail(token, "newline expected at the end of #else directive");
    }

    advance_token_list;
    comp->directive_end = token;

    while (can_start_group_part(state, token))
    {
        preprocessing_component_t* part = pp_parse_group_part(&token, state);
        if (!part)
        {
            pp_component_delete(comp);
            return fallthrough_fail;
        }
        if (!comp->elseg_parts) comp->elseg_parts = vector_init();
        vector_add(comp->elseg_parts, part);
    }

    return found;
}

preprocessing_component_t* pp_parse_endif_line(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_ENDIF_LINE);
    // move past hash
    advance_token;
    // move past endif
    advance_token_list;
    if (!is_whitespace_containing_newline(token))
    {
        pp_component_delete(comp);
        return fail(token, "#endif directive must end with a newline");
    }
    advance_token_list; // move past whitespace
    return found;
}

preprocessing_component_t* pp_parse_if_section(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_IF_SECTION);
    if (can_start_if_group(state, token))
        comp->ifs_if_group = pp_parse_if_group(&token, state);
    else if (can_start_ifdef_group(state, token))
        comp->ifs_if_group = pp_parse_ifdef_group(&token, state);
    else
        comp->ifs_if_group = pp_parse_ifndef_group(&token, state);
    if (!comp->ifs_if_group)
    {
        pp_component_delete(comp);
        return fallthrough_fail;
    }
    while (can_start_elif_group(state, token))
    {
        if (!comp->ifs_elif_groups) comp->ifs_elif_groups = vector_init();
        preprocessing_component_t* elifg = pp_parse_elif_group(&token, state);
        if (!elifg)
        {
            pp_component_delete(comp);
            return fallthrough_fail;
        }
        vector_add(comp->ifs_elif_groups, elifg);
    }
    if (can_start_else_group(state, token))
    {
        comp->ifs_else_group = pp_parse_else_group(&token, state);
        if (!comp->ifs_else_group)
        {
            pp_component_delete(comp);
            return fallthrough_fail;
        }
    }
    if (!can_start_endif_line(state, token))
    {
        pp_component_delete(comp);
        return fail(token, "#endif directive expected at the end of conditional inclusion");
    }
    comp->ifs_endif_line = pp_parse_endif_line(&token, state);
    if (!comp->ifs_endif_line)
    {
        pp_component_delete(comp);
        return fallthrough_fail;
    }
    return found;
}

preprocessing_component_t* pp_parse_token_sequence(preprocessing_token_t** tokens, preprocessing_state_t* state, bool (*terminator)(preprocessing_token_t* token))
{
    init_preprocess(PPC_TOKEN_SEQUENCE);
    for (; !terminator(token); advance_token_list);
    return found;
}

preprocessing_component_t* pp_parse_include_line(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_INCLUDE_LINE);
    // move past hash
    advance_token;
    // move past include
    advance_token;
    
    comp->incl_sequence = pp_parse_token_sequence(&token, state, is_whitespace_containing_newline);

    if (!token)
    {
        pp_component_delete(comp);
        return fail(token, "#include directive must end with a newline");
    }

    advance_token_list; // move past whitespace
    return found;
}

preprocessing_component_t* pp_parse_define_line(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_DEFINE_LINE);
    // move past hash
    advance_token;
    // move past define
    advance_token;
    if (!is_pp_type(token, PPT_IDENTIFIER))
    {
        pp_component_delete(comp);
        return fail(token, "expected name for macro definition");
    }
    comp->defl_id = strdup(token->identifier);
    advance_token_list;
    if (is_punctuator(token, P_LEFT_PARENTHESIS))
    {
        advance_token;
        comp->defl_params = vector_init();
        while (!is_punctuator(token, P_RIGHT_PARENTHESIS))
        {
            if (is_punctuator(token, P_ELLIPSIS))
            {
                vector_add(comp->defl_params, strdup("__VA_ARGS__"));
                advance_token;
                if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
                {
                    pp_component_delete(comp);
                    return fail(token, "ellipsis for a variadic function-like macro must be at the end of the parameter list");
                }
                advance_token_list;
                break;
            }
            if (!is_pp_type(token, PPT_IDENTIFIER))
            {
                pp_component_delete(comp);
                return fail(token, "identifier expected for function-like macro parameter list");
            }
            vector_add(comp->defl_params, strdup(token->identifier));
            advance_token;
            if (is_punctuator(token, P_COMMA))
            {
                advance_token;
                if (!is_pp_type(token, PPT_IDENTIFIER) && !is_punctuator(token, P_ELLIPSIS))
                {
                    pp_component_delete(comp);
                    return fail(token, "identifier expected for function-like macro parameter list");
                }
            }
        }
        advance_token_list; // go past right paren
    }
    if (is_whitespace(token) && !is_whitespace_containing_newline(token))
        advance_token_list;
    comp->defl_replacement = pp_parse_token_sequence(&token, state, is_whitespace_containing_newline);
    if (!token)
    {
        pp_component_delete(comp);
        return fail(token, "#define directive must end with a newline");
    }

    advance_token_list;
    return found;
}

preprocessing_component_t* pp_parse_undef_line(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_UNDEF_LINE);
    // move past hash
    advance_token;
    // move past undef
    advance_token;

    if (!is_pp_type(token, PPT_IDENTIFIER))
    {
        pp_component_delete(comp);
        return fail(token, "identifier expected for #undef directive");
    }

    comp->undefl_id = strdup(token->identifier);

    advance_token_list;

    if (!is_whitespace_containing_newline(token))
    {
        pp_component_delete(comp);
        return fail(token, "#undef directive must end with a newline");
    }

    advance_token_list;
    return found;
}

preprocessing_component_t* pp_parse_control_line(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    if (can_start_standard_directive(state, *tokens, "include"))
        return pp_parse_include_line(tokens, state);
    if (can_start_standard_directive(state, *tokens, "define"))
        return pp_parse_define_line(tokens, state);
    if (can_start_standard_directive(state, *tokens, "undef"))
        return pp_parse_undef_line(tokens, state);
    return fail(*tokens, "#line, #error, #pragma, and the null directive are not supported yet");
}

preprocessing_component_t* pp_parse_text_line(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_TEXT_LINE);
    for (; !is_whitespace_containing_newline(token); advance_token_list);
    if (!token)
    {
        pp_component_delete(comp);
        return fail(token, "translation unit must end with a newline");
    }
    advance_token_list; // move past whitespace
    return found;
}

preprocessing_component_t* pp_parse_non_directive(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_NON_DIRECTIVE);
    for (; !is_whitespace_containing_newline(token); advance_token_list);
    // this error won't happen probably
    if (token == comp->start)
    {
        pp_component_delete(comp);
        return fail(token, "non-directives must have content after the hash");
    }
    if (!token)
    {
        pp_component_delete(comp);
        return fail(token, "translation unit must end with a newline");
    }
    advance_token_list; // move past whitespace
    return found;
}

preprocessing_component_t* pp_parse_group_part(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    if (can_start_if_section(state, *tokens))
        return pp_parse_if_section(tokens, state);
    if (can_start_control_line(state, *tokens))
        return pp_parse_control_line(tokens, state);
    if (can_start_text_line(state, *tokens))
        return pp_parse_text_line(tokens, state);
    if (can_start_non_directive(state, *tokens))
        return pp_parse_non_directive(tokens, state);
    return fallthrough_fail;
}

preprocessing_component_t* pp_parse_preprocessing_file(preprocessing_token_t** tokens, preprocessing_state_t* state)
{
    init_preprocess(PPC_PP_FILE);
    comp->ppf_parts = vector_init();
    while (can_start_group_part(state, token))
    {
        preprocessing_component_t* part = pp_parse_group_part(&token, state);
        if (!part)
        {
            pp_component_delete(comp);
            return fallthrough_fail;
        }
        vector_add(comp->ppf_parts, part);
    }
    // there's more and it's not a group part? (most definitely means there's just a missing newline)
    if (token)
        return fail(token, "file must end with a newline");
    return found;
}

bool preprocess_group(vector_t* group, preprocessing_state_t* state);

bool preprocess_if_section(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    bool held = false;
    switch (comp->ifs_if_group->type)
    {
        case PPC_IF_GROUP:
            (void) fail(comp->start, "#if directives are not supported yet");
            return false;
        case PPC_IFDEF_GROUP:
        case PPC_IFNDEF_GROUP:
            bool f = preprocessing_table_get(state->table, comp->ifs_if_group->ifndg_id, NULL, NULL);
            held = comp->ifs_if_group->type == PPC_IFDEF_GROUP ? f : !f;
            break;
        default:
            (void) fail(comp->start, "nope... that makes no sense...");
            return false;
    }
    if (held)
    {
        // delete #if directive, every #elif group, #else group, and #endif directive
        remove_token_sequence(comp->ifs_if_group->start, comp->ifs_if_group->directive_end);
        if (comp->ifs_elif_groups)
        {
            VECTOR_FOR(preprocessing_component_t*, group, comp->ifs_elif_groups)
                remove_token_sequence(group->start, group->end);
        }
        if (comp->ifs_else_group)
            remove_token_sequence(comp->ifs_else_group->start, comp->ifs_else_group->end);
        remove_token_sequence(comp->ifs_endif_line->start, comp->ifs_endif_line->end);
        preprocess_group(comp->ifs_if_group->ifg_parts, state);
        return true;
    }
    if (comp->ifs_elif_groups)
    {
        (void) fail(comp->start, "#elif directives are not supported yet");
        return false;
    }
    // delete #if group, every #elif group, #else directive, and #endif directive
    remove_token_sequence(comp->ifs_if_group->start, comp->ifs_if_group->end);
    if (comp->ifs_elif_groups)
    {
        VECTOR_FOR(preprocessing_component_t*, group, comp->ifs_elif_groups)
            remove_token_sequence(group->start, group->end);
    }
    if (comp->ifs_else_group)
        remove_token_sequence(comp->ifs_else_group->start, comp->ifs_else_group->directive_end);
    remove_token_sequence(comp->ifs_endif_line->start, comp->ifs_endif_line->end);
    if (comp->ifs_else_group)
        preprocess_group(comp->ifs_else_group->elseg_parts, state);
    return true;
}

FILE* open_include_path(char* inc, bool quote_delimited, preprocessing_state_t* state, char** path)
{
    *path = malloc(LINUX_MAX_PATH_LENGTH);

    // if the include path is quote delimited, try searching in the path of the current source file
    if (quote_delimited && state->settings->filepath)
    {
        char* dirpath = get_directory_path(state->settings->filepath);
        snprintf(*path, LINUX_MAX_PATH_LENGTH, "%s/%s", dirpath, inc);
        free(dirpath);
        FILE* file = fopen(*path, "r");
        if (file)
            return file;
    }

    // if not or if just in angled brackets, search in the list of directories
    for (int i = 0; i < sizeof(ANGLED_INCLUDE_SEARCH_DIRECTORIES) / sizeof(ANGLED_INCLUDE_SEARCH_DIRECTORIES[0]); ++i)
    {
        snprintf(*path, LINUX_MAX_PATH_LENGTH, "%s/%s", ANGLED_INCLUDE_SEARCH_DIRECTORIES[i], inc);
        FILE* file = fopen(*path, "r");
        if (file)
            return file;
    }

    // if nothing's found, bye bye
    return NULL;
}

bool preprocess_include_file(FILE* file, char* path, preprocessing_state_t* state, preprocessing_token_t** tokens)
{
    preprocessing_token_t* pp_tokens = lex(file, false);
    if (!pp_tokens)
        return false;

    preprocessing_settings_t settings;
    settings.filepath = path;
    settings.error = state->settings->error;
    settings.table = state->table;
    if (!preprocess(&pp_tokens, &settings))
        return false;
    
    if (tokens) *tokens = pp_tokens;
    return true;
}

bool preprocess_include_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    preprocessing_token_t* seq_start = NULL;
    for (preprocessing_token_t* token = comp->incl_sequence->start; token && token != comp->incl_sequence->end; token = token->next)
        if (!(token = expand(token, state, seq_start ? NULL : &seq_start)))
            return false;
    comp->incl_sequence->start = seq_start;
    if (!is_pp_type(seq_start, PPT_HEADER_NAME))
    {
        (void) fail(seq_start, "#include directives that do not expand directly to header names are not supported yet");
        return false;
    }

    char* path = NULL;
    FILE* file = open_include_path(seq_start->header_name.name, seq_start->header_name.quote_delimited, state, &path);
    if (!file)
    {
        (void) fail(seq_start, "could not open file '%s'", seq_start->header_name.name);
        return false;
    }

    preprocessing_token_t* included = NULL;
    if (!preprocess_include_file(file, path, state, &included))
        return false;

    if (included)
    {
        insert_token_sequence_after(included, comp->incl_sequence->end);
        comp->end = included;
    }

    remove_token_sequence(comp->start, comp->end);
    return true;
}

bool preprocess_define_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    preprocessing_table_add(state->table, comp->defl_id,
        comp->defl_replacement->start,
        comp->defl_replacement->end,
        comp->defl_params);
    remove_token_sequence(comp->start, comp->end);
    return true;
}

bool preprocess_undef_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    preprocessing_table_remove(state->table, comp->undefl_id);
    remove_token_sequence(comp->start, comp->end);
    return true;
}

bool preprocess_line_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    return false;
}

bool preprocess_error_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    return false;
}

bool preprocess_pragma_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    return false;
}

bool preprocess_control_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    switch (comp->type)
    {
        case PPC_INCLUDE_LINE:
            return preprocess_include_line(comp, state);
        case PPC_DEFINE_LINE:
            return preprocess_define_line(comp, state);
        case PPC_UNDEF_LINE:
            return preprocess_undef_line(comp, state);
        case PPC_LINE_LINE:
            return preprocess_line_line(comp, state);
        case PPC_ERROR_LINE:
            return preprocess_error_line(comp, state);
        case PPC_PRAGMA_LINE:
            return preprocess_pragma_line(comp, state);
        default:
            (void) fail(comp->start, "unexpected control line type");
            return false;
    }
}

bool preprocess_text_line(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    for (preprocessing_token_t* token = comp->start; token && token != comp->end; token = token->next)
        if (!(token = expand(token, state, NULL)))
            return false;
    return true;
}

bool preprocess_non_directive(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    return false;
}

bool preprocess_group_part(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    switch (comp->type)
    {
        case PPC_IF_SECTION:
            return preprocess_if_section(comp, state);
        case PPC_INCLUDE_LINE:
        case PPC_DEFINE_LINE:
        case PPC_UNDEF_LINE:
        case PPC_LINE_LINE:
        case PPC_ERROR_LINE:
        case PPC_PRAGMA_LINE:
            return preprocess_control_line(comp, state);
        case PPC_TEXT_LINE:
            return preprocess_text_line(comp, state);
        case PPC_NON_DIRECTIVE:
            return preprocess_non_directive(comp, state);
        default:
            (void) fail(comp->start, "unexpected group part type");
            return false;
    }
}

bool preprocess_group(vector_t* group, preprocessing_state_t* state)
{
    if (!group)
        return false;
    VECTOR_FOR(preprocessing_component_t*, part, group)
        if (!preprocess_group_part(part, state))
            return false;
    return true;
}

bool preprocess_preprocessing_file(preprocessing_component_t* comp, preprocessing_state_t* state)
{
    return preprocess_group(comp->ppf_parts, state);
}

bool preprocess(preprocessing_token_t** tokens, preprocessing_settings_t* settings)
{
    preprocessing_token_t* dummy = calloc(1, sizeof *dummy);
    dummy->type = PPT_OTHER;
    (*tokens)->prev = dummy;
    dummy->next = *tokens;

    preprocessing_state_t* state = calloc(1, sizeof *state);
    state->settings = settings;
    state->tokens = *tokens;
    if (settings->table)
        state->table = settings->table;
    else
        state->table = preprocessing_table_init();
    preprocessing_token_t* tmp = *tokens;

    // part 1: treeify
    preprocessing_component_t* pp_file = pp_parse_preprocessing_file(&tmp, state);
    if (!pp_file)
    {
        state_delete(state);
        return false;
    }

    if (get_program_options()->iflag)
    {
        printf("<<preprocessing tree>>\n");
        pp_component_print(pp_file, printf);
    }

    // part 2: analyze tree
    bool success = preprocess_preprocessing_file(pp_file, state);

    if (get_program_options()->iflag)
    {
        printf("<<preprocessing table>>\n");
        preprocessing_table_print(state->table, printf);
    }
    
    if (settings->table)
        state->table = NULL;
    state_delete(state);
    pp_component_delete(pp_file);

    *tokens = dummy->next;
    pp_token_delete(dummy);
    if (*tokens)
        (*tokens)->prev = NULL;

    return success;
}

// translation phase 5
void charconvert(preprocessing_token_t* tokens)
{

}

// translation phase 6
void strlitconcat(preprocessing_token_t* tokens)
{
    while (tokens)
    {
        if (tokens->type != PPT_STRING_LITERAL)
        {
            tokens = tokens->next;
            continue;
        }
        preprocessing_token_t* next = tokens;
        do next = next->next; while (next && next->type == PPT_WHITESPACE);
        if (!next || next->type != PPT_STRING_LITERAL)
        {
            tokens = tokens->next;
            continue;
        }
        buffer_t* buf = buffer_init();
        buffer_append_str(buf, tokens->string_literal.value);
        buffer_append_str(buf, next->string_literal.value);
        free(tokens->string_literal.value);
        tokens->string_literal.value = buffer_export(buf);
        buffer_delete(buf);
        tokens->string_literal.wide = tokens->string_literal.wide || next->string_literal.wide;
        remove_token(next);
    }
}