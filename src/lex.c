#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "cc.h"

#define lex_terminate(pos, fmt, ...) terminate("(line %d, col. %d) " fmt, pos->row, pos->col, ## __VA_ARGS__)

#define isloweralpha(x) ((x) >= 'a' && (x) <= 'z')
#define isupperalpha(x) ((x) >= 'A' && (x) <= 'Z')
#define isnumeric(x) ((x) >= '0' && (x) <= '9')
#define isalpha(x) (isloweralpha((x)) || isupperalpha((x)))
#define isalphanumeric(x) (isalpha((x)) || isnumeric((x)))

#define IGNORE_TOKEN ((lexer_token_t*) 1)

typedef struct filepos_t
{
    unsigned row, col;
    unsigned pcol;
} filepos_t;

int unread_buffer[1024];
int unread_buffer_pos = -1;

static int readc_opt_term(FILE* file, filepos_t* pos, bool terminate_eof)
{
    int c;
    if (unread_buffer_pos == -1)
        c = fgetc(file);
    else
        c = unread_buffer[unread_buffer_pos--];
    if (c == EOF)
    {
        if (terminate_eof)
            lex_terminate(pos, "reached end of file while parsing token")
        else
            return c;
    }
    if (c == '\n')
    {
        ++(pos->row);
        pos->pcol = pos->col;
        pos->col = 1;
    }
    else
        ++(pos->col);
    return c;
}

static int readc(FILE* file, filepos_t* pos)
{
    return readc_opt_term(file, pos, true);
}

static void unreadc(int c, FILE* file, filepos_t* pos)
{
    unread_buffer[++unread_buffer_pos] = c;
    if (c == '\n')
    {
        --(pos->row);
        pos->col = pos->pcol;
    }
    else
        --(pos->col);
}

static int peekc(FILE* file, filepos_t* pos)
{
    int c = readc_opt_term(file, pos, false);
    unreadc(c, file, pos);
    return c;
}

