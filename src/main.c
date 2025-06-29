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
#include <time.h>

#include <getopt.h>
#include <unistd.h>
#include <sys/wait.h>

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
    printf("  %-*sLex, preprocess, and exit\n", OPTION_DESCRIPTION_LENGTH, "-P");
    printf("  %-*sLex, preprocess, tokenize, parse, and exit\n", OPTION_DESCRIPTION_LENGTH, "-p");
    printf("  %-*sLex, preprocess, tokenize, parse, type, analyze, and exit\n", OPTION_DESCRIPTION_LENGTH, "-a");
    printf("  %-*sBleeding edge work\n", OPTION_DESCRIPTION_LENGTH, "-x");
    return EXIT_FAILURE;
}

char* work(char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        errorf("file '%s' not found", filename);
        return NULL;
    }
    preprocessing_token_t* tokens = lex(file, true);
    if (!tokens) return NULL;

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
    settings.table = NULL;

    if (!preprocess(&tokens, &settings))
    {
        printf("%s", settings.error);
        return NULL;
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

    if (opts.ppflag)
    {
        pp_token_delete_all(tokens);
        return NULL;
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
        return NULL;
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
    if (!tlu) return NULL;

    if (opts.iflag)
    {
        printf("<<syntax tree>>\n");
        print_syntax(tlu, printf);
    }

    if (opts.pflag)
    {
        token_delete_all(ts);
        free_syntax(tlu, tlu);
        return NULL;
    }

    analysis_error_t* type_errors = type(tlu);
    if (type_errors)
    {
        dump_errors(type_errors);
        if (error_list_size(type_errors, false) > 0)
            return NULL;
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
            return NULL;
    }

    if (opts.iflag)
    {
        printf("<<typed syntax tree>>\n");
        print_syntax(tlu, printf);

        printf("<<symbol table>>\n");
        symbol_table_print(tlu->tlu_st, printf);
    }

    if (opts.aflag)
    {
        token_delete_all(ts);
        free_syntax(tlu, tlu);
        return NULL;
    }

    if (opts.xflag)
    {
        air_t* air = airinize(tlu);

        if (opts.iflag)
        {
            printf("<<AIR>>\n");
            air_print(air, printf);
        }

        localize(air, LOC_X86_64);

        if (opts.iflag)
        {
            printf("<<AIR (x86-localized)>>\n");
            air_print(air, printf);
        }

        token_delete_all(ts);
        free_syntax(tlu, tlu);
        return NULL;
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

    allocator_options_t alloc_options;
    alloc_options.procregmap = x86procregmap;
    alloc_options.no_volatile = 9; // rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
    alloc_options.no_nonvolatile = 5; // rbx, r12, r13, r14, r15

    allocate(insns, &alloc_options);

    if (opts.iflag)
    {
        printf("<<allocated linear IR>>\n");
        insn_clike_print_all(insns, printf);
    }

    x86_insn_t* x86_insns = x86_generate(insns, tlu->tlu_st);

    if (opts.iflag)
    {
        printf("<<x86 assembly code>>\n");
        x86_write_all(x86_insns, stdout);
    }

    char* asm_filepath = temp_filepath_gen(".s");
    char* obj_filepath = temp_filepath_gen(".o");
    FILE* out = fopen(asm_filepath, "w");
    x86_write_all(x86_insns, out);
    fclose(out);
    if (opts.iflag)
        printf("assembly written to %s\n", asm_filepath);

    fclose(file);
    x86_insn_delete_all(x86_insns);
    free_syntax(tlu, tlu);
    token_delete_all(ts);

    pid_t as_pid = fork();
    if (as_pid == -1)
    {
        free(obj_filepath);
        free(asm_filepath);
        errorf("failed to spawn assembler process\n");
        return NULL;
    }
    if (as_pid == 0)
    {
        char* argv[] = { "/usr/bin/as", asm_filepath, "-o", obj_filepath, NULL };
        int status = execv(argv[0], argv);
        if (status == -1)
        {
            free(obj_filepath);
            free(asm_filepath);
            errorf("failed to execute assembler\n");
            exit(EXIT_FAILURE);
        }
    }
    int as_status = EXIT_FAILURE;
    waitpid(as_pid, &as_status, 0);
    as_status = WEXITSTATUS(as_status);
    if (as_status)
    {
        free(obj_filepath);
        free(asm_filepath);
        errorf("assembler has failed to produce an object file, cannot proceed with compilation\n");
        return NULL;
    }

    free(asm_filepath);
    return obj_filepath;
}

static void delete_array(void** arr, size_t length)
{
    for (size_t i = 0; i < length; ++i)
        free(arr[i]);
    free(arr);
}

int main(int argc, char** argv)
{
    PROGRAM_NAME = argv[0];
    srand(time(NULL));
    if (argc <= 1)
    {
        errorf("no input files\n");
        return EXIT_FAILURE;
    }
    memset(&opts, 0, sizeof(program_options_t));
    for (int c; (c = getopt(argc, argv, "hiPpax")) != -1;)
    {
        switch (c)
        {
            case 'h':
                return usage();
            case 'i':
                opts.iflag = true;
                break;
            case 'P':
                opts.ppflag = true;
                break;
            case 'p':
                opts.pflag = true;
                break;
            case 'a':
                opts.aflag = true;
                break;
            case 'x':
                opts.xflag = true;
                break;
            case '?':
            default:
            {
                errorf("unknown option specified: -%c\n", c);
                return EXIT_FAILURE;
            }
        }
    }

    size_t object_count = argc - optind;
    char** objects = calloc(object_count, sizeof(char*));
    for (unsigned i = optind; i < argc; ++i)
    {
        char* obj_filepath = work(argv[i]);
        if (obj_filepath == NULL)
        {
            delete_array((void**) objects, object_count);
            return EXIT_FAILURE;
        }
        objects[i - optind] = obj_filepath;
    }

    pid_t ld_pid = fork();
    if (ld_pid == -1)
    {
        delete_array((void**) objects, object_count);
        errorf("failed to spawn linker process\n");
        return EXIT_FAILURE;
    }

    if (ld_pid == 0)
    {
        #define NO_LIBRARIES 3
        char** argv = calloc(object_count + NO_LIBRARIES + 4, sizeof(char*));
        argv[0] = "/usr/bin/ld";
        for (size_t i = 0; i < object_count; ++i)
            argv[i + 1] = objects[i];
        argv[1 + object_count] = "lib/crt0.o";
        argv[2 + object_count] = "lib/sdef.o";
        argv[3 + object_count] = "lib/putint.o";
        argv[1 + object_count + NO_LIBRARIES] = "-o";
        argv[2 + object_count + NO_LIBRARIES] = "a.out";
        argv[3 + object_count + NO_LIBRARIES] = NULL;
        int status = execv("/usr/bin/ld", argv);
        if (status == -1)
        {
            free(argv);
            delete_array((void**) objects, object_count);
            errorf("failed to execute linker\n");
            exit(EXIT_FAILURE);
        }
    }

    int ld_status = EXIT_FAILURE;
    waitpid(ld_pid, &ld_status, 0);
    ld_status = WEXITSTATUS(ld_status);
    if (ld_status)
    {
        delete_array((void**) objects, object_count);
        errorf("linker has failed to produce an executable\n");
        return EXIT_FAILURE;
    }
    
    delete_array((void**) objects, object_count);
    return EXIT_SUCCESS;
}