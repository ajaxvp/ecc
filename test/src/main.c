#include "test.h"

#include "parse.h"
#include "analyze.h"
#include "type.h"

int main(int argc, char** argv)
{
    add_test(test_declspec1_a);
    add_test(test_declspec2_a);
    add_test(test_declspec3_a);
    add_test(test_declspec4_a);
    add_test(test_declspec5_a);
    add_test(test_declspec6_a);
    add_test(test_declspec7_a);
    add_test(test_declspec8_a);
    add_test(test_declspec9_a);
    add_test(test_declspec10_a);
    add_test(test_declspec11_a);
    add_test(test_declspec12_a);
    add_test(test_declspec13_a);
    add_test(test_declspec14_a);
    add_test(test_declspec15_a);
    add_test(test_declspec16_a);
    add_test(test_declspec17_a);
    add_test(test_declspec18_a);
    add_test(test_declspec19_a);
    add_test(test_declspec20_a);
    add_test(test_declspec21_a);
    add_test(test_declspec22_a);
    add_test(test_struct_basic_a);
    add_test(test_struct_named_a);
    add_test(test_struct_multi_declr_a);
    add_test(test_struct_bitfield_a);
    add_test(test_struct_bitfield_no_declr_a);
    add_test(test_struct_bitfield_no_declr2_a);
    add_test(test_struct_specifier_a);
    add_test(test_struct_nested_a);
    add_test(test_struct_no_declr_r);
    add_test(test_struct_empty_r);
    add_test(test_struct_no_spec_qual_r);
    add_test(test_struct_storage_class_r);
    add_test(test_struct_func_specifier_r);
    add_test(test_struct_specifier_unnamed_r);
    add_test(test_union_a);
    add_test(test_enum_basic_a);
    add_test(test_enum_named_a);
    add_test(test_enum_multiple_a);
    add_test(test_enum_comma_a);
    add_test(test_enum_start_a);
    add_test(test_enum_multistart_a);
    add_test(test_enum_specifier_a);
    add_test(test_enum_empty_r);
    add_test(test_enum_specifier_unnamed_r);
    add_test(test_absdeclr_none_a);
    add_test(test_absdeclr_ptr_a);
    add_test(test_absdeclr_array_a);
    add_test(test_absdeclr_ptr_array_a);
    add_test(test_absdeclr_array_ptr_a);
    add_test(test_absdeclr_vla_ptr_a);
    add_test(test_absdeclr_func_ptr_a);
    add_test(test_absdeclr_func_ptr_array_a);
    add_test(test_typedef_a);
    add_test(test_fdef_basic_a);
    add_test(test_fdef_param_a);
    add_test(test_fdef_knr_a);
    add_test(test_fdef_mix_r);
    add_test(test_fdef_no_body_r);
    add_test(test_if_basic_a);
    add_test(test_if_else_a);
    add_test(test_if_no_condition_r);
    add_test(test_if_no_body_r);
    add_test(test_if_empty_else_r);
    add_test(test_switch_a);
    add_test(test_switch_labels_a);
    add_test(test_switch_no_condition_r);
    add_test(test_switch_no_body_r);
    add_test(test_while_basic_a);
    add_test(test_while_no_condition_r);
    add_test(test_while_no_body_r);
    add_test(test_for_decl_a);
    add_test(test_for_expr_a);
    add_test(test_for_no_init_a);
    add_test(test_for_no_condition_a);
    add_test(test_for_no_post_a);
    add_test(test_for_nothing_a);
    add_test(test_for_no_semicolon_r);
    add_test(test_for_one_semicolon_r);
    add_test(test_for_no_header_r);
    add_test(test_for_no_body_r);
    add_test(test_do_basic_a);
    add_test(test_do_no_condition1_r);
    add_test(test_do_no_condition2_r);
    add_test(test_do_no_body_r);
    add_test(test_do_no_semicolon_r);
    add_test(test_label_basic_a);
    add_test(test_label_no_id_r);
    add_test(test_goto_basic_a);
    add_test(test_goto_no_label_r);
    add_test(test_continue_a);
    add_test(test_break_a);
    add_test(test_return_basic_a);
    add_test(test_return_expr_a);
    add_test(test_compound_basic_a);
    add_test(test_compound_items_a);
    add_test(test_compound_no_end_brace_r);
    add_test(test_expression_stmt_basic_a);
    add_test(test_expression_stmt_expr_a);
    add_test(test_initializer_basic_a);
    add_test(test_initializer_array_a);
    add_test(test_initializer_array_designations_a);
    add_test(test_initializer_comma_a);
    add_test(test_initializer_struct_a);
    add_test(test_initializer_struct_designations_a);
    add_test(test_initializer_repeated_array_designations_a);
    add_test(test_initializer_repeated_struct_designations_a);
    add_test(test_initializer_nested_a);
    add_test(test_expr_basic_a);
    add_test(test_expr_comma_a);
    add_test(test_expr_assign_a);
    add_test(test_expr_ternary_a);
    add_test(test_expr_logical_or_a);
    add_test(test_expr_logical_and_a);
    add_test(test_expr_bitwise_or_a);
    add_test(test_expr_bitwise_xor_a);
    add_test(test_expr_bitwise_and_a);
    add_test(test_expr_equality_a);
    add_test(test_expr_inequality_a);
    add_test(test_expr_lt_a);
    add_test(test_expr_lteq_a);
    add_test(test_expr_gt_a);
    add_test(test_expr_gteq_a);
    add_test(test_expr_shl_a);
    add_test(test_expr_shr_a);
    add_test(test_expr_add_a);
    add_test(test_expr_sub_a);
    add_test(test_expr_mult_a);
    add_test(test_expr_div_a);
    add_test(test_expr_mod_a);
    add_test(test_expr_cast_a);
    add_test(test_expr_prefix_inc_a);
    add_test(test_expr_prefix_dec_a);
    add_test(test_expr_ref_a);
    add_test(test_expr_deref_a);
    add_test(test_expr_plus_a);
    add_test(test_expr_minus_a);
    add_test(test_expr_complement_a);
    add_test(test_expr_not_a);
    add_test(test_expr_sizeof_expr_a);
    add_test(test_expr_sizeof_type_a);
    add_test(test_expr_postfix_inc_a);
    add_test(test_expr_postfix_dec_a);
    add_test(test_expr_member_a);
    add_test(test_expr_deref_member_a);
    add_test(test_expr_subscript_a);
    add_test(test_expr_function_call_a);
    add_test(test_expr_compound_literal_a);
    add_test(test_expr_compound_literal_comma_a);
    add_test(test_expr_id_a);
    add_test(test_expr_integer_constant_a);
    add_test(test_expr_floating_constant_a);
    add_test(test_expr_character_constant_a);
    add_test(test_expr_string_literal_a);
    add_test(test_expr_nested_a);
    add_test(test_expr_precedence_a);
    add_test(test_expr_nested_twice_a);
    add_test(test_type_name_basic_a);
    add_test(test_type_name_ptr_a);
    add_test(test_type_name_fspec_r);
    add_test(test_type_name_storage_class_r);

    add_test(test_multi_error_r);
    add_test(test_tlu_d_register_r);
    add_test(test_tlu_f_auto_r);
    add_test(test_tlu_d_auto_r);
    add_test(test_tlu_f_register_r);
    add_test(test_typespec_void_a);
    add_test(test_typespec_invalid1_r);
    add_test(test_decl_empty_r);
    add_test(test_invalid_id_r);
    add_test(test_constexpr_simple);
    add_test(test_constexpr_long);
    add_test(test_constexpr_logical_or);
    add_test(test_constexpr_logical_and);
    add_test(test_constexpr_bitwise_or);
    add_test(test_constexpr_bitwise_xor);
    add_test(test_constexpr_bitwise_and);
    add_test(test_constexpr_enum_constant);
    add_test(test_constexpr_enum_no_value);
    add_test(test_constexpr_enum_sequential);

    add_test(test_typing_simple);
    add_test(test_typing_nested);
    add_test(test_typing_ptr);
    add_test(test_typing_function);
    add_test(test_typing_interleaved);
    add_test(test_typing_function_ptr);
    add_test(test_typing_function_ptr_array);
    add_test(test_typing_struct);
    add_test(test_typing_enum);
    add_test(test_typing_enum_constant);
    add_test(test_typing_abs_declr);

    run_tests(argc, argv);
}