// this monstrosity of a function handles EVERY SINGLE TOKEN
static lexer_token_t* lex_single(FILE* file, filepos_t* pos)
{
    int c = readc_opt_term(file, pos, false);
    lexer_token_t* tok = calloc(1, sizeof *tok);
    tok->row = pos->row;
    tok->col = pos->col;
    tok->next = NULL;
    tok->type = LEXER_TOKEN_OPERATOR; // (it's the most common token)
    int cpk = peekc(file, pos);
    bool chk_long_const = c == 'L' && (cpk == '"' || cpk == '\'');
    bool is_unicode_identifier = c == '\\' && (tolower(cpk) == 'u');
    if ((isalpha(c) || c == '_' || is_unicode_identifier) && !chk_long_const)
    {
        buffer_t* b = buffer_init();
        buffer_append(b, c);
        int d = readc(file, pos);
        for (; isalphanumeric(d) || d == '\\'; d = readc(file, pos))
        {
            // LOL UNICODEEEEEEEEEEE
            if (d == '\\' || is_unicode_identifier) // is_unicode_identifier is reused here to catch the skipped backslash
            {
                int digits = d == 'u' ? 4 : 8;
                buffer_append(b, d);
                int e = readc(file, pos);
                if (d == '\\')
                {
                    if (tolower(e) != 'u') lex_terminate(pos, "expected 'u' or 'U' for unicode character in identifier");
                    buffer_append(b, e);
                    digits = e == 'u' ? 4 : 8;
                    e = readc(file, pos); // skip past u or U
                }
                for (unsigned i = 0; i < digits; ++i)
                {
                    if (!isnumeric(e) && (tolower(e) < 'a' || tolower(e) > 'f'))
                        lex_terminate(pos, "expected %d hexadecimal digits for unicode character", digits);
                    buffer_append(b, e);
                    e = readc(file, pos);
                }
                unreadc(e, file, pos);
                is_unicode_identifier = false;
                continue;
            }
            buffer_append(b, d);
        }
        unreadc(d, file, pos); // get rid of the non-alphanumeric character at the end
        // check if it's a keyword
        for (unsigned i = 0; i < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); ++i)
        {
            if (!strcmp(b->data, KEYWORDS[i])) // compare the found identifier to each keyword
            {
                // if found, return the keyword
                tok->type = LEXER_TOKEN_KEYWORD;
                tok->keyword_id = i;
                buffer_delete(b);
                return tok;
            }
        }
        tok->type = LEXER_TOKEN_IDENTIFIER;
        tok->string_value = buffer_export(b);
        buffer_delete(b);
        return tok;
    }
    if (c == '*' || c == '%' || c == '^' || c == '=' || c == '!')
    {
        tok->operator_id = c;
        if (peekc(file, pos) == '=')
        {
            readc(file, pos);
            tok->operator_id = c * '=';
        }
        return tok;
    }
    if (c == '/')
    {
        tok->operator_id = c;
        int d = peekc(file, pos);
        if (d == '=')
        {
            readc(file, pos);
            tok->operator_id = c * '=';
        }
        if (d == '/')
        {
            readc(file, pos); // go past '/'
            while (readc(file, pos) != '\n');
            free(tok);
            return IGNORE_TOKEN;
        }
        if (d == '*')
        {
            readc(file, pos); // go past '*'
            while (readc(file, pos) != '*');
            int e = readc(file, pos);
            if (e != '/') lex_terminate(pos, "expected '*/' to end block comment");
            free(tok);
            return IGNORE_TOKEN;
        }
        return tok;
    }
    if (c == '+')
    {
        tok->operator_id = c;
        int d = peekc(file, pos);
        if (d == c || d == '=')
        {
            readc(file, pos);
            tok->operator_id = c * d;
        }
        return tok;
    }
    if (c == '-')
    {
        tok->operator_id = c;
        int d = peekc(file, pos);
        if (d == c || d == '=' || d == '>')
        {
            readc(file, pos);
            tok->operator_id = c * d;
        }
        return tok;
    }
    if (c == '&' || c == '|')
    {
        tok->operator_id = c;
        int d = peekc(file, pos);
        if (d == c || d == '=')
        {
            readc(file, pos);
            tok->operator_id = c * d;
        }
        return tok;
    }
    if (c == '<' || c == '>')
    {
        tok->operator_id = c;
        int d = peekc(file, pos);
        if (d == c)
        {
            readc(file, pos);
            tok->operator_id = c * d;
            int e = peekc(file, pos);
            if (e == '=')
            {
                readc(file, pos);
                tok->operator_id = c * d * e;
            }
        }
        if (d == '=')
        {
            readc(file, pos);
            tok->operator_id = c * d;
        }
        return tok;
    }
    if (c == '~' || c == '[' || c == ']' || c == '?' || c == ':')
    {
        tok->operator_id = c;
        return tok;
    }
    int maybe_num = peekc(file, pos);
    if (c == '.' && !isnumeric(maybe_num))
    {
        tok->operator_id = c;
        int d = readc(file, pos);
        int e = readc(file, pos);
        if (d == c && e == c)
            tok->operator_id = c * d * e;
        else
        {
            unreadc(e, file, pos);
            unreadc(d, file, pos);
        }
        return tok;
    }
    if (c == '(' || c == ')' || c == '{' || c == '}' || c == ',' || c == ';')
    {
        tok->type = LEXER_TOKEN_SEPARATOR;
        tok->operator_id = c;
        return tok;
    }
    if (isnumeric(c) || c == '.')
    {
        buffer_t* b = buffer_init();
        buffer_append(b, c); // add the first character

        // check for prefix
        int d = peekc(file, pos);
        bool hex = d == 'x' || d == 'X';
        bool binary = d == 'b' || d == 'B';
        bool prefixed = hex || binary;
        if (prefixed)
            buffer_append(b, readc(file, pos));
        
        bool floating = false, numbers = false;
        int e = readc(file, pos);
        int decimals = 0;
        // read the significand (for floats) or the whole number (for integers)
        // allow for a-f when hex is true
        // allow for decimal points but only one
        for (; isnumeric(e) || (hex && tolower(e) >= 'a' && tolower(e) <= 'f') || e == '.'; e = readc(file, pos))
        {
            if (e == '.') 
            {
                ++decimals; // count the decimals
                floating = true; // if there's a decimal, it must be a floating point number
                // if there's more than one decimal, get em out
                if (decimals > 1) lex_terminate(pos, "too many decimals in floating point literal");
            }
            else
                numbers = true;
            // if a digit provided is greater than 1 and the number is binary by prefix, get em out
            if (e >= '2' && binary) lex_terminate(pos, "only digits 0 and 1 allowed in binary literal");
            // if a digit provided is greater than 7 and the number has a leading zero and is not prefixed (octal), get em out
            if (e >= '8' && c == '0' && !prefixed) lex_terminate(pos, "only digits 0-7 allowed in octal literal");
            // add the digit
            buffer_append(b, e);
        }
        if (prefixed && !numbers) lex_terminate(pos, "expected a number after the prefix");
        // record here if the for loop's failure was on an E or a P (exponent)
        bool e_exp = tolower(e) == 'e';
        bool p_exp = tolower(e) == 'p';
        if (e_exp || p_exp) // if either is found, handle em
        {
            // don't allow integer literal prefixes with 'E' notation
            if (e_exp && prefixed) lex_terminate(pos, "integer literal prefixes not allowed for 'E' exponent notation");
            // only allow hex literals with 'P' notation
            if (p_exp && !hex) lex_terminate(pos, "expected hexadecimal constant for 'P' exponent notation");
            floating = true; // this number is by necessity floating
            buffer_append(b, e); // add the loop failure character
            int f = readc(file, pos);
            bool numbers = false, sign = f == '+' || f == '-';
            // read numbers for the exponent and allow for +s and -s
            for (int signs = 0; isnumeric(f) || f == '+' || f == '-'; f = readc(file, pos))
            {
                if (f == '+' || f == '-') // if it's a + or a -
                {
                    ++signs; // add to the signs count
                    // if there is more than one sign or if the sign is not at the start, get em out
                    if (signs > 1 || numbers) lex_terminate(pos, "malformed use of the exponent sign");
                }
                else numbers = true;
                buffer_append(b, f);
            }
            // update the outer character with the one bound for looping over the digits of the exponent, which should be right past the those digits
            e = f;
            // make sure they actually put in an exponent number
            if (!numbers) lex_terminate(pos, "exponent expected after 'E' or 'P' exponent notation");
        }
        // loop here and find suffixes
        bool f_suffix = false, u_suffix = false, l_suffix = false, ll_suffix = false;
        for (; tolower(e) == 'f' || tolower(e) == 'l' || tolower(e) == 'u'; e = readc(file, pos))
        {
            if (tolower(e) == 'f')
            {
                if (f_suffix) lex_terminate(pos, "repeated use of 'F' suffix");
                if (l_suffix || ll_suffix) lex_terminate(pos, "the 'L' and 'LL' suffixes cannot be used in conjunction with the 'F' suffix");
                // f suffix does not work if there is no decimal point and no E or P extension
                if (!floating) lex_terminate(pos, "decimal point or 'E' or 'P' exponent notation expected before 'F' suffix");
                buffer_append(b, e);
                f_suffix = true;
            }
            if (tolower(e) == 'l')
            {
                if (peekc(file, pos) == e)
                {
                    int g = readc(file, pos);
                    if (ll_suffix) lex_terminate(pos, "repeated use of the 'LL' suffix");
                    if (l_suffix) lex_terminate(pos, "constant already has 'L' suffix");
                    if (floating) lex_terminate(pos, "'LL' suffix can only be used for non-floating point constants");
                    buffer_append(b, e);
                    buffer_append(b, g);
                    ll_suffix = true;
                }
                else
                {
                    if (l_suffix) lex_terminate(pos, "repeated use of the 'L' suffix");
                    if (ll_suffix) lex_terminate(pos, "constant already has 'LL' suffix");
                    buffer_append(b, e);
                    l_suffix = true;
                }
            }
            if (tolower(e) == 'u')
            {
                if (u_suffix) lex_terminate(pos, "repeated use of the 'U' suffix");
                if (f_suffix) lex_terminate(pos, "the 'F' suffix cannot be used in conjunction with the 'F' suffix");
                if (floating) lex_terminate(pos, "'U' suffix can only be used for non-floating point constants");
                buffer_append(b, e);
                u_suffix = true;
            }
        }
        unreadc(e, file, pos);
        tok->type = floating ? LEXER_TOKEN_FLOATING_CONSTANT : LEXER_TOKEN_INTEGER_CONSTANT;
        tok->string_value = buffer_export(b);
        buffer_delete(b);
        return tok;
    }
    if (chk_long_const || c == '"' || c == '\'')
    {
        buffer_t* b = buffer_init();
        buffer_append(b, c);
        if (chk_long_const)
            buffer_append(b, c = readc(file, pos));
        bool string_const = c == '"';
        c = readc(file, pos);
        // i know this condition expression is ridiculous but i cba to invert everything
        for (bool escape = false; !(!escape && ((c == '"' && string_const) || (c == '\'' && !string_const))); escape = c == '\\' && !escape, c = readc(file, pos))
            buffer_append(b, c);
        buffer_append(b, c); // add ending quote
        tok->type = string_const ? LEXER_TOKEN_STRING_CONSTANT : LEXER_TOKEN_CHARACTER_CONSTANT;
        tok->string_value = buffer_export(b);
        buffer_delete(b);
        return tok;
    }
    if (c == EOF)
        return NULL;
    free(tok);
    return IGNORE_TOKEN;
}

lexer_token_t* lex(FILE* file)
{
    if (!file)
        return NULL;
    lexer_token_t* start = NULL, * current = NULL;
    filepos_t pos;
    pos.row = 1;
    pos.col = 1;
    pos.pcol = 1;
    for (;;)
    {
        lexer_token_t* tok = lex_single(file, &pos);
        if (!tok)
            break;
        if (tok == IGNORE_TOKEN)
            continue;
        if (!start)
            start = tok;
        if (current)
            current->next = tok;
        current = tok;
    }
    return start;
}

// delete an entire chain of tokens
void lex_delete(lexer_token_t* start)
{
    if (!start) return;
    if (start->type == LEXER_TOKEN_IDENTIFIER || (start->type >= LEXER_TOKEN_INTEGER_CONSTANT && start->type <= LEXER_TOKEN_STRING_CONSTANT))
        free(start->string_value);
    lexer_token_t* nxt = start->next;
    free(start);
    lex_delete(nxt);
}

