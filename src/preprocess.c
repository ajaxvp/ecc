#include <stdlib.h>
#include <string.h>

#include "cc.h"

typedef struct preprocessing_table
{
    char** key;
    preprocessor_token_t** value;
    unsigned size;
    unsigned capacity;
} preprocessing_table_t;

typedef struct preprocessing_state
{
    preprocessing_table_t* table;
    preprocessor_token_t* tokens;
} preprocessing_state_t;

typedef enum pp_status_code
{
    UNKNOWN_STATUS = 1,
    FOUND,
    NOT_FOUND,
    ABORT
} pp_status_code_t;

typedef enum pp_request_code
{
    UNKNOWN_REQUEST = 1,
    CHECK,
    OPTIONAL,
    EXPECTED
} pp_request_code_t;

#define init_preprocess preprocessor_token_t* token = *tokens;

// goes to the next non-whitespace token (since
// technically whitespace are not preprocessing tokens)
#define advance_token do token = token->next; while (token && token->type == PPT_WHITESPACE);

// goes to the next element of the token list no matter what it is
#define advance_token_list token = (token ? token->next : NULL)

#define fail_status (req == OPTIONAL ? NOT_FOUND : ABORT)
#define fail_preprocess(token, fmt, ...) (/* something w/ the error, */ fail_status)

#define found_status (*tokens = token, FOUND)

preprocessing_table_t* preprocessing_table_init(void)
{
    preprocessing_table_t* t = calloc(1, sizeof *t);
    t->key = calloc(50, sizeof(char*));
    memset(t->key, 0, 50 * sizeof(char*));
    t->value = calloc(50, sizeof(preprocessor_token_t*));
    memset(t->value, 0, 50 * sizeof(preprocessor_token_t*));
    t->size = 0;
    t->capacity = 50;
    return t;
}

preprocessing_table_t* preprocessing_table_resize(preprocessing_table_t* t)
{
    unsigned old_capacity = t->capacity;
    t->key = realloc(t->key, (t->capacity = (unsigned) (t->capacity * 1.5)) * sizeof(char*));
    memset(t->key + old_capacity, 0, (t->capacity - old_capacity) * sizeof(char*));
    t->value = realloc(t->value, t->capacity * sizeof(symbol_t*));
    memset(t->value + old_capacity, 0, (t->capacity - old_capacity) * sizeof(char*));
    return t;
}

// the function will copy params passed in, i.e., the key and the token list
preprocessor_token_t* preprocessing_table_add(preprocessing_table_t* t, char* k, preprocessor_token_t* token, preprocessor_token_t* end)
{
    if (!t) return NULL;
    if (t->size >= t->capacity)
        (void) preprocessing_table_resize(t);
    unsigned long index = hash(k) % t->capacity;
    preprocessor_token_t* v = NULL;
    for (unsigned long i = index;;)
    {
        if (t->key[i] == NULL)
        {
            t->key[i] = strdup(k);
            v = t->value[i] = pp_token_copy_range(token, end);
            ++(t->size);
            break;
        }
        i = (i + 1) % t->capacity;
        if (i == index) // for safety but this should never happen
            return NULL;
    }
    return v;
}

preprocessor_token_t* preprocessing_table_get_internal(preprocessing_table_t* t, char* k, int* i)
{
    if (!t) return NULL;
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

preprocessor_token_t* preprocessing_table_get(preprocessing_table_t* t, char* k)
{
    return preprocessing_table_get_internal(t, k, NULL);
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
            pp_token_delete_all(t->value[i]);
            t->key[i] = NULL;
            t->value[i] = NULL;
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
        pp_token_delete_all(t->value[i]);
    }
    free(t->key);
    free(t->value);
}

void state_delete(preprocessing_state_t* state)
{
    if (!state) return;
    preprocessing_table_delete(state->table);
    free(state);
}

static bool is_punctuator(preprocessor_token_t* token, punctuator_type_t type)
{
    if (!token) return false;
    if (token->type != PPT_PUNCTUATOR) return false;
    return token->punctuator == type;
}

static bool is_identifier(preprocessor_token_t* token, char* identifier)
{
    if (!token) return false;
    if (token->type != PPT_IDENTIFIER) return false;
    return !strcmp(token->identifier, identifier);
}

static bool is_pp_token(preprocessor_token_t* token)
{
    return token && token->type != PPT_WHITESPACE;
}

static bool is_whitespace(preprocessor_token_t* token)
{
    return token && token->type == PPT_WHITESPACE;
}

static bool is_whitespace_ending_newline(preprocessor_token_t* token)
{
    return is_whitespace(token) && ends_with_ignore_case(token->whitespace, "\n");
}

static bool is_header_name(preprocessor_token_t* token)
{
    return token && token->type == PPT_HEADER_NAME;
}

static preprocessor_token_t* insert_token_after(preprocessor_token_t* token, preprocessor_token_t* inserting)
{
    if (!token || !inserting) return NULL;
    preprocessor_token_t* next = token->next;
    inserting->prev = token;
    inserting->next = next;
    token->next = inserting;
    if (next)
        next->prev = inserting;
    return token;
}

