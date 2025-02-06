#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cc.h"

typedef struct lex_state
{
    unsigned char* data;
    size_t length;
    long long cursor;
    unsigned row, col;
    bool counting;
    int counter;
    char* error;
    int include_condition;
    preprocessing_token_t* prev;
} lex_state_t;

void lex_state_delete(lex_state_t* state)
{
    if (!state) return;
    free(state->data);
    free(state->error);
    free(state);
}

static int read_impl(lex_state_t* state)
{
    if (state->cursor >= state->length)
        return EOF;

    if (state->cursor + 1 < state->length &&
        state->data[state->cursor] == '\\' &&
        state->data[state->cursor + 1] == '\n')
        state->cursor += 2;
    
    if (state->cursor >= 1 && state->data[state->cursor - 1] == '\n')
    {
        ++state->row;
        state->col = 1;
    }
    else
        ++state->col;
    
    if (state->cursor + 2 < state->length &&
        state->data[state->cursor] == '?' &&
        state->data[state->cursor + 1] == '?')
    {
        #define trigraph_select(c, d) \
            case c: return state->col += 2, state->cursor += 3, d;
        switch (state->data[state->cursor + 2])
        {
            trigraph_select('=', '#')
            trigraph_select('(', '[')
            trigraph_select('/', '\\')
            trigraph_select(')', ']')
            trigraph_select('\'', '^')
            trigraph_select('<', '{')
            trigraph_select('!', '|')
            trigraph_select('>', '}')
            trigraph_select('-', '~')
            default: break;
        }
        #undef trigraph_select
    }

    return state->data[state->cursor++];
}

static unsigned get_column(lex_state_t* state, long long cursor)
{
    long long orig = cursor;
    for (--cursor; cursor >= 0 && state->data[cursor] != '\n'; --cursor);
    if (cursor == -1)
        return 1;
    return orig - cursor;
}

static bool unread_impl(lex_state_t* state)
{
    if (state->cursor <= 0)
    {
        state->cursor = 0;
        return false;
    }
    int c = state->data[--state->cursor];
    if (state->cursor >= 2 &&
        state->data[state->cursor - 1] == '?' &&
        state->data[state->cursor - 2] == '?' &&
        (c == '=' ||
        c == '(' ||
        c == '/' ||
        c == ')' ||
        c == '\'' ||
        c == '<' ||
        c == '!' ||
        c == '>' ||
        c == '-'))
    {
        state->cursor -= 2;
        state->col -= 2;
    }

    if (c == '\n' && state->cursor >= 1 && state->data[state->cursor - 1] == '\\')
        --state->cursor;
    if (state->cursor >= 1 && state->data[state->cursor - 1] == '\n')
    {
        --state->row;
        state->col = get_column(state, state->cursor);
    }
    else
        --state->col;
    return true;
}

static bool jump_impl(lex_state_t* state, long long cursor, unsigned row, unsigned col)
{
    state->cursor = cursor;
    state->row = row;
    state->col = col;
    return true;
}

static preprocessing_token_t* add_token(preprocessing_token_t* head, preprocessing_token_t* token)
{
    preprocessing_token_t* orig = head;
    if (!head) return token;
    for (; head->next; head = head->next);
    head->next = token;
    return orig;
}

static bool in_source_charset(int c)
{
    return c < 128;
}

static bool is_nondigit(int c)
{
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

static bool is_digit(int c)
{
    return c >= '0' && c <= '9';
}

static bool is_octal_digit(int c)
{
    return c >= '0' && c <= '7';
}

static bool is_hexadecimal_digit(int c)
{
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool is_whitespace(int c)
{
    return c == ' ' || c == '\t' || c == '\v' || c == '\n' || c == '\f';
}

void pp_token_delete(preprocessing_token_t* token)
{
    if (!token) return;
    switch (token->type)
    {
        case PPT_STRING_LITERAL:
            free(token->string_literal.value);
            break;
        case PPT_IDENTIFIER:
            free(token->identifier);
            break;
        case PPT_PP_NUMBER:
            free(token->pp_number);
            break;
        case PPT_HEADER_NAME:
            free(token->header_name.name);
            break;
        case PPT_WHITESPACE:
            free(token->whitespace);
            break;
        default:
            break;
    }
    free(token);
}

void pp_token_delete_all(preprocessing_token_t* tokens)
{
    if (!tokens) return;
    pp_token_delete_all(tokens->next);
    pp_token_delete(tokens);
}

void pp_token_print(preprocessing_token_t* token, int (*printer)(const char* fmt, ...))
{
    if (!token) return;
    printer("preprocessor token { type: %s, line: %u, column: %u", PP_TOKEN_NAMES[token->type], token->row, token->col);
    if (token->can_start_directive)
        printer(", can start preprocessor directive");
    switch (token->type)
    {
        case PPT_HEADER_NAME:
        {
            printer(", name: \"%s\", quote delimited: %s", token->header_name.name, BOOL_NAMES[token->header_name.quote_delimited]);
            break;
        }
        case PPT_IDENTIFIER:
        {
            printer(", identifier: \"%s\"", token->identifier);
            break;
        }
        case PPT_PP_NUMBER:
        {
            printer(", pp number: %s", token->pp_number);
            break;
        }
        case PPT_CHARACTER_CONSTANT:
        {
            printer(", value: %s, wide: %s", token->character_constant.value, BOOL_NAMES[token->character_constant.wide]);
            break;
        }
        case PPT_STRING_LITERAL:
        {
            printer(", value: \"%s\", wide: %s", token->string_literal.value, BOOL_NAMES[token->string_literal.wide]);
            break;
        }
        case PPT_PUNCTUATOR:
        {
            printer(", punctuator: %s", PUNCTUATOR_STRING_REPRS[token->punctuator]);
            break;
        }
        case PPT_OTHER:
        {
            printer(", value: %c", token->other);
            break;
        }
        case PPT_WHITESPACE:
        {
            printer(", whitespace: \"");
            repr_print(token->whitespace, printer);
            printer("\"");
            break;
        }
        default:
            break;
    }
    printer(" }");
}

void pp_token_normal_print(preprocessing_token_t* token, int (*printer)(const char* fmt, ...))
{
    switch (token->type)
    {
        case PPT_HEADER_NAME:
        {
            if (token->header_name.quote_delimited)
                printer("\"%s\"", token->header_name.name);
            else
                printer("<%s>", token->header_name.name);
            break;
        }
        case PPT_IDENTIFIER:
            printer("%s", token->identifier);
            break;
        case PPT_PP_NUMBER:
            printer("%s", token->pp_number);
            break;
        case PPT_CHARACTER_CONSTANT:
            printer("%s%s", token->character_constant.wide ? "L" : "", token->character_constant.value);
            break;
        case PPT_STRING_LITERAL:
            printer("%s\"%s\"", token->string_literal.wide ? "L" : "", token->string_literal.value);
            break;
        case PPT_PUNCTUATOR:
            printer("%s", PUNCTUATOR_STRING_REPRS[token->punctuator]);
            break;
        case PPT_OTHER:
            printer("%c", token->other);
            break;
        case PPT_WHITESPACE:
            printer("%s", token->whitespace);
            break;
        default:
            break;
    }
}

// print how code would normally be written from token (inclusive) to end (exclusive) or if end is NULL, till the end
void pp_token_normal_print_range(preprocessing_token_t* token, preprocessing_token_t* end, int (*printer)(const char* fmt, ...))
{
    for (; token && token != end; token = token->next)
        pp_token_normal_print(token, printer);
}

// does not recursively copy if in a linked list structure
preprocessing_token_t* pp_token_copy(preprocessing_token_t* token)
{
    if (!token) return NULL;
    preprocessing_token_t* n = calloc(1, sizeof *n);
    n->type = token->type;
    n->row = token->row;
    n->col = token->col;
    n->prev = token->prev;
    n->next = token->next;
    switch (token->type)
    {
        case PPT_HEADER_NAME:
        {
            n->header_name.name = strdup(token->header_name.name);
            n->header_name.quote_delimited = token->header_name.quote_delimited;
            break;
        }
        case PPT_IDENTIFIER:
        {
            n->identifier = strdup(token->identifier);
            break;
        }
        case PPT_PP_NUMBER:
        {
            n->pp_number = strdup(token->pp_number);
            break;
        }
        case PPT_CHARACTER_CONSTANT:
        {
            n->character_constant.value = strdup(token->character_constant.value);
            n->character_constant.wide = token->character_constant.wide;
            break;
        }
        case PPT_STRING_LITERAL:
        {
            n->string_literal.value = strdup(token->string_literal.value);
            n->string_literal.wide = token->string_literal.wide;
            break;
        }
        case PPT_PUNCTUATOR:
        {
            n->punctuator = token->punctuator;
            break;
        }
        case PPT_OTHER:
        {
            n->other = token->other;
            break;
        }
        case PPT_WHITESPACE:
        {
            n->whitespace = strdup(token->whitespace);
            break;
        }
        default:
            break;
    }
    return n;
}

// copies from start inclusive to end exclusive
// NULL will copy till the end of the token sequence
preprocessing_token_t* pp_token_copy_range(preprocessing_token_t* start, preprocessing_token_t* end)
{
    preprocessing_token_t* prev = NULL;
    preprocessing_token_t* nstart = NULL;
    while (start && start != end)
    {
        preprocessing_token_t* token = pp_token_copy(start);
        if (!nstart)
            nstart = token;
        token->prev = prev;
        token->next = NULL;
        if (prev)
            prev->next = token;
        prev = token;
        start = start->next;
    }
    return nstart;
}

typedef preprocessing_token_t* (*lex_function)(lex_state_t* state);

#define create_jump(x) int x##_cursor = state->cursor; unsigned x##_row = state->row; unsigned x##_col = state->col;
#define jump(x) jump_impl(state, x##_cursor, x##_row, x##_col)
#define init_base int c = 0, p = 0; create_jump(o)
#define init_helper init_base
#define init_lex(t) \
    init_base \
    state->counter = 0; \
    preprocessing_token_t* token = calloc(1, sizeof *token); \
    token->type = t; \
    p = read_impl(state); \
    token->row = state->row; \
    token->col = state->col; \
    p != EOF ? unread_impl(state) : false;
#define cleanup_retreat jump(o)
#define cleanup_helper cleanup_retreat
#define cleanup_lex_fail cleanup_retreat, pp_token_delete(token)
#define cleanup_lex_pass state->counting ? (cleanup_lex_fail) : (void) 0
#define read (++state->counter, c = read_impl(state))
#define unread (--state->counter, unread_impl(state))
#define peek (p = read_impl(state), p != EOF ? unread_impl(state) : false, p)
#define SET_ERROR(fmt, ...) snerrorf(state->error, MAX_ERROR_LENGTH, "[%u:%u] " fmt "\n", state->row, state->col, ## __VA_ARGS__)

preprocessing_token_t* lex_header_name(lex_state_t* state)
{
    init_lex(PPT_HEADER_NAME);
    if (peek != '<' && peek != '"')
    {
        cleanup_lex_fail;
        SET_ERROR("expected '<' or '\"' for start of header name");
        return NULL;
    }
    read;
    int ending = c == '<' ? '>' : '"';
    buffer_t* buf = buffer_init();
    while (in_source_charset(read) && c != '\n' && c != ending)
        buffer_append(buf, c);
    if (c != ending)
    {
        cleanup_lex_fail;
        buffer_delete(buf);
        SET_ERROR("expected '%c' for end of header name", ending);
        return NULL;
    }
    token->header_name.name = buffer_export(buf);
    buffer_delete(buf);
    token->header_name.quote_delimited = ending == '"';
    cleanup_lex_pass;
    return token;
}

// gives back a string of form "\uXXXX" or "\UXXXXXXXX"
char* lex_universal_character(lex_state_t* state, preprocessing_token_t* token)
{
    init_helper;
    buffer_t* buf = buffer_init();
    if (peek != '\\')
    {
        cleanup_helper;
        buffer_delete(buf);
        SET_ERROR("expected universal character of form '\\uXXXX' or '\\UXXXXXXXX'");
        return NULL;
    }
    buffer_append(buf, read);
    if (peek != 'u' && peek != 'U')
    {
        cleanup_helper;
        buffer_delete(buf);
        SET_ERROR("expected universal character of form '\\uXXXX' or '\\UXXXXXXXX'");
        return NULL;
    }
    int type = read;
    buffer_append(buf, type);
    int i = 0;
    for (; i < 4 && is_hexadecimal_digit(peek); ++i)
        buffer_append(buf, read);
    if (i != 4)
    {
        cleanup_helper;
        buffer_delete(buf);
        SET_ERROR("universal character should have %d hex digits with '\\%c' prefix", type == 'u' ? 4 : 8, type);
        return NULL;
    }
    if (type == 'U')
    {
        for (; i < 8 && is_hexadecimal_digit(peek); ++i)
            buffer_append(buf, read);
        if (i != 8)
        {
            cleanup_helper;
            buffer_delete(buf);
            SET_ERROR("universal character should have 8 hex digits with '\\U' prefix");
        }
    }
    char* unichar = buffer_export(buf);
    unsigned value = get_universal_character_hex_value(unichar, strlen(unichar));
    buffer_delete(buf);
    if ((value < 0x00A0U && value != 0x0024U && value != 0x0040U && value != 0x0060U) || (value >= 0xD800U && value <= 0xDFFFU))
    {
        cleanup_helper;
        free(unichar);
        // ISO: 6.4.3 (2)
        SET_ERROR("invalid universal character name");
        return NULL;
    }
    return unichar;
}

preprocessing_token_t* lex_identifier(lex_state_t* state)
{
    init_lex(PPT_IDENTIFIER);
    buffer_t* buf = buffer_init();
    for (;;)
    {
        if (is_nondigit(peek))
        {
            buffer_append(buf, read);
            continue;
        }
        if (buf->size >= 1 && is_digit(peek))
        {
            buffer_append(buf, read);
            continue;
        }
        if (peek == '\\')
        {
            char* unichar = lex_universal_character(state, token);
            if (!unichar)
            {
                cleanup_lex_fail;
                buffer_delete(buf);
                // error provided by helper function
                free(unichar);
                return NULL;
            }
            buffer_append_str(buf, unichar);
            free(unichar);
            continue;
        }
        break;
    }
    if (!buf->size)
    {
        cleanup_lex_fail;
        buffer_delete(buf);
        SET_ERROR("identifier cannot be empty");
        return NULL;
    }
    token->identifier = buffer_export(buf);
    buffer_delete(buf);
    if (!strcmp(token->identifier, "include"))
        ++state->include_condition;
    cleanup_lex_pass;
    return token;
}

preprocessing_token_t* lex_pp_number(lex_state_t* state)
{
    init_lex(PPT_PP_NUMBER);
    buffer_t* buf = buffer_init();
    if (peek == '.')
        buffer_append(buf, read);
    if (!is_digit(peek))
    {
        cleanup_lex_fail;
        buffer_delete(buf);
        SET_ERROR("expected at least one digit in number");
        return NULL;
    }
    read;
    buffer_append(buf, c);
    for (;;)
    {
        if (is_digit(peek))
        {
            buffer_append(buf, read);
            continue;
        }
        if (peek == '.')
        {
            buffer_append(buf, read);
            continue;
        }
        if (peek == 'e' || peek == 'E' ||
            peek == 'p' || peek == 'P')
        {
            buffer_append(buf, read);
            if (peek == '+' || peek == '-')
                buffer_append(buf, read);
            continue;
        }
        if (is_nondigit(peek))
        {
            buffer_append(buf, read);
            continue;
        }
        if (peek == '\\')
        {
            char* unichar = lex_universal_character(state, token);
            if (!unichar)
            {
                cleanup_lex_fail;
                buffer_delete(buf);
                // error provided by helper function
                free(unichar);
                return NULL;
            }
            buffer_append_str(buf, unichar);
            free(unichar);
            continue;
        }
        break;
    }
    token->pp_number = buffer_export(buf);
    buffer_delete(buf);
    cleanup_lex_pass;
    return token;
}

preprocessing_token_t* lex_character_constant(lex_state_t* state)
{
    init_lex(PPT_CHARACTER_CONSTANT);
    if (peek == 'L')
    {
        read;
        token->character_constant.wide = true;
    }
    if (peek != '\'')
    {
        cleanup_lex_fail;
        SET_ERROR("expected single quotes enclosing character constant");
        return NULL;
    }
    buffer_t* buf = buffer_init();
    buffer_append(buf, read);
    if (peek == '\'')
    {
        cleanup_lex_fail;
        buffer_delete(buf);
        SET_ERROR("character constant cannot be empty");
        return NULL;
    }
    for (;;)
    {
        if (peek == '\'')
        {
            buffer_append(buf, read);
            break;
        }
        if (peek == '\n')
        {
            cleanup_lex_fail;
            buffer_delete(buf);
            SET_ERROR("newlines not allowed in character constant");
            return NULL;
        }
        if (peek == '\\')
        {
            buffer_append(buf, read);
            if (peek == '\'' ||
                peek == '"' ||
                peek == '?' ||
                peek == '\\' ||
                peek == 'a' ||
                peek == 'b' ||
                peek == 'f' ||
                peek == 'n' ||
                peek == 'r' ||
                peek == 't' ||
                peek == 'v')
                buffer_append(buf, read);
            else if (peek == 'x')
            {
                buffer_append(buf, read);
                int count = 0;
                for (; is_hexadecimal_digit(peek); ++count)
                    buffer_append(buf, read);
                if (!count)
                {
                    cleanup_lex_fail;
                    buffer_delete(buf);
                    // ISO: 6.4.4.4 (1)
                    SET_ERROR("hex escape sequence should have at least 1 hex digit");
                    return NULL;
                }
            }
            else if (is_octal_digit(peek))
            {
                buffer_append(buf, read);
                if (!is_octal_digit(peek))
                    continue;
                buffer_append(buf, read);
                if (!is_octal_digit(peek))
                    continue;
                buffer_append(buf, read);
            }
            else if (peek == 'u' || peek == 'U')
            {
                unread;
                buffer_pop(buf);
                char* unichar = lex_universal_character(state, token);
                if (!unichar)
                {
                    cleanup_lex_fail;
                    // error provided by helper function
                    buffer_delete(buf);
                    free(unichar);
                    return NULL;
                }
                buffer_append_str(buf, unichar);
                free(unichar);
            }
            else
            {
                unread;
                cleanup_lex_fail;
                buffer_delete(buf);
                SET_ERROR("invalid escape sequence in character constant");
                return NULL;
            }
            continue;
        }
        buffer_append(buf, read);
    }
    token->character_constant.value = buffer_export(buf);
    buffer_delete(buf);
    cleanup_lex_pass;
    return token;
}

preprocessing_token_t* lex_string_literal(lex_state_t* state)
{
    init_lex(PPT_STRING_LITERAL);
    if (peek == 'L')
    {
        read;
        token->string_literal.wide = true;
    }
    if (peek != '"')
    {
        cleanup_lex_fail;
        SET_ERROR("expected double quotes enclosing string literal");
        return NULL;
    }
    read;
    buffer_t* buf = buffer_init();
    for (;;)
    {
        if (peek == '"')
        {
            read;
            break;
        }
        if (peek == '\n')
        {
            cleanup_lex_fail;
            buffer_delete(buf);
            SET_ERROR("newlines not allowed in string literal");
            return NULL;
        }
        if (peek == '\\')
        {
            buffer_append(buf, read);
            if (peek == '\'' ||
                peek == '"' ||
                peek == '?' ||
                peek == '\\' ||
                peek == 'a' ||
                peek == 'b' ||
                peek == 'f' ||
                peek == 'n' ||
                peek == 'r' ||
                peek == 't' ||
                peek == 'v')
                buffer_append(buf, read);
            else if (peek == 'x')
            {
                buffer_append(buf, read);
                int count = 0;
                for (; is_hexadecimal_digit(peek); ++count)
                    buffer_append(buf, read);
                if (!count)
                {
                    cleanup_lex_fail;
                    buffer_delete(buf);
                    // ISO: 6.4.4.4 (1)
                    SET_ERROR("hex escape sequence should have at least 1 hex digit");
                    return NULL;
                }
            }
            else if (is_octal_digit(peek))
            {
                buffer_append(buf, read);
                if (!is_octal_digit(peek))
                    continue;
                buffer_append(buf, read);
                if (!is_octal_digit(peek))
                    continue;
                buffer_append(buf, read);
            }
            else if (peek == 'u' || peek == 'U')
            {
                unread;
                buffer_pop(buf);
                char* unichar = lex_universal_character(state, token);
                if (!unichar)
                {
                    cleanup_lex_fail;
                    // error provided by helper function
                    buffer_delete(buf);
                    free(unichar);
                    return NULL;
                }
                buffer_append_str(buf, unichar);
                free(unichar);
            }
            else
            {
                unread;
                cleanup_lex_fail;
                buffer_delete(buf);
                SET_ERROR("invalid escape sequence in character constant");
                return NULL;
            }
            continue;
        }
        buffer_append(buf, read);
    }
    token->string_literal.value = buffer_export(buf);
    buffer_delete(buf);
    cleanup_lex_pass;
    return token;
}

preprocessing_token_t* lex_punctuator(lex_state_t* state)
{
    init_lex(PPT_PUNCTUATOR);
    (void) c;
    #define single_check(c, punct) \
        if (peek == c) \
        { \
            read; \
            token->punctuator = punct; \
            cleanup_lex_pass; \
            return token; \
        }
    
    single_check('[', P_LEFT_BRACKET)
    single_check(']', P_RIGHT_BRACKET)
    single_check('(', P_LEFT_PARENTHESIS)
    single_check(')', P_RIGHT_PARENTHESIS)
    single_check('{', P_LEFT_BRACE)
    single_check('}', P_RIGHT_BRACE)

    // "." or "..."
    if (peek == '.')
    {
        read;
        if (peek == '.')
        {
            read;
            single_check('.', P_ELLIPSIS)
            else
                unread;
        }
        token->punctuator = P_PERIOD;
        cleanup_lex_pass;
        return token;
    }

    // "-", "--", "-=", or "->"
    if (peek == '-')
    {
        read;
        single_check('-', P_DECREMENT)
        single_check('=', P_SUB_ASSIGNMENT)
        single_check('>', P_DEREFERENCE_MEMBER)
        token->punctuator = P_MINUS;
        cleanup_lex_pass;
        return token;
    }

    // "+", "++", or "+="
    if (peek == '+')
    {
        read;
        single_check('+', P_INCREMENT)
        single_check('=', P_ADD_ASSIGNMENT)
        token->punctuator = P_PLUS;
        cleanup_lex_pass;
        return token;
    }

    // "&", "&&", or "&="
    if (peek == '&')
    {
        read;
        single_check('&', P_LOGICAL_AND)
        single_check('=', P_BITWISE_AND_ASSIGNMENT)
        token->punctuator = P_AND;
        cleanup_lex_pass;
        return token;
    }

    // "*" or "*="
    if (peek == '*')
    {
        read;
        single_check('=', P_MULTIPLY_ASSIGNMENT)
        token->punctuator = P_ASTERISK;
        cleanup_lex_pass;
        return token;
    }

    single_check('~', P_TILDE)

    // "!" or "!="
    if (peek == '!')
    {
        read;
        single_check('=', P_INEQUAL)
        token->punctuator = P_EXCLAMATION_POINT;
        cleanup_lex_pass;
        return token;
    }

    // "/" or "/="
    if (peek == '/')
    {
        read;
        single_check('=', P_DIVIDE_ASSIGNMENT)
        token->punctuator = P_SLASH;
        cleanup_lex_pass;
        return token;
    }

    // "%", "%=", "%>", "%:", or "%:%:"
    if (peek == '%')
    {
        read;
        single_check('=', P_MODULO_ASSIGNMENT)
        single_check('>', P_RIGHT_BRACE)
        if (peek == ':')
        {
            read;
            if (peek == '%')
            {
                read;
                if (peek == ':')
                {
                    read;
                    token->punctuator = P_DOUBLE_HASH;
                    cleanup_lex_pass;
                    return token;
                }
                else
                    unread;
            }
            token->punctuator = P_HASH;
            cleanup_lex_pass;
            return token;
        }
        token->punctuator = P_PERCENT;
        cleanup_lex_pass;
        return token;
    }

    // "<", "<<", "<=", "<%", "<<=", or "<:"
    if (peek == '<')
    {
        read;
        single_check('=', P_LESS_EQUAL)
        single_check('%', P_LEFT_BRACE)
        single_check(':', P_LEFT_BRACKET)
        if (peek == '<')
        {
            read;
            single_check('=', P_SHIFT_LEFT_ASSIGNMENT)
            token->punctuator = P_LEFT_SHIFT;
            cleanup_lex_pass;
            return token;
        }
        token->punctuator = P_LESS;
        cleanup_lex_pass;
        return token;
    }

    // ">", ">>", ">=", or ">>="
    if (peek == '>')
    {
        read;
        single_check('=', P_GREATER_EQUAL)
        if (peek == '>')
        {
            read;
            single_check('=', P_SHIFT_RIGHT_ASSIGNMENT)
            token->punctuator = P_RIGHT_SHIFT;
            cleanup_lex_pass;
            return token;
        }
        token->punctuator = P_GREATER;
        cleanup_lex_pass;
        return token;
    }

    // "=" or "=="
    if (peek == '=')
    {
        read;
        single_check('=', P_EQUAL)
        token->punctuator = P_ASSIGNMENT;
        cleanup_lex_pass;
        return token;
    }

    // "^" or "^="
    if (peek == '^')
    {
        read;
        single_check('=', P_BITWISE_XOR_ASSIGNMENT)
        token->punctuator = P_CARET;
        cleanup_lex_pass;
        return token;
    }

    // "|", "||", or "|="
    if (peek == '|')
    {
        read;
        single_check('=', P_BITWISE_OR_ASSIGNMENT)
        single_check('|', P_LOGICAL_OR)
        token->punctuator = P_PIPE;
        cleanup_lex_pass;
        return token;
    }

    if (peek == ':')
    {
        read;
        single_check('>', P_RIGHT_BRACKET)
        token->punctuator = P_COLON;
        cleanup_lex_pass;
        return token;
    }

    single_check(';', P_SEMICOLON)
    single_check(',', P_COMMA)
    if (peek == '#')
    {
        read;
        single_check('#', P_DOUBLE_HASH)
        ++state->include_condition;
        token->punctuator = P_HASH;
        if (!state->prev || (state->prev->type == PPT_WHITESPACE && (contains_substr(state->prev->whitespace, "\n") || state->prev->can_start_directive)))
            token->can_start_directive = true;
        cleanup_lex_pass;
        return token;
    }

    #undef single_check

    cleanup_lex_fail;
    SET_ERROR("unknown punctuator");
    return NULL;
}

preprocessing_token_t* lex_other(lex_state_t* state)
{
    init_lex(PPT_OTHER);
    if (is_whitespace(peek))
    {
        cleanup_lex_fail;
        return NULL;
    }
    token->other = read;
    cleanup_lex_pass;
    return token;
}

preprocessing_token_t* lex_whitespace(lex_state_t* state)
{
    init_lex(PPT_WHITESPACE);
    buffer_t* buf = buffer_init();
    bool newlines = false;
    while (is_whitespace(peek))
    {
        if (p == '\n')
            newlines = true;
        buffer_append(buf, read);
    }
    token->whitespace = buffer_export(buf);
    buffer_delete(buf);
    if (!state->prev)
        token->can_start_directive = true;
    if (state->include_condition != 0 && !newlines)
        ++state->include_condition;
    cleanup_lex_pass;
    return token;
}

preprocessing_token_t* lex_comment(lex_state_t* state)
{
    init_lex(PPT_WHITESPACE);
    if (peek != '/')
    {
        SET_ERROR("comment must start with '//' or '/*'");
        cleanup_lex_fail;
        return NULL;
    }
    read;
    if (peek == '/')
    {
        read;
        for (;;)
        {
            read;
            if (c == EOF)
            {
                SET_ERROR("source file must end with a newline");
                cleanup_lex_fail;
                return NULL;
            }
            if (c == '\n')
                break;
        }
        char* space = malloc(2);
        space[0] = ' ';
        space[1] = '\0';
        token->whitespace = space;
        if (!state->prev)
            token->can_start_directive = true;
        cleanup_lex_pass;
        return token;
    }
    else if (peek == '*')
    {
        read;
        for (;;)
        {
            read;
            if (c == EOF)
            {
                SET_ERROR("source file must not end in a comment");
                cleanup_lex_fail;
                return NULL;
            }
            if (c == '*')
            {
                if (peek == '/')
                {
                    read;
                    break;
                }
            }
        }
        char* space = malloc(2);
        space[0] = ' ';
        space[1] = '\0';
        token->whitespace = space;
        if (!state->prev)
            token->can_start_directive = true;
        cleanup_lex_pass;
        return token;
    }
    SET_ERROR("comment must start with '//' or '/*'");
    cleanup_lex_fail;
    return NULL;
}

preprocessing_token_t* lex(FILE* file, bool dump_error)
{
    lex_state_t* state = calloc(1, sizeof *state);

    size_t buffer_size = 1024;
    size_t count = 0;
    state->data = malloc(buffer_size);
    for (int c; (c = fgetc(file)) != EOF;)
    {
        if (count % 1024 == 0)
            state->data = realloc(state->data, buffer_size += 1024);
        state->data[count++] = c;
    }

    state->length = count;
    state->row = 1;
    state->col = 1;
    state->cursor = 0;
    state->error = malloc(MAX_ERROR_LENGTH);
    state->error[0] = '\0';
    state->counting = false;
    state->counter = 0;
    state->include_condition = 0;
    state->prev = NULL;

    preprocessing_token_t* tokens = NULL;

    lex_function functions[PPT_NO_ELEMENTS] = {
        lex_string_literal,
        lex_header_name,
        lex_identifier,
        lex_pp_number,
        lex_character_constant,
        lex_punctuator,
        lex_whitespace,
        lex_comment,
        lex_other,
    };
    int counts[PPT_NO_ELEMENTS];
    while (state->cursor < state->length)
    {
        memset(counts, 0, sizeof(int) * PPT_NO_ELEMENTS);

        state->counting = true;

        for (unsigned i = 0; i < PPT_NO_ELEMENTS; ++i)
        {
            functions[i](state);
            counts[i] = state->counter;
        }

        state->counting = false;
        int inc_cond_before = state->include_condition;

        if (state->include_condition >= 2)
        {
            lex_function tmp = functions[0];
            functions[0] = functions[1];
            functions[1] = tmp;
            int tmpc = counts[0];
            counts[0] = counts[1];
            counts[1] = tmpc;
        }
        else
            // do not consider header names if we're not in an include directive
            counts[1] = 0;

        int max = int_array_index_max(counts, PPT_NO_ELEMENTS);
        preprocessing_token_t* token = functions[max](state);
    
        if (state->include_condition >= 2)
        {
            lex_function tmp = functions[0];
            functions[0] = functions[1];
            functions[1] = tmp;
        }

        if (inc_cond_before + 1 != state->include_condition)
            state->include_condition = 0;

        if (!token)
        {
            print_int_array(counts, PPT_NO_ELEMENTS);
            pp_token_delete_all(tokens);
            tokens = NULL;
            if (dump_error)
                fprintf(stderr, "%s", state->error);
            break;
        }

        // concatenate whitespace tokens together
        if (state->prev && state->prev->type == PPT_WHITESPACE && token->type == PPT_WHITESPACE)
        {
            buffer_t* buf = buffer_init();
            buffer_append_str(buf, state->prev->whitespace);
            buffer_append_str(buf, token->whitespace);
            free(state->prev->whitespace);
            pp_token_delete(token);
            token = NULL;
            state->prev->whitespace = buffer_export(buf);
            buffer_delete(buf);
        }
        else
        {
            tokens = add_token(tokens, token);
            token->prev = state->prev;
            state->prev = token;
        }
    }

    lex_state_delete(state);
    return tokens;
}