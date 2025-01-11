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
parse_test(test_struct_multi_declr_a, accept("struct foo { int a, b; };"));
parse_test(test_struct_bitfield_a, accept("struct foo { int a : 5; };"));
parse_test(test_struct_bitfield_no_declr_a, accept("struct foo { int : 5; };"));
parse_test(test_struct_bitfield_no_declr2_a, accept("struct foo { const : 5; };"));
parse_test(test_struct_specifier_a, accept("struct foo;"));
parse_test(test_struct_nested_a, accept("struct foo { struct bar { int x; } b; };"));
parse_test(test_struct_no_declr_r, reject("struct foo { int; };"));
parse_test(test_struct_empty_r, reject("struct foo { };"));
parse_test(test_struct_no_spec_qual_r, reject("struct foo { b; };"));
parse_test(test_struct_storage_class_r, reject("struct foo { extern b; };"));
parse_test(test_struct_func_specifier_r, reject("struct foo { inline b; };"));
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

parse_test(test_typedef_a, accept("typedef int num; num a;"));

parse_test(test_fdef_basic_a, accept("int f(void) { }"));
parse_test(test_fdef_param_a, accept("int f(int a, int b) { }"));
parse_test(test_fdef_knr_a, accept("int f(a, b) int a; int b; { }"));
parse_test(test_fdef_mix_r, reject("int f(int a, b) int b; { }"));
parse_test(test_fdef_no_body_r, reject("int f(void)"));

parse_test(test_if_basic_a, accept("int f(void) { if (1) { } }"));
parse_test(test_if_else_a, accept("int f(void) { if (1) ; else { } }"));
parse_test(test_if_no_condition_r, reject("int f(void) { if { } }"));
parse_test(test_if_no_body_r, reject("int f(void) { if (1) }"));
parse_test(test_if_empty_else_r, reject("int f(void) { if (1) else }"));

parse_test(test_switch_a, accept("int f(void) { switch (1) { } }"));
parse_test(test_switch_labels_a, accept("int f(void) { switch (1) { case 1: break; default: break; } }"));
parse_test(test_switch_no_condition_r, reject("int f(void) { switch { } }"));
parse_test(test_switch_no_body_r, reject("int f(void) { switch (1) }"));

parse_test(test_while_basic_a, accept("int f(void) { while (1) { } }"));
parse_test(test_while_no_condition_r, reject("int f(void) { while { } }"));
parse_test(test_while_no_body_r, reject("int f(void) { while (1) }"));

parse_test(test_for_decl_a, accept("int f(void) { for (int i; i; i) { } }"));
parse_test(test_for_expr_a, accept("int f(void) { int i; for (i; i; i) { } }"));
parse_test(test_for_no_init_a, accept("int f(void) { int i; for (; i; i) { } }"));
parse_test(test_for_no_condition_a, accept("int f(void) { int i; for (i; ; i) { } }"));
parse_test(test_for_no_post_a, accept("int f(void) { int i; for (i; i; ) { } }"));
parse_test(test_for_nothing_a, accept("int f(void) { for (;;) { } }"));
parse_test(test_for_no_semicolon_r, reject("int f(void) { for () { } }"));
parse_test(test_for_one_semicolon_r, reject("int f(void) { for (;) { } }"));
parse_test(test_for_no_header_r, reject("int f(void) { for { } }"));
parse_test(test_for_no_body_r, reject("int f(void) { for (;;) }"));

parse_test(test_do_basic_a, accept("int f(void) { do { } while (1); }"));
parse_test(test_do_no_condition1_r, reject("int f(void) { do { } while }"));
parse_test(test_do_no_condition2_r, reject("int f(void) { do { } }"));
parse_test(test_do_no_body_r, reject("int f(void) { do while (1); }"));
parse_test(test_do_no_semicolon_r, reject("int f(void) { do { } while (1) }"));

parse_test(test_label_basic_a, accept("int f(void) { l: ; }"));
parse_test(test_label_no_id_r, reject("int f(void) { : ; }"));

parse_test(test_goto_basic_a, accept("int f(void) { l: goto l; }"));
parse_test(test_goto_no_label_r, reject("int f(void) { goto; }"));

parse_test(test_continue_a, accept("int f(void) { while (1) { continue; } }"));

parse_test(test_break_a, accept("int f(void) { while (1) { break; } }"));

parse_test(test_return_basic_a, accept("void f(void) { return; }"));
parse_test(test_return_expr_a, accept("int f(void) { return 0; }"));

parse_test(test_compound_basic_a, accept("int f(void) { { } }"));
parse_test(test_compound_items_a, accept("int f(void) { { int a; int b; } int c; }"));
parse_test(test_compound_no_end_brace_r, reject("int f(void) { { int a; }"));

parse_test(test_expression_stmt_basic_a, accept("int f(void) { ; }"));
parse_test(test_expression_stmt_expr_a, accept("int f(void) { 5; }"));

parse_test(test_initializer_basic_a, accept("int i = 5;"));
parse_test(test_initializer_array_a, accept("int a[5] = { 0, 1, 2, 3, 4 };"));
parse_test(test_initializer_array_designations_a, accept("int a[5] = { [0] = 3, [2] = 5 };"));
parse_test(test_initializer_comma_a, accept("int a[5] = { [0] = 3, };"));
parse_test(test_initializer_struct_a, accept("struct point { int x; int y; } p = { 0, 0 };"));
parse_test(test_initializer_struct_designations_a, accept("struct point { int x; int y; } p = { .y = 5, .x = 3 };"));
parse_test(test_initializer_repeated_array_designations_a, accept("int a[5][5] = { [0][0] = 3, [2][0] = 5 };"));
parse_test(test_initializer_repeated_struct_designations_a, accept("struct foo { struct bar { int x; } b; } f = { .b.x = 0 };"));
parse_test(test_initializer_nested_a, accept("struct foo { struct bar { int x; } b; } f = { { .x = 0 } };"));

