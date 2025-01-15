#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "test.h"
#include "testutils.h"
#include "../../src/cc.h"

char buffer[1024];
size_t buffer_off = 0;

int fake_printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = buffer_off += vsnprintf(buffer + buffer_off, 1024 - buffer_off, fmt, args);
    va_end(args);
    return i;
}

static test_exit_code_t ttype1(char* tlu_str, char* type_str)
{
    lexer_token_t* tokens = testutils_tokenize(tlu_str);
    syntax_component_t* tlu = parse(tokens);
    if (!tlu)
    {
        free_syntax(tlu, tlu);
        lex_delete(tokens);
        return FAIL;
    }
    analysis_error_t* errors = type(tlu);
    if (test_debug) print_syntax(tlu, printf);
    if (test_debug) symbol_table_print(tlu->tlu_st, printf);
    if (test_debug) dump_errors(errors);
    symbol_t* sylist = symbol_table_get_all(tlu->tlu_st, "i");
    bool pass = false;
    if (sylist)
    {
        buffer_off = 0;
        type_humanized_print(sylist->type, fake_printf);
        if (test_debug) printf("got type: %s\n", buffer);
        pass = !strcmp(type_str, buffer);
    }
    error_delete_all(errors);
    free_syntax(tlu, tlu);
    lex_delete(tokens);
    return pass ? OK : FAIL;
}

#define type_test1(name, tlu, type_str) START_TEST(name) { return ttype1(tlu, type_str); } END_TEST

type_test1(test_typing_simple, "int i;", "int");
type_test1(test_typing_nested, "_Bool (i);", "_Bool");
type_test1(test_typing_ptr, "float *i;", "pointer to float");
type_test1(test_typing_function, "long long i(int x);", "function(int) returning long long int");
type_test1(test_typing_interleaved, "int const long i[];", "array of long int");
type_test1(test_typing_function_ptr, "short int (*i)(void);", "pointer to function(void) returning short int");
type_test1(test_typing_function_ptr_array, "char signed (*i[5])(void);", "array of pointer to function(void) returning signed char");
type_test1(test_typing_struct, "struct point { int x; int y; } i;", "struct point");
type_test1(test_typing_enum, "enum i { X };", "enum i");
type_test1(test_typing_enum_constant, "enum x { i };", "int");
type_test1(test_typing_abs_declr, "int i(int (*)(const char*, ...));", "function(pointer to function(pointer to char, ...) returning int) returning int");