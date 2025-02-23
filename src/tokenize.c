#include <stdlib.h>
#include <string.h>

#include "ecc.h"

#define init_token(t) \
    token_t* token = calloc(1, sizeof *token); \
    token->type = (t); \
    token->row = pp_token->row; \
    token->col = pp_token->col;
#define fail_token(fmt, ...) (snerrorf(settings->error, MAX_ERROR_LENGTH, "[%s:%d:%d] " fmt "\n", get_file_name(settings->filepath, false), pp_token->row, pp_token->col, ## __VA_ARGS__), NULL)

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

void token_print(token_t* token, int (*printer)(const char* fmt, ...))
{
    if (!token) return;
    printer("token { type: %s, line: %u, column: %u", TOKEN_NAMES[token->type], token->row, token->col);
    switch (token->type)
    {
        case T_KEYWORD:
            printer(", keyword: %s", KEYWORDS[token->keyword]);
            break;
        case T_IDENTIFIER:
            printer(", identifier: %s", token->identifier);
            break;
        case T_INTEGER_CONSTANT:
            printer(", integer constant (ULL): %llu, type: %s", token->integer_constant.value, C_TYPE_CLASS_NAMES[token->integer_constant.class]);
            break;
        case T_FLOATING_CONSTANT:
            printer(", floating constant (LD): %Lf, type: %s", token->floating_constant.value, C_TYPE_CLASS_NAMES[token->floating_constant.class]);
            break;
        case T_CHARACTER_CONSTANT:
            printer(", character constant: %d, wide: %s", token->character_constant.value, BOOL_NAMES[token->character_constant.wide]);
            break;
        case T_STRING_LITERAL:
            if (token->string_literal.value_reg)
                printer(", string literal: \"%s\"", token->string_literal.value_reg);
            else
                printer(", string literal: L\"%ls\"", token->string_literal.value_wide);
            break;
        case T_PUNCTUATOR:
            printer(", punctuator: %s", PUNCTUATOR_STRING_REPRS[token->punctuator]);
            break;
        default:
            break;
    }
    printer(" }");
}

void token_delete(token_t* token)
{
    if (!token) return;
    switch (token->type)
    {
        case T_STRING_LITERAL:
            free(token->string_literal.value_reg);
            free(token->string_literal.value_wide);
            break;
        case T_IDENTIFIER:
            free(token->identifier);
            break;
        default:
            break;
    }
    free(token);
}

void token_delete_all(token_t* token)
{
    if (!token) return;
    token_delete_all(token->next);
    token_delete(token);
}

token_t* tokenize_identifier(preprocessing_token_t* pp_token, tokenizing_settings_t* settings)
{
    if (!pp_token || pp_token->type != PPT_IDENTIFIER)
        return fail_token("expected identifier");
    int idx = contains((void**) KEYWORDS, KW_ELEMENTS, pp_token->identifier, (int (*)(void*, void*)) strcmp);
    init_token(idx == -1 ? T_IDENTIFIER : T_KEYWORD);
    if (idx == -1)
        token->identifier = strdup(pp_token->identifier);
    else
        token->keyword = (c_keyword_t) idx;
    return token;
}

token_t* tokenize_punctuator(preprocessing_token_t* pp_token, tokenizing_settings_t* settings)
{
    if (!pp_token || pp_token->type != PPT_PUNCTUATOR)
        return fail_token("expected punctuator");
    init_token(T_PUNCTUATOR);
    token->punctuator = pp_token->punctuator;
    return token;
}

token_t* tokenize_string_literal(preprocessing_token_t* pp_token, tokenizing_settings_t* settings)
{
    if (!pp_token || pp_token->type != PPT_STRING_LITERAL)
        return fail_token("expected string literal");
    init_token(T_STRING_LITERAL);
    // TODO: do escape sequence replacement
    if (contains_substr(pp_token->string_literal.value, "\\"))
        warnf("[%s:%d:%d] escape sequences in string literals are not yet supported\n", get_file_name(settings->filepath, false), token->row, token->col);
    if (pp_token->string_literal.wide)
        token->string_literal.value_wide = strdup_widen(pp_token->string_literal.value);
    else
        token->string_literal.value_reg = strdup(pp_token->string_literal.value);
    return token;
}

