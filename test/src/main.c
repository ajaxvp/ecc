#include "test.h"

#include "parse.h"

int main(void)
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
    add_test(test_struct_no_declr_r);
    add_test(test_struct_empty_r);
    add_test(test_struct_no_spec_qual_r);

    add_test(test_union_a);

    run_tests();
}