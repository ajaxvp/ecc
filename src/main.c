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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <getopt.h>

#include "ecc.h"

#define OPTION_DESCRIPTION_LENGTH 12

char* PROGRAM_NAME = NULL;
program_options_t opts;

program_options_t* get_program_options(void)
{
    return &opts;
}

int usage(void)
{
    printf("Usage: ecc [options] file...\n");
    printf("Options:\n");
    printf("  %-*sDisplay this help message\n", OPTION_DESCRIPTION_LENGTH, "-h");
    printf("  %-*sDisplay internal states (tokens, IRs, etc.)\n", OPTION_DESCRIPTION_LENGTH, "-i");
    return EXIT_SUCCESS;
}

void work(char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        errorf("file '%s' not found", filename);
        return;
    }
    preprocessing_token_t* tokens = lex(file, true);
    if (!tokens) return;

    if (opts.iflag)
    {
        printf("<<lexer output>>\n");
        for (preprocessing_token_t* token = tokens; token; token = token->next)
        {
            pp_token_print(token, printf);
            printf("\n");
        }
    }

    preprocessing_settings_t settings;
    settings.filepath = filename;
    char pp_error[MAX_ERROR_LENGTH];
    settings.error = pp_error;

    if (!preprocess(&tokens, &settings))
    {
        printf("%s", settings.error);
        return;
    }

    if (opts.iflag)
    {
        printf("<<preprocessor output>>\n");
        for (preprocessing_token_t* token = tokens; token; token = token->next)
        {
            pp_token_print(token, printf);
            printf("\n");
        }
    }

    charconvert(tokens);
    strlitconcat(tokens);

    tokenizing_settings_t tk_settings;
    tk_settings.filepath = filename;
    char tok_error[MAX_ERROR_LENGTH];
    tk_settings.error = tok_error;
    tk_settings.error[0] = '\0';

    token_t* ts = tokenize(tokens, &tk_settings);
    if (tk_settings.error[0])
    {
        printf("%s", tk_settings.error);
        return;
    }

    pp_token_delete_all(tokens);

    if (opts.iflag)
    {
        printf("<<tokenizer output>>\n");
        for (token_t* t = ts; t; t = t->next)
        {
            token_print(t, printf);
            printf("\n");
        }
    }

    syntax_component_t* tlu = parse(ts);
    if (!tlu) return;

    analysis_error_t* type_errors = type(tlu);
    if (type_errors)
    {
        dump_errors(type_errors);
        if (error_list_size(type_errors, false) > 0)
            return;
    }

    if (opts.iflag)
    {
        printf("<<typed syntax tree>>\n");
        print_syntax(tlu, printf);

        printf("<<symbol table>>\n");
        symbol_table_print(tlu->tlu_st, printf);
    }

    analysis_error_t* errors = analyze(tlu);
    if (errors)
    {
        dump_errors(errors);
        if (error_list_size(errors, false) > 0)
            return;
    }

    if (opts.iflag)
    {
        printf("<<typed syntax tree>>\n");
        print_syntax(tlu, printf);

        printf("<<symbol table>>\n");
        symbol_table_print(tlu->tlu_st, printf);
    }

    ir_insn_t* insns = linearize(tlu);

    if (opts.iflag)
    {
        printf("<<linear IR>>\n");
        insn_clike_print_all(insns, printf);
    }

    ir_optimize(insns, ir_opt_profile_basic());

    if (opts.iflag)
    {
        printf("<<optimized linear IR>>\n");
        insn_clike_print_all(insns, printf);
    }

    // allocator_options_t alloc_options;
    // alloc_options.procregmap = x86procregmap;
    // alloc_options.no_volatile = 9; // rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
    // alloc_options.no_nonvolatile = 5; // rbx, r12, r13, r14, r15

    // allocate(insns, &alloc_options);

    // printf("<<allocated linear IR>>\n");
    // insn_clike_print_all(insns, printf);

    // x86_insn_t* x86_insns = x86_generate(insns, tlu->tlu_st);

    // printf("<<x86 assembly code>>\n");
    // x86_write_all(x86_insns, stdout);

    // FILE* out = fopen("prgmtest.s", "w");
    // x86_write_all(x86_insns, out);
    // fclose(out);
    // printf("assembly written to prgmtest.s\n");

    fclose(file);
    //x86_insn_delete_all(x86_insns);
    free_syntax(tlu, tlu);
    token_delete_all(ts);
    return;
}

int main(int argc, char** argv)
{
    PROGRAM_NAME = argv[0];
    if (argc <= 1)
    {
        errorf("no input files\n");
        return EXIT_FAILURE;
    }
    for (int c; (c = getopt(argc, argv, "hi")) != -1;)
    {
        switch (c)
        {
            case 'h':
                return usage();
            case 'i':
                opts.iflag = true;
                break;
            case '?':
            default:
            {
                errorf("unknown option specified: -%c\n", c);
                return EXIT_FAILURE;
            }
        }
    }
    for (unsigned i = optind; i < argc; ++i)
        work(argv[i]);
    return 0;
}