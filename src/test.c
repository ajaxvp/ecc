#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cc.h"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        errorf("expected one argument (filepath)\n");
        return 1;
    }
    FILE* file = fopen(argv[1], "r");
    for (lexer_token_t* tok = lex(file); tok; tok = lex_next(tok))
    {
        switch (tok->type)
        {
            case LEXER_TOKEN_KEYWORD:
            {
                printf("[kw]\t%s\n", KEYWORDS[tok->keyword_id]);
                break;
            }
            case LEXER_TOKEN_IDENTIFIER:
            {
                printf("[ident]\t%s\n", tok->string_value);
                break;
            }
            case LEXER_TOKEN_OPERATOR:
            {
                printf(tok->operator_id < 256 ? "[op]\t%c\n" : "[op]\t%d\n", tok->operator_id);
                break;
            }
            case LEXER_TOKEN_SEPARATOR:
            {
                printf("[sep]\t%c\n", tok->separator_id);
                break;
            }
            case LEXER_TOKEN_INTEGER_CONSTANT:
            case LEXER_TOKEN_FLOATING_CONSTANT:
            case LEXER_TOKEN_CHARACTER_CONSTANT:
            case LEXER_TOKEN_STRING_CONSTANT:
            {
                printf("[const]\t%s\n", tok->string_value);
                break;
            }
        }
    }
    fclose(file);
}