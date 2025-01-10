#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cc.h"

// parser test code
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        errorf("expected one argument (filepath)\n");
        return 1;
    }
    FILE* file = fopen(argv[1], "r");
    lexer_token_t* tokens = lex(file);
    if (!tokens) return 1;
    // printf("<<lexer output>>\n");
    // for (lexer_token_t* tok = toks; tok; tok = tok->next)
    //     print_token(tok, printf);
    syntax_component_t* tlu = parse(tokens);
    if (!tlu) return 1;
    printf("<<parser output>>\n");
    print_syntax(tlu, printf);
    printf("<<symbol table>>\n");
    symbol_table_print(tlu->tlu_st, printf);
    //FILE* out = fopen("out.s", "w");
    //emit(unit, out);
    //fclose(out);
    fclose(file);
    free_syntax(tlu, tlu);
    return 0;
}