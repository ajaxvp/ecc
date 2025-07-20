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

#define accept(tlu) tanalyze(tlu) ? OK : FAIL
#define reject(tlu) tanalyze(tlu) ? FAIL : OK

#define analyze_test(name, t) START_TEST(name) { return (t); } END_TEST

analyze_test(test_multi_error_r, reject("register int a; register int b;"));
analyze_test(test_tlu_d_register_r, reject("register int a;"));
analyze_test(test_tlu_f_auto_r, reject("auto int f(void) { }"));
analyze_test(test_tlu_d_auto_r, reject("auto int a;"));
analyze_test(test_tlu_f_register_r, reject("register int f(void) { }"));
analyze_test(test_typespec_void_a, accept("void f(void) { }"));
analyze_test(test_typespec_invalid1_r, reject("unsigned unsigned f(void) { }"));
analyze_test(test_decl_empty_r, reject("int;"));
analyze_test(test_invalid_id_r, reject("int i = a;"));
