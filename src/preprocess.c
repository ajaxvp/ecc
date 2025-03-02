#include <stdlib.h>
#include <string.h>

#include "ecc.h"

typedef struct preprocessing_table
{
    char** key;
    preprocessing_token_t** v_repl_list;
    vector_t** v_id_list;
    unsigned size;
    unsigned capacity;
} preprocessing_table_t;

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

typedef enum pp_request_code
{
    UNKNOWN_REQUEST = 1,
    CHECK,
    OPTIONAL,
    EXPECTED
} pp_request_code_t;

#define init_preprocess preprocessing_token_t* token = *tokens;

// goes to the next non-whitespace token (since
// technically whitespace are not preprocessing tokens)
#define advance_token do token = token->next; while (token && token->type == PPT_WHITESPACE);

// goes to the next element of the token list no matter what it is
#define advance_token_list token = (token ? token->next : NULL)

#define fail_status (req == OPTIONAL ? NOT_FOUND : ABORT)
#define fail_preprocess(token, fmt, ...) (token ? snerrorf(state->settings->error, MAX_ERROR_LENGTH, "[%s:%d:%d] " fmt "\n", get_file_name(state->settings->filepath, false), token->row, token->col, ## __VA_ARGS__) : snerrorf(state->settings->error, MAX_ERROR_LENGTH, "[%s:?:?] " fmt "\n", get_file_name(state->settings->filepath, false), ## __VA_ARGS__), fail_status)

#define found_status (*tokens = token, FOUND)

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

void preprocessing_table_get_internal(preprocessing_table_t* t, char* k, int* i, preprocessing_token_t** token, vector_t** id_list)
{
    if (!t) return;
    unsigned long index = hash(k) % t->capacity;
    unsigned long oidx = index;
    do
    {
        if (t->key[index] != NULL && !strcmp(t->key[index], k))
        {
            if (i) *i = index;
            if (token) *token = t->v_repl_list[index];
            if (id_list) *id_list = t->v_id_list[index];
            break;
        }
        index = (index + 1 == t->capacity ? 0 : index + 1);
    }
    while (index != oidx);
    if (i) *i = -1;
}