parse_test(test_expr_basic_a, accept("int f(void) { 5; }"));
parse_test(test_expr_comma_a, accept("int f(void) { 5, 6; }"));
parse_test(test_expr_assign_a, accept("int f(void) { int a; a = 5; }"));
parse_test(test_expr_ternary_a, accept("int f(void) { 0 ? 5 : 3; }"));
parse_test(test_expr_logical_or_a, accept("int f(void) { 5 || 3; }"));
parse_test(test_expr_logical_and_a, accept("int f(void) { 5 && 3; }"));
parse_test(test_expr_bitwise_or_a, accept("int f(void) { 5 | 3; }"));
parse_test(test_expr_bitwise_xor_a, accept("int f(void) { 5 ^ 3; }"));
parse_test(test_expr_bitwise_and_a, accept("int f(void) { 5 & 3; }"));
parse_test(test_expr_equality_a, accept("int f(void) { 5 == 3; }"));
parse_test(test_expr_inequality_a, accept("int f(void) { 5 != 3; }"));
parse_test(test_expr_lt_a, accept("int f(void) { 5 < 3; }"));
parse_test(test_expr_lteq_a, accept("int f(void) { 5 <= 3; }"));
parse_test(test_expr_gt_a, accept("int f(void) { 5 > 3; }"));
parse_test(test_expr_gteq_a, accept("int f(void) { 5 >= 3; }"));
parse_test(test_expr_shl_a, accept("int f(void) { 5 << 3; }"));
parse_test(test_expr_shr_a, accept("int f(void) { 5 >> 3; }"));
parse_test(test_expr_add_a, accept("int f(void) { 5 + 3; }"));
parse_test(test_expr_sub_a, accept("int f(void) { 5 - 3; }"));
parse_test(test_expr_mult_a, accept("int f(void) { 5 * 3; }"));
parse_test(test_expr_div_a, accept("int f(void) { 5 / 3; }"));
parse_test(test_expr_mod_a, accept("int f(void) { 5 % 3; }"));
parse_test(test_expr_cast_a, accept("int f(void) { (short) 5; }"));
parse_test(test_expr_prefix_inc_a, accept("int f(void) { int a; ++a; }"));
parse_test(test_expr_prefix_dec_a, accept("int f(void) { int a; --a; }"));
parse_test(test_expr_ref_a, accept("int f(void) { int a; &a; }"));
parse_test(test_expr_deref_a, accept("int f(void) { int *a; *a; }"));
parse_test(test_expr_plus_a, accept("int f(void) { int a; +a; }"));
parse_test(test_expr_minus_a, accept("int f(void) { int a; -a; }"));
parse_test(test_expr_complement_a, accept("int f(void) { int a; ~a; }"));
parse_test(test_expr_not_a, accept("int f(void) { int a; !a; }"));
parse_test(test_expr_sizeof_expr_a, accept("int f(void) { int a; sizeof a; }"));
parse_test(test_expr_sizeof_type_a, accept("int f(void) { sizeof(int); }"));
parse_test(test_expr_postfix_inc_a, accept("int f(void) { int a; a++; }"));
parse_test(test_expr_postfix_dec_a, accept("int f(void) { int a; a--; }"));
parse_test(test_expr_member_a, accept("int f(void) { struct point { int x; int y; } p; p.x = 5; }"));
parse_test(test_expr_deref_member_a, accept("int f(void) { struct point { int x; int y; }; struct point p; struct point *ptr = &p; ptr->x = 5; }"));
parse_test(test_expr_subscript_a, accept("int f(void) { int a[5]; a[0]; }"));
parse_test(test_expr_function_call_a, accept("int g(int x, int y); int f(void) { g(5, 3); }"));
parse_test(test_expr_compound_literal_a, accept("int f(void) { struct point { int x; int y; }; (struct point) { .x = 5, .y = 3 }; }"));
parse_test(test_expr_compound_literal_comma_a, accept("int f(void) { struct point { int x; int y; }; (struct point) { .x = 0, }; }"));
parse_test(test_expr_id_a, accept("int f(void) { int a; a; }"));
parse_test(test_expr_integer_constant_a, accept("int f(void) { 5; }"));
parse_test(test_expr_floating_constant_a, accept("int f(void) { 5.3; }"));
parse_test(test_expr_character_constant_a, accept("int f(void) { 'a'; }"));
parse_test(test_expr_string_literal_a, accept("int f(void) { \"abc\"; }"));
parse_test(test_expr_nested_a, accept("int f(void) { (5 + 3) * 6; }"));
parse_test(test_expr_precedence_a, accept("int f(void) { 5 + 3 * 6; }"));
parse_test(test_expr_nested_twice_a, accept("int f(void) { 5 / ((5 + 3) * 6); }"));

parse_test(test_type_name_basic_a, accept("int f(void) { int a = 5; (short) a; }"));
parse_test(test_type_name_ptr_a, accept("int f(void) { int a = 5; (const*) a; }"));
parse_test(test_type_name_fspec_r, reject("int f(void) { int a = 5; (inline) a; }"));
parse_test(test_type_name_storage_class_r, reject("int f(void) { int a = 5; (extern) a; }"));