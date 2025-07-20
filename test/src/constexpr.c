#include <stdlib.h>
#include <stdio.h>

#include "test.h"
#include "testutils.h"
#include "../../src/ecc.h"

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
    
    constexpr_t* ce = constexpr_evaluate_integer(initializer);
    if (!constexpr_evaluation_succeeded(ce))
    {
        if (test_debug) printf("could not evaluate the given expression\n");
        constexpr_delete(ce);
        free_return(false);
    }
    c_type_class_t class = ce->ct->class;
    constexpr_convert_class(ce, CTC_UNSIGNED_LONG_LONG_INT);
    uint64_t value = constexpr_as_u64(ce);
    constexpr_delete(ce);

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

#define constexpr_test(name, tlu, exp, exp_class) START_TEST(name) { return tconstexpr(tlu, exp, exp_class) ? OK : FAIL; } END_TEST