static void remove_token(preprocessor_token_t* token)
{
    if (!token) return;
    if (token->prev)
        token->prev->next = token->next;
    if (token->next)
        token->next->prev = token->prev;
    pp_token_delete(token);
}

// the token returned is the token at the start of the expansion
// expands recursively
// TODO: handle function-like macros and __VA_ARGS__
static preprocessor_token_t* expand_if_possible(preprocessing_state_t* state, preprocessor_token_t* token)
{
    if (!token || token->type != PPT_IDENTIFIER)
        return NULL;
    preprocessor_token_t* tokens = preprocessing_table_get(state->table, token->identifier);
    if (!tokens)
        return NULL;
    preprocessor_token_t* start = NULL;
    for (preprocessor_token_t* inserting = token; tokens; tokens = tokens->next)
    {
        inserting = insert_token_after(pp_token_copy(tokens), inserting);
        inserting = expand_if_possible(state, inserting);
        if (!start)
            start = inserting;
    }
    remove_token(token);
    return start;
}

static bool can_start_control_line(preprocessing_state_t* state, preprocessor_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    // null directive
    if (is_whitespace_ending_newline(token->next))
        return true;
    
    advance_token;

    return is_identifier(token, "include") ||
        is_identifier(token, "define") ||
        is_identifier(token, "undef") ||
        is_identifier(token, "line") ||
        is_identifier(token, "error") ||
        is_identifier(token, "pragma");
}

static bool can_start_if_section(preprocessing_state_t* state, preprocessor_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    advance_token;

    return is_identifier(token, "if") ||
        is_identifier(token, "ifdef") ||
        is_identifier(token, "ifndef");
}

static bool can_start_non_directive(preprocessing_state_t* state, preprocessor_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    advance_token;

    if (!token)
        return false;
    
    return true;
}

static bool can_start_text_line(preprocessing_state_t* state, preprocessor_token_t* token)
{
    for (; !is_whitespace_ending_newline(token); advance_token_list);
    return token;
}

static bool can_start_group_part(preprocessing_state_t* state, preprocessor_token_t* token)
{
    return can_start_control_line(state, token) ||
        can_start_if_section(state, token) ||
        can_start_non_directive(state, token) ||
        can_start_text_line(state, token);
}

pp_status_code_t preprocess_include_line(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;

    // pass "include"
    advance_token;

    if (!is_header_name(token))
        // TODO
        return fail_preprocess(token, "#include with preprocessor tokens is not supported yet");
    
    char* filename = token->header_name.name;
    bool quote_delimited = token->header_name.quote_delimited;

    // pass <...> or "..." to get to whitespace
    advance_token_list;

    if (!is_whitespace_ending_newline(token))
        return fail_preprocess(token, "expected newline to end include directive");
    
    if (quote_delimited)
    {
        fopen(filename, "r");
    }
    
    return found_status;
}

pp_status_code_t preprocess_control_line(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    if (!is_punctuator(token, P_HASH))
        return fail_preprocess(token, "expected '#' to begin controlling preprocessing directive");
    advance_token;
    if (is_identifier(token, "include") && preprocess_include_line(state, &token, EXPECTED) == ABORT)
        return fail_status;
}

pp_status_code_t preprocess_if_section(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    return found_status;
}

pp_status_code_t preprocess_non_directive(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    return found_status;
}

pp_status_code_t preprocess_text_line(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    return found_status;
}

pp_status_code_t preprocess_group_part(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    if (can_start_control_line(state, token) && preprocess_control_line(state, &token, EXPECTED) == ABORT)
        return fail_status;
    if (can_start_if_section(state, token) && preprocess_if_section(state, &token, EXPECTED) == ABORT)
        return fail_status;
    if (can_start_non_directive(state, token) && preprocess_non_directive(state, &token, EXPECTED) == ABORT)
        return fail_status;
    if (can_start_text_line(state, token) && preprocess_text_line(state, &token, EXPECTED) == ABORT)
        return fail_status;
    return found_status;
}

pp_status_code_t preprocess_preprocessing_file(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    while (can_start_group_part(state, token))
        if (preprocess_group_part(state, &token, EXPECTED) == ABORT)
            return fail_status;
    return found_status;
}

void preprocess(preprocessor_token_t* tokens)
{
    preprocessing_state_t* state = calloc(1, sizeof *state);
    state->tokens = tokens;
    state->table = preprocessing_table_init();
    preprocessor_token_t* tmp = calloc(1, sizeof *tmp);
    tmp->type = PPT_PP_NUMBER;
    tmp->row = tmp->col = 0;
    tmp->pp_number = strdup("1");
    preprocessing_table_add(state->table, "__STDC__", tmp, tmp);
    preprocess_preprocessing_file(state, &tokens, EXPECTED);
    state_delete(state);
}