token_t* tokenize_pp_number(preprocessing_token_t* pp_token, tokenizing_settings_t* settings)
{
    if (!pp_token || pp_token->type != PPT_PP_NUMBER)
        return fail_token("expected preprocessing number");
    c_type_class_t c = CTC_ERROR;
    unsigned long long ivalue = process_integer_constant(pp_token->pp_number, &c);
    if (c != CTC_ERROR)
    {
        init_token(T_INTEGER_CONSTANT);
        token->integer_constant.class = c;
        token->integer_constant.value = ivalue;
        return token;
    }
    long double fvalue = process_floating_constant(pp_token->pp_number, &c);
    if (c == CTC_ERROR)
        return fail_token("expected integer or floating constant");
    init_token(T_FLOATING_CONSTANT);
    token->floating_constant.class = c;
    token->floating_constant.value = fvalue;
    return token;
}

token_t* tokenize_character_constant(preprocessing_token_t* pp_token, tokenizing_settings_t* settings)
{
    if (!pp_token || pp_token->type != PPT_CHARACTER_CONSTANT)
        return fail_token("expected character constant");
    unsigned long long value = 0;
    char* con = pp_token->character_constant.value;
    int length = 0;
    if (*con++ != '\'') report_return_value(NULL);
    if (*con == '\'') report_return_value(NULL);
    while (*con != '\'')
    {
        if (*con == '\\')
        {
            ++con;
            if (*con == '\'' ||
                *con == '"' ||
                *con == '?' ||
                *con == '\\')
                value = (value << (length += 8, 8)) | *con;
            else if (*con == 'a')
                value = (value << (length += 8, 8)) | '\a';
            else if (*con == 'b')
                value = (value << (length += 8, 8)) | '\b';
            else if (*con == 'f')
                value = (value << (length += 8, 8)) | '\f';
            else if (*con == 'n')
                value = (value << (length += 8, 8)) | '\n';
            else if (*con == 'r')
                value = (value << (length += 8, 8)) | '\r';
            else if (*con == 't')
                value = (value << (length += 8, 8)) | '\t';
            else if (*con == 'v')
                value = (value << (length += 8, 8)) | '\v';
            else if (is_octal_digit(*con))
            {
                value = *con - '0';
                if (is_octal_digit(con[1]))
                    value = (value << (length += 3, 3)) | (*(++con) - '0');
                if (is_octal_digit(con[1]))
                    value = (value << (length += 3, 3)) | (*(++con) - '0');
            }
            else if (*con == 'x')
            {
                while (is_hexadecimal_digit(*(++con)))
                    value = (value << (length += 4, 4)) | hexadecimal_digit_value(*con);
                --con;
            }
            else if (*con == 'u' || *con == 'U')
            {
                unsigned v = get_universal_character_hex_value(con - 1, *con == 'u' ? 6 : 10);
                value = (value << (length += 32, 32)) | get_universal_character_utf8_encoding(v);
                con += (*con == 'u' ? 4 : 8);
            }
            else
                report_return_value(NULL);
        }
        else if (*con == '\n')
            report_return_value(NULL)
        else
            value = (value << (length += 8, 8)) | *con;
        ++con;
    }
    // TODO: better checking on object size bounds
    if (length > sizeof(int) * 8)
        return fail_token("character constant value too big for its type");
    init_token(T_CHARACTER_CONSTANT);
    token->character_constant.value = value;
    token->character_constant.wide = pp_token->character_constant.wide;
    return token;
}

// translation phase 7 (part I)
token_t* tokenize(preprocessing_token_t* pp_tokens, tokenizing_settings_t* settings)
{
    token_t* head = NULL;
    token_t* current = NULL;
    for (; pp_tokens; pp_tokens = pp_tokens->next)
    {
        token_t* token = NULL;
        switch (pp_tokens->type)
        {
            case PPT_IDENTIFIER:
                token = tokenize_identifier(pp_tokens, settings);
                break;
            case PPT_PUNCTUATOR:
                token = tokenize_punctuator(pp_tokens, settings);
                break;
            case PPT_STRING_LITERAL:
                token = tokenize_string_literal(pp_tokens, settings);
                break;
            case PPT_PP_NUMBER:
                token = tokenize_pp_number(pp_tokens, settings);
                break;
            case PPT_CHARACTER_CONSTANT:
                token = tokenize_character_constant(pp_tokens, settings);
                break;
            // ignore everything else
            default:
                continue;
        }
        if (!token)
        {
            token_delete_all(head);
            return NULL;
        }
        if (current)
            current = current->next = token;
        else
            head = current = token;
    }
    return head;
}