#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdio.h>

#include "test.h"

bool first_pass_test = true;
size_t no_tests = 0;
size_t passed_tests = 0;
char* test_names[MAX_TESTS];
test_exit_code_t (*tests[MAX_TESTS])(void);

char* test_strdup(char* str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    dup[len] = '\0';
    strncpy(dup, str, len + 1);
    return dup;
}

void segfault_handler(int signo)
{
    exit(FAIL_SEGFAULT);
}

void test(test_exit_code_t (*fn)(void), size_t index)
{
    pid_t pid = fork();
    if (!pid)
    {
        struct sigaction sa = {0};
        sa.sa_handler = segfault_handler;
        if (sigaction(SIGSEGV, &sa, NULL) == -1)
            exit(FAIL_SETUP);
        exit(fn());
    }
    int status;
    waitpid(pid, &status, 0);
    test_exit_code_t code = WEXITSTATUS(status);
    printf("%s: ", test_names[index]);
    free(test_names[index]);
    switch (code)
    {
        case OK:
            ++passed_tests;
            printf("pass\n");
            break;
        case FAIL:
            printf("fail\n");
            break;
        case FAIL_SEGFAULT:
            printf("fail (segfault)\n");
            break;
        case FAIL_SETUP:
            printf("could not test\n");
            break;
        default:
            break;
    }
}

void add_test(test_exit_code_t (*t)(void))
{
    if (no_tests >= MAX_TESTS)
    {
        printf("failed to add test: too many tests\n");
        return;
    }
    tests[no_tests++] = t;
}

void run_tests(void)
{
    if (!no_tests)
    {
        printf("no tests to run\n");
        return;
    }
    size_t nt = no_tests;
    no_tests = 0;
    for (size_t i = 0; i < nt; ++i)
        tests[i]();
    first_pass_test = false;
    for (size_t i = 0; i < nt; ++i)
        test(tests[i], i);
    printf("passed %ld/%ld (%.1f%%) tests!\n", passed_tests, no_tests, ((double) passed_tests / (double) no_tests) * 100.0);
}