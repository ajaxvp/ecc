#ifndef TEST_H
#define TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_TESTS 4096

#define START_TEST(name) \
    test_exit_code_t name(void) \
    { \
        if (first_pass_test) \
        { \
            test_names[no_tests++] = test_strdup((char*) __func__); \
            return FIRST_PASS; \
        }

#define END_TEST }

#define EXPORT_TEST(name) test_exit_code_t name(void)

extern bool test_debug;

typedef enum test_exit_code
{
    OK = 0,
    FAIL = 1,
    FAIL_SEGFAULT = 2,
    FAIL_SETUP = 3,
    FIRST_PASS = 4
} test_exit_code_t;

extern bool first_pass_test;
extern size_t no_tests;
extern char* test_names[MAX_TESTS];

void add_test(test_exit_code_t (*t)(void));
void run_tests(int argc, char** argv);
char* test_strdup(char* str);

#endif