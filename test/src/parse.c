#include <stdlib.h>
#include <stdio.h>

#include "test.h"
#include "testutils.h"
#include "../../src/cc.h"

static bool tparse(char* tlu_str)
{
    lexer_token_t* tokens = testutils_tokenize(tlu_str);
    syntax_component_t* tlu = parse(tokens);
    if (test_debug) print_syntax(tlu, printf);
    bool accepted = tlu != NULL;
    free_syntax(tlu, tlu);
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

parse_test(test_struct_basic_a, accept("struct { int a; };"));
parse_test(test_struct_named_a, accept("struct foo { int a; int b; };"));
parse_test(test_struct_bitfield_a, accept("struct foo { int a : 5; };"));
parse_test(test_struct_bitfield_no_declr_a, accept("struct foo { int : 5; };"));
parse_test(test_struct_bitfield_no_declr2_a, accept("struct foo { const : 5; };"));
parse_test(test_struct_specifier_a, accept("struct foo;"));
parse_test(test_struct_no_declr_r, reject("struct foo { int; };"));
parse_test(test_struct_empty_r, reject("struct foo { };"));
parse_test(test_struct_no_spec_qual_r, reject("struct foo { b; };"));
parse_test(test_struct_specifier_unnamed_r, reject("struct;"));

parse_test(test_union_a, accept("union foo { int b; };"));

parse_test(test_enum_basic_a, accept("enum { A };"));
parse_test(test_enum_named_a, accept("enum foo { B };"));
parse_test(test_enum_multiple_a, accept("enum foo { B, C };"));
parse_test(test_enum_comma_a, accept("enum foo { B, C, };"));
parse_test(test_enum_start_a, accept("enum foo { B = 2, C };"));
parse_test(test_enum_multistart_a, accept("enum foo { B = 2, C, D = 7, E };"));
parse_test(test_enum_specifier_a, accept("enum foo;"));
parse_test(test_enum_empty_r, reject("enum foo { };"));
parse_test(test_enum_specifier_unnamed_r, reject("enum;"));

parse_test(test_absdeclr_none_a, accept("int f(int);"));
parse_test(test_absdeclr_ptr_a, accept("int f(int *);"));
parse_test(test_absdeclr_array_a, accept("int f(int []);"));
parse_test(test_absdeclr_ptr_array_a, accept("int f(int *[]);"));
parse_test(test_absdeclr_array_ptr_a, accept("int f(int (*)[]);"));
parse_test(test_absdeclr_vla_ptr_a, accept("int f(int (*)[*]);"));
parse_test(test_absdeclr_func_ptr_a, accept("int f(int (*)(void));"));
parse_test(test_absdeclr_func_ptr_array_a, accept("int f(int (*const [])(unsigned int, ...));"));

parse_test(test_fdef_simple_a, accept("int main() {  }"));
