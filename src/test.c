#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cc.h"

/*

the code for this project is developed to match the specifications of ISO/IEC 9899:1999, otherwise known as the C99 standard

STD C TRANSLATION PHASES -> FUNCTIONS IN THIS PROJECT:
 - translation phase 1: lex
 - translation phase 2: lex
 - translation phase 3: lex
 - translation phase 4: preprocess
 - translation phase 5: charconvert
 - translation phase 6: strlitconcat
 - translation phase 7: tokenize, parse, type, analyze, linearize, allocate
 - translation phase 8: (handled externally by linker)

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

    preprocessing_token_t* tokens = lex(file, true);
    if (!tokens) return 1;

    printf("<<lexer output>>\n");
    for (preprocessing_token_t* token = tokens; token; token = token->next)
    {
        pp_token_print(token, printf);
        printf("\n");
    }

    preprocessing_settings_t settings;
    settings.filepath = argv[1];
    char pp_error[MAX_ERROR_LENGTH];
    settings.error = pp_error;

    if (!preprocess(&tokens, &settings))
    {
        printf("%s", settings.error);
        return 1;
    }

    printf("<<preprocessor output>>\n");
    for (preprocessing_token_t* token = tokens; token; token = token->next)
    {
        pp_token_print(token, printf);
        printf("\n");
    }

    charconvert(tokens);
    strlitconcat(tokens);

    tokenizing_settings_t tk_settings;
    tk_settings.filepath = argv[1];
    char tok_error[MAX_ERROR_LENGTH];
    tk_settings.error = tok_error;
    tk_settings.error[0] = '\0';

    token_t* ts = tokenize(tokens, &tk_settings);
    if (tk_settings.error[0])
    {
        printf("%s", tk_settings.error);
        return 1;
    }

    pp_token_delete_all(tokens);

    printf("<<tokenizer output>>\n");
    for (token_t* t = ts; t; t = t->next)
    {
        token_print(t, printf);
        printf("\n");
    }

    syntax_component_t* tlu = parse(ts);
    if (!tlu) return 1;

    analysis_error_t* type_errors = type(tlu);
    if (type_errors)
    {
        dump_errors(type_errors);
        if (error_list_size(type_errors, false) > 0)
            return 1;
    }

    analysis_error_t* errors = analyze(tlu);
    if (errors)
    {
        dump_errors(errors);
        if (error_list_size(errors, false) > 0)
            return 1;
    }

    printf("<<typed syntax tree>>\n");
    print_syntax(tlu, printf);

    printf("<<symbol table>>\n");
    symbol_table_print(tlu->tlu_st, printf);

    ir_insn_t* insns = linearize(tlu);
    printf("<<linear IR>>\n");
    insn_clike_print_all(insns, printf);

    allocate(insns, x86procregmap, 9);

    printf("<<allocated linear IR>>\n");
    insn_clike_print_all(insns, printf);

    x86_insn_t* x86_insns = x86_generate(insns, tlu->tlu_st);

    printf("<<x86 assembly code>>\n");
    x86_write_all(x86_insns, stdout);

    fclose(file);
    x86_insn_delete_all(x86_insns);
    free_syntax(tlu, tlu);
    token_delete_all(ts);
    return 0;
}