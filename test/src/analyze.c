#include <stdlib.h>
#include <stdio.h>

#include "test.h"
#include "testutils.h"
#include "../../src/ecc.h"

static bool tanalyze(char* tlu_str)
{
    syntax_component_t* tlu = quickparse(tlu_str);
    if (test_debug) print_syntax(tlu, printf);
    if (!tlu)
        return false;
    analysis_error_t* type_errors = type(tlu);
    if (test_debug) dump_errors(type_errors);
    if (type_errors)
    {
        error_delete_all(type_errors);
        free_syntax(tlu, tlu);
        return false;
    }
    analysis_error_t* errors = analyze(tlu);
    if (test_debug) dump_errors(errors);
    bool accepted = error_list_size(errors, false) == 0;
    error_delete_all(errors);
    error_delete_all(type_errors);
    free_syntax(tlu, tlu);
    return accepted;
}

static bool tconstexpr(char* tlu_str, unsigned long long expected, c_type_class_t expected_class)
{
    syntax_component_t* tlu = quickparse(tlu_str);
    if (test_debug) print_syntax(tlu, printf);
    if (!tlu)
        return false;
    analysis_error_t* type_errors = type(tlu);
    if (test_debug) dump_errors(type_errors);

    #define free_return(value) \
        { \
            error_delete_all(type_errors); \
            free_syntax(tlu, tlu); \
            return (value); \
        }
    
    if (type_errors)
        free_return(false);
    
    symbol_t* sy = symbol_table_get_all(tlu->tlu_st, "i");
    
    if (!sy)
        free_return(false);
    
    syntax_component_t* full_declr = syntax_get_full_declarator(sy->declarer);

    if (!full_declr)
        free_return(false);
    
    if (full_declr->type != SC_INIT_DECLARATOR)
        free_return(false);
    
    syntax_component_t* initializer = full_declr->ideclr_initializer;

    if (!initializer || !syntax_is_expression_type(initializer->type))
        free_return(false);
    
    c_type_class_t class = CTC_ERROR;
    unsigned long long value = evaluate_constant_expression(initializer, &class, CE_ANY);

    if (class != expected_class)
    {
        if (test_debug) printf("expected class %s, got class %s\n", C_TYPE_CLASS_NAMES[expected_class], C_TYPE_CLASS_NAMES[class]);
        free_return(false);
    }
    
    if (value != expected)
    {
        if (test_debug) printf("expected value %llu, got value %llu\n", expected, value);
        free_return(false);
    }
    
    free_return(true);
    #undef free_return
}

#define accept(tlu) tanalyze(tlu) ? OK : FAIL
#define reject(tlu) tanalyze(tlu) ? FAIL : OK

#define analyze_test(name, t) START_TEST(name) { return (t); } END_TEST
#define constexpr_test(name, tlu, exp, exp_class) START_TEST(name) { return tconstexpr(tlu, exp, exp_class) ? OK : FAIL; } END_TEST

analyze_test(test_multi_error_r, reject("register int a; register int b;"));
analyze_test(test_tlu_d_register_r, reject("register int a;"));
analyze_test(test_tlu_f_auto_r, reject("auto int f(void) { }"));
analyze_test(test_tlu_d_auto_r, reject("auto int a;"));
analyze_test(test_tlu_f_register_r, reject("register int f(void) { }"));
analyze_test(test_typespec_void_a, accept("void f(void) { }"));
analyze_test(test_typespec_invalid1_r, reject("unsigned unsigned f(void) { }"));
analyze_test(test_decl_empty_r, reject("int;"));
analyze_test(test_invalid_id_r, reject("int i = a;"));

constexpr_test(test_constexpr_simple, "int i = 3;", 3, CTC_INT);
constexpr_test(test_constexpr_long, "int i = 5L;", 5, CTC_LONG_INT);
constexpr_test(test_constexpr_logical_or, "int i = 0 || 1;", 1, CTC_INT);
constexpr_test(test_constexpr_logical_and, "int i = (1 || 0) && 0;", 0, CTC_INT);
constexpr_test(test_constexpr_bitwise_or, "int i = 6 | 10;", 14, CTC_INT);
constexpr_test(test_constexpr_bitwise_xor, "int i = 72 ^ 72;", 0, CTC_INT);
constexpr_test(test_constexpr_bitwise_and, "int i = 11 & 6;", 2, CTC_INT);
constexpr_test(test_constexpr_enum_constant, "enum { A = 3 }; int i = A;", 3, CTC_INT);
constexpr_test(test_constexpr_enum_no_value, "enum { A, B }; int i = B;", 1, CTC_INT);
constexpr_test(test_constexpr_enum_sequential, "enum { A = 7, B, C = 5, D }; int i = D;", 6, CTC_INT);
