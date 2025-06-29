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

char* process_one_character(preprocessing_token_t* pp_token, tokenizing_settings_t* settings, char* str, unsigned long long* v, int* l)
{
    if (!v) return NULL;
    if (!l) return NULL;
    unsigned long long value = *v;
    int length = *l;
    if (*str == '\\')
    {
        ++str;
        if (*str == '\'' ||
            *str == '"' ||
            *str == '?' ||
            *str == '\\')
            value = (value << (length += 8, 8)) | *str;
        else if (*str == 'a')
            value = (value << (length += 8, 8)) | '\a';
        else if (*str == 'b')
            value = (value << (length += 8, 8)) | '\b';
        else if (*str == 'f')
            value = (value << (length += 8, 8)) | '\f';
        else if (*str == 'n')
            value = (value << (length += 8, 8)) | '\n';
        else if (*str == 'r')
            value = (value << (length += 8, 8)) | '\r';
        else if (*str == 't')
            value = (value << (length += 8, 8)) | '\t';
        else if (*str == 'v')
            value = (value << (length += 8, 8)) | '\v';
        else if (is_octal_digit(*str))
        {
            value = *str - '0';
            if (is_octal_digit(str[1]))
                value = (value << (length += 3, 3)) | (*(++str) - '0');
            if (is_octal_digit(str[1]))
                value = (value << (length += 3, 3)) | (*(++str) - '0');
        }
        else if (*str == 'x')
        {
            while (is_hexadecimal_digit(*(++str)))
                value = (value << (length += 4, 4)) | hexadecimal_digit_value(*str);
            --str;
        }
        else if (*str == 'u' || *str == 'U')
        {
            unsigned v = get_universal_character_hex_value(str - 1, *str == 'u' ? 6 : 10);
            value = (value << (length += 32, 32)) | get_universal_character_utf8_encoding(v);
            str += (*str == 'u' ? 4 : 8);
        }
        else
            report_return_value(NULL);
    }
    else if (*str == '\n')
        report_return_value(NULL)
    else
        value = (value << (length += 8, 8)) | *str;
    *v = value;
    *l = length;
    ++str;
    return str;
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
    if (pp_token->string_literal.wide)
    {
        vector_t* v = vector_init();
        for (char* str = pp_token->string_literal.value; *str;)
        {
            unsigned long long value = 0;
            int length = 0;
            str = process_one_character(pp_token, settings, str, &value, &length);
            if (!str)
                return NULL;
            if (length > C_TYPE_WCHAR_T_WIDTH * 8)
                warnf("[%s:%d:%d] character in wide string literal out of representable range\n", get_file_name(settings->filepath, false), token->row, token->col);
            vector_add(v, (void*) (value = (int) value));
        }
        vector_add(v, (void*) '\0');
        token->string_literal.value_wide = strdup_wide((int*) v->data);
        vector_delete(v);
    }
    else
    {
        buffer_t* buf = buffer_init();
        for (char* str = pp_token->string_literal.value; *str;)
        {
            unsigned long long value = 0;
            int length = 0;
            str = process_one_character(pp_token, settings, str, &value, &length);
            if (!str)
                return NULL;
            if (length > UNSIGNED_CHAR_WIDTH * 8)
                warnf("[%s:%d:%d] character in string literal out of representable range\n", get_file_name(settings->filepath, false), token->row, token->col);
            buffer_append(buf, (char) value);
        }
        token->string_literal.value_reg = buffer_export(buf);
        buffer_delete(buf);
    }
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
        process_one_character(pp_token, settings, con, &value, &length);
    if (length > C_TYPE_WCHAR_T_WIDTH * 8)
        return fail_token("character constant value too big for its type");
    init_token(T_CHARACTER_CONSTANT);
    token->character_constant.value = value;
    token->character_constant.wide = pp_token->character_constant.wide;
    return token;
}

token_t* tokenize_sequence(preprocessing_token_t* pp_tokens, preprocessing_token_t* end, tokenizing_settings_t* settings)
{
    token_t* head = NULL;
    token_t* current = NULL;
    for (; pp_tokens && pp_tokens != end; pp_tokens = pp_tokens->next)
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

// translation phase 7 (part I)
token_t* tokenize(preprocessing_token_t* pp_tokens, tokenizing_settings_t* settings)
{
    return tokenize_sequence(pp_tokens, NULL, settings);
}