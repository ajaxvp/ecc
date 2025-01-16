#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cc.h"

/*

SEVEN PHASES OF THIS COMPILER:

LEXER - lex(file): take a text source file and split it into manageable tokens to work with in future stages
(NOT WRITTEN) PREPROCESSOR - preprocess(tokens): take in a sequence of tokens and give back a new sequence of tokens with macros expanded
PARSER - parse(tokens): take the preprocessed tokens and arrange them into a tree structure
TYPER - type(tree): takes the tree and types all of the symbols associated with it
STATIC ANALYZER - analyze(tree): takes the tree and catches errors in the semantics of it, along with typing expressions
(NOT WRITTEN) LINEARIZER - linearize(tree): takes the tree and converts it into a list of assembly instructions/macros/labels
(NOT WRITTEN) ALLOCATOR - allocate(list): takes the linearized code and properly allocates its register usage

once all of these steps are done, it will take the list and dump it into the output file

*/

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
    // for (lexer_token_t* tok = tokens; tok; tok = tok->next)
    //     print_token(tok, printf);

    syntax_component_t* tlu = parse(tokens);
    if (!tlu) return 1;

    printf("<<parser output>>\n");
    print_syntax(tlu, printf);

    printf("<<symbol table>>\n");
    symbol_table_print(tlu->tlu_st, printf);

    analysis_error_t* type_errors = type(tlu);
    if (type_errors)
    {
        dump_errors(type_errors);
        if (error_list_size(type_errors, false) > 0)
            return 1;
    }

    printf("<<typed symbol table>>\n");
    symbol_table_print(tlu->tlu_st, printf);

    analysis_error_t* errors = analyze(tlu);
    if (errors)
    {
        dump_errors(errors);
        if (error_list_size(errors, false) > 0)
            return 1;
    }

    printf("<<typed syntax tree>>\n");
    print_syntax(tlu, printf);

    fclose(file);
    free_syntax(tlu, tlu);
    return 0;
}