void preprocessing_table_get(preprocessing_table_t* t, char* k, preprocessing_token_t** token, vector_t** id_list)
{
    preprocessing_table_get_internal(t, k, NULL, token, id_list);
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

static bool is_pp_token(preprocessing_token_t* token)
{
    return token && token->type != PPT_WHITESPACE;
}

static bool is_whitespace(preprocessing_token_t* token)
{
    return token && token->type == PPT_WHITESPACE;
}

static bool is_whitespace_ending_newline(preprocessing_token_t* token)
{
    return is_whitespace(token) && ends_with_ignore_case(token->whitespace, "\n");
}

static bool is_header_name(preprocessing_token_t* token)
{
    return token && token->type == PPT_HEADER_NAME;
}

static bool is_identifier_type(preprocessing_token_t* token)
{
    return token && token->type == PPT_IDENTIFIER;
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

// given the starting # of a preprocessor directive, delete it
// assumes that it's a preprocessor directive
static preprocessing_token_t* remove_preprocessor_directive(preprocessing_token_t* token)
{
    while (token && !is_whitespace_ending_newline(token))
        token = remove_token(token);
    if (!token) return NULL;
    return token = remove_token(token);
}

// the token returned is the token at the start of the expansion
// expands recursively
// TODO: handle function-like macros and __VA_ARGS__
static preprocessing_token_t* expand_if_possible(preprocessing_state_t* state, preprocessing_token_t* token)
{
    if (!token || token->type != PPT_IDENTIFIER)
        return NULL;
    preprocessing_token_t* tokens = NULL;
    vector_t* id_list = NULL;
    preprocessing_table_get(state->table, token->identifier, &tokens, &id_list);
    if (!tokens)
        return NULL;
    preprocessing_token_t* start = NULL;
    for (preprocessing_token_t* inserting = token; tokens; tokens = tokens->next)
    {
        inserting = insert_token_after(pp_token_copy(tokens), inserting);
        inserting = expand_if_possible(state, inserting);
        if (!start)
            start = inserting;
    }
    remove_token(token);
    return start;
}

static bool can_start_control_line(preprocessing_state_t* state, preprocessing_token_t* token)
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

static bool can_start_if_section(preprocessing_state_t* state, preprocessing_token_t* token)
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

static bool can_start_non_directive(preprocessing_state_t* state, preprocessing_token_t* token)
{
    if (!is_punctuator(token, P_HASH))
        return false;
    
    if (!token->can_start_directive)
        return false;

    advance_token;

    if (!is_pp_token(token))
        return false;
    
    return true;
}

static bool can_start_text_line(preprocessing_state_t* state, preprocessing_token_t* token)
{
    for (; token && !is_whitespace_ending_newline(token); advance_token_list);
    return token;
}

static bool can_start_group_part(preprocessing_state_t* state, preprocessing_token_t* token)
{
    return can_start_control_line(state, token) ||
        can_start_if_section(state, token) ||
        can_start_non_directive(state, token) ||
        can_start_text_line(state, token);
}

pp_status_code_t preprocess_include_line(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;

    // pass "include"
    advance_token;

    if (!is_header_name(token))
        // TODO
        return fail_preprocess(token, "#include with preprocessor tokens is not supported yet");
    
    preprocessing_token_t* header_name = token;
    char* filename = token->header_name.name;
    bool quote_delimited = token->header_name.quote_delimited;

    // pass <...> or "..." to get to whitespace
    advance_token_list;

    if (!is_whitespace_ending_newline(token))
        return fail_preprocess(token, "expected newline to end include directive");
    
    FILE* file = NULL;
    char path[LINUX_MAX_PATH_LENGTH];

    if (quote_delimited)
    {
        char* dirpath = get_directory_path(state->settings->filepath);
        snprintf(path, LINUX_MAX_PATH_LENGTH, "%s/%s", dirpath, filename);
        free(dirpath);
        file = fopen(path, "r");
    }

    if (!file)
    {
        for (int i = 0; i < sizeof(ANGLED_INCLUDE_SEARCH_DIRECTORIES) / sizeof(ANGLED_INCLUDE_SEARCH_DIRECTORIES[0]); ++i)
        {
            snprintf(path, LINUX_MAX_PATH_LENGTH, "%s/%s", ANGLED_INCLUDE_SEARCH_DIRECTORIES[i], filename);
            file = fopen(path, "r");
            if (file)
                break;
        }
    }

    if (!file)
        return fail_preprocess(header_name, "could not find file '%s'", filename);
    
    preprocessing_token_t* subtokens = lex(file, true);
    fclose(file);
    if (!subtokens)
        return fail_status;
    preprocessing_settings_t settings;
    settings.filepath = path;
    char suberror[MAX_ERROR_LENGTH];
    settings.error = suberror;
    if (!preprocess(&subtokens, &settings))
        return fail_preprocess(token, "%s", settings.error); // hmmmm
    for (preprocessing_token_t* subtoken = subtokens; subtoken; subtoken = subtoken->next)
        token = insert_token_after(pp_token_copy(subtoken), token);

    pp_token_delete_all(subtokens);
    
    return found_status;
}

pp_status_code_t preprocess_define_line(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    
    // pass "define"
    advance_token;

    if (!is_identifier_type(token))
        return fail_preprocess(token, "expected identifier for macro replacement definition");
    
    preprocessing_token_t* macro_name_token = token;
    char* macro_name = token->identifier;

    advance_token;

    vector_t* id_list = NULL;
    
    if (is_punctuator(token, P_LEFT_PARENTHESIS) && !is_whitespace(token->prev))
    {
        id_list = vector_init();
        advance_token;
        while (!is_punctuator(token, P_RIGHT_PARENTHESIS))
        {
            // special case for variadic function-like macros
            if (is_punctuator(token, P_ELLIPSIS))
            {
                vector_add(id_list, strdup("__VA_ARGS__"));
                advance_token;
                if (!is_punctuator(token, P_RIGHT_PARENTHESIS))
                    return fail_preprocess(token, "macro parameter list with ellipsis must end with that ellipsis");
                break;
            }
            if (!is_identifier_type(token))
                return fail_preprocess(token, "expected macro parameter list to end with ) or continue with more identifiers");
            vector_add(id_list, strdup(token->identifier));
            advance_token;
            if (is_punctuator(token, P_COMMA))
            {
                advance_token;
                if (is_punctuator(token, P_RIGHT_PARENTHESIS))
                    return fail_preprocess(token, "expected macro parameter list to end with an identifier or an ellipsis");
            }
        }
        advance_token_list;
    }

    preprocessing_token_t* start = token;
    bool no_repl_list = is_whitespace_ending_newline(start);

    for (; token && !is_whitespace_ending_newline(token); advance_token_list);
    if (!token)
        return fail_preprocess(macro_name_token, "macro definition must end with a newline");

    preprocessing_table_add(state->table, macro_name, no_repl_list ? NULL : start, token, id_list);

    advance_token_list;
    return found_status;
}

pp_status_code_t preprocess_undef_line(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;

    // pass "undef"
    advance_token;

    if (!is_identifier_type(token))
        return fail_preprocess(token, "expected identifier to undefine as a macro");
    
    preprocessing_token_t* macro_name_token = token;
    
    preprocessing_table_remove(state->table, token->identifier);
    advance_token_list;

    if (!is_whitespace_ending_newline(token))
        return fail_preprocess(macro_name_token, "macro undefinition must end with a newline");
    
    advance_token_list;
    return found_status;
}

pp_status_code_t preprocess_error_line(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;

    preprocessing_token_t* hash = token->prev;

    // pass "error"
    advance_token_list;

    for (; token && !is_whitespace_ending_newline(token); advance_token_list);
    if (!token)
        return fail_preprocess(hash, "error directive must end with a newline");
    
    quickbuffer_setup(MAX_ERROR_LENGTH - 17);
    pp_token_normal_print_range(hash, token, quickbuffer_printf);
    pp_status_code_t code = fail_preprocess(hash, "error directive: %s", quickbuffer());
    quickbuffer_release();
    advance_token_list;
    return code;
}

pp_status_code_t preprocess_control_line(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    if (!is_punctuator(token, P_HASH))
        return fail_preprocess(token, "expected '#' to begin controlling preprocessing directive");
    preprocessing_token_t* hash = token;
    advance_token;
    if (is_identifier(token, "include"))
    {
        if (preprocess_include_line(state, &token, EXPECTED) == FOUND)
        {
            remove_preprocessor_directive(hash);
            return found_status;
        }
    }
    else if (is_identifier(token, "define"))
    {
        if (preprocess_define_line(state, &token, EXPECTED) == FOUND)
        {
            remove_preprocessor_directive(hash);
            return found_status;
        }
    }
    else if (is_identifier(token, "undef"))
    {
        if (preprocess_undef_line(state, &token, EXPECTED) == FOUND)
        {
            remove_preprocessor_directive(hash);
            return found_status;
        }
    }
    else if (is_identifier(token, "error"))
    {
        if (preprocess_error_line(state, &token, EXPECTED) == FOUND)
        {
            remove_preprocessor_directive(hash);
            return found_status;
        }
    }
    return fail_status;
}

pp_status_code_t preprocess_if_section(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    return found_status;
}

pp_status_code_t preprocess_non_directive(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    return found_status;
}

pp_status_code_t preprocess_text_line(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    for (; token && !is_whitespace_ending_newline(token); advance_token_list);
    if (!token)
        return fail_preprocess(token, "file must end with a newline");
    advance_token_list;
    return found_status;
}

pp_status_code_t preprocess_group_part(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    if (can_start_control_line(state, token))
        return preprocess_control_line(state, &token, EXPECTED) == FOUND ? found_status : fail_status;
    if (can_start_if_section(state, token))
        return preprocess_if_section(state, &token, EXPECTED) == FOUND ? found_status : fail_status;
    if (can_start_non_directive(state, token))
        return preprocess_non_directive(state, &token, EXPECTED) == FOUND ? found_status : fail_status;
    if (can_start_text_line(state, token))
        return preprocess_text_line(state, &token, EXPECTED) == FOUND ? found_status : fail_status;
    return fail_status;
}

pp_status_code_t preprocess_preprocessing_file(preprocessing_state_t* state, preprocessing_token_t** tokens, pp_request_code_t req)
{
    init_preprocess;
    while (can_start_group_part(state, token))
        if (preprocess_group_part(state, &token, EXPECTED) == ABORT)
            return fail_status;
    return found_status;
}

// translation phase 4
bool preprocess(preprocessing_token_t** tokens, preprocessing_settings_t* settings)
{
    preprocessing_state_t* state = calloc(1, sizeof *state);

    preprocessing_token_t* dummy = calloc(1, sizeof *dummy);
    dummy->type = PPT_OTHER;
    (*tokens)->prev = dummy;
    dummy->next = *tokens;

    state->tokens = *tokens;
    state->table = preprocessing_table_init();
    state->settings = settings;
    preprocessing_token_t* tmp = calloc(1, sizeof *tmp);
    tmp->type = PPT_PP_NUMBER;
    tmp->row = tmp->col = 0;
    tmp->pp_number = strdup("1");
    preprocessing_table_add(state->table, "__STDC__", tmp, NULL, NULL);
    preprocessing_token_t* ts = *tokens;
    pp_status_code_t code = preprocess_preprocessing_file(state, &ts, EXPECTED);
    if (get_program_options()->iflag)
        preprocessing_table_print(state->table, printf);
    state_delete(state);

    *tokens = dummy->next;
    pp_token_delete(dummy);
    if (*tokens)
        (*tokens)->prev = NULL;

    return code == FOUND;
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