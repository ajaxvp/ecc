#include <stdlib.h>
#include <string.h>

#include "cc.h"

typedef struct preprocessing_state
{
    // some type of symbol table?
    preprocessor_token_t* tokens;
} preprocessing_state_t;

void state_delete(preprocessing_state_t* state)
{
    if (!state) return;
    free(state);
}

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

// goes to the next non-whitespace and non-comment token (since
// technically whitespace and comments are not preprocessing tokens)
#define advance_token do token = token->next; while (token && (token->type == PPT_WHITESPACE || token->type == PPT_COMMENT));

// goes to the next element of the token list no matter what it is
#define advance_token_list token = (token ? token->next : NULL)

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
    if (!token) return false;
    return token->type != PPT_WHITESPACE && token->type != PPT_COMMENT;
}

static bool is_whitespace(preprocessor_token_t* token)
{
    return token && token->type == PPT_WHITESPACE;
}

static bool is_whitespace_ending_newline(preprocessor_token_t* token)
{
    return is_whitespace(token) && ends_with_ignore_case(token->whitespace, "\n");
}

static bool can_start_control_line(preprocessing_state_t* state, preprocessor_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    // TODO: check that hash is first ting

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
    
    // TODO: check that hash is first ting

    advance_token;

    return is_identifier(token, "if") ||
        is_identifier(token, "ifdef") ||
        is_identifier(token, "ifndef");
}

static bool can_start_non_directive(preprocessing_state_t* state, preprocessor_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    // TODO: check that hash is first ting
}

static bool can_start_text_line(preprocessing_state_t* state, preprocessor_token_t* token)
{
    for (; !is_whitespace_ending_newline(token); advance_token_list);
    return token;
}

static bool can_start_group_part(preprocessing_state_t* state, preprocessor_token_t* token)
{
    return can_start_control_line(state, token) ||
        can_start_if_section(state, token);
}

pp_status_code_t preprocess_group_part(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
}

pp_status_code_t preprocess_preprocessing_file(preprocessing_state_t* state, preprocessor_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    while (can_start_group_part(state, token))
        preprocess_group_part(state, &token, EXPECTED);
}

void preprocess(preprocessor_token_t* tokens)
{
    preprocessing_state_t* state = calloc(1, sizeof *state);
    state->tokens = tokens;
    preprocess_preprocessing_file(state, &tokens, EXPECTED);
    state_delete(state);
}