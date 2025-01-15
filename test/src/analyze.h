#ifndef ANALYZE_H
#define ANALYZE_H

#include "test.h"

EXPORT_TEST(test_multi_error_r);
EXPORT_TEST(test_tlu_d_register_r);
EXPORT_TEST(test_tlu_f_auto_r);
EXPORT_TEST(test_tlu_d_auto_r);
EXPORT_TEST(test_tlu_f_register_r);
EXPORT_TEST(test_typespec_void_a);
EXPORT_TEST(test_typespec_invalid1_r);
EXPORT_TEST(test_decl_empty_r);
EXPORT_TEST(test_invalid_id_r);
EXPORT_TEST(test_constexpr_simple);
EXPORT_TEST(test_constexpr_long);
EXPORT_TEST(test_constexpr_logical_or);
EXPORT_TEST(test_constexpr_logical_and);
EXPORT_TEST(test_constexpr_bitwise_or);
EXPORT_TEST(test_constexpr_bitwise_xor);
EXPORT_TEST(test_constexpr_bitwise_and);
EXPORT_TEST(test_constexpr_enum_constant);
EXPORT_TEST(test_constexpr_enum_no_value);
EXPORT_TEST(test_constexpr_enum_sequential);

#endif
