#include "test.h"

#include "parse.h"

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
    add_test(test_struct_bitfield_a);
    add_test(test_struct_bitfield_no_declr_a);
    add_test(test_struct_bitfield_no_declr2_a);
    add_test(test_struct_specifier_a);
    add_test(test_struct_no_declr_r);
    add_test(test_struct_empty_r);
    add_test(test_struct_no_spec_qual_r);
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
    add_test(test_fdef_simple_a);

    run_tests(argc, argv);
}
