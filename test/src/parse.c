#include <stdlib.h>
#include <stdio.h>

#include "test.h"
#include "testutils.h"
#include "../../src/cc.h"

static bool tparse(char* tlu_str)
{
    lexer_token_t* tokens = testutils_tokenize(tlu_str);
    syntax_component_t* tlu = parse(tokens);
    bool accepted = tlu != NULL;
    free_syntax(tlu);
    lex_delete(tokens);
    return accepted;
}

#define accept(tlu) tparse(tlu) ? OK : FAIL
#define reject(tlu) tparse(tlu) ? FAIL : OK

#define parse_test(name, t) START_TEST(name) { return (t); } END_TEST

parse_test(test_declspec1_a, accept("int;"))
parse_test(test_declspec2_a, accept("void;"))
parse_test(test_declspec3_a, accept("long int long;"))
parse_test(test_declspec4_a, accept("const _Bool;"))
parse_test(test_declspec5_a, accept("int volatile const;"))
parse_test(test_declspec6_a, accept("inline short;"))
parse_test(test_declspec7_a, accept("long unsigned long inline;"))
parse_test(test_declspec8_a, accept("typedef;"))
parse_test(test_declspec9_a, accept("double;"))
parse_test(test_declspec10_a, accept("extern signed char;"))
parse_test(test_declspec11_a, accept("static float;"))
parse_test(test_declspec12_a, accept("_Complex;"))
parse_test(test_declspec13_a, accept("_Imaginary;"))

parse_test(test_declspec14_a, accept("int a;"))
parse_test(test_declspec15_a, accept("int *b;"))
parse_test(test_declspec16_a, accept("int (*c);"))
parse_test(test_declspec17_a, accept("int d[];"))
parse_test(test_declspec18_a, accept("int e(int x, int y);"))
parse_test(test_declspec19_a, accept("int f();"))
parse_test(test_declspec20_a, accept("int g(x, y);"))
parse_test(test_declspec21_a, accept("int h(int x, ...);"))
parse_test(test_declspec22_a, accept("int i[5];"))