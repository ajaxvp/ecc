#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdio.h>
#include <getopt.h>

#include "test.h"

bool test_debug = false;

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

/*
  int option;
  while ((option = getopt (argc, argv, "aps")) != -1)
  {
    switch (option)
      {
      case 'a':
        *all = true;
        break;
      case 'p':
        *perms = true;
        break;
      case 's':
        *sizes = true;
        break;
      default:
        return -1;
      }
  }
  return 0;
*/

bool handle_options(int argc, char** argv)
{
    for (int op; (op = getopt(argc, argv, "d")) != -1;)
    {
        switch (op)\
        {
            case 'd':
                test_debug = true;
                break;
            default:
                opterr = 0;
                return false;
        }
    }
    return true;
}

bool contains(char** array, size_t length, char* item)
{
    for (size_t i = 0; i < length; ++i)
    {
        if (!strcmp(array[i], item))
            return true;
    }
    return false;
}

void run_tests(int argc, char** argv)
{
    if (!handle_options(argc, argv))
    {
        printf("usage: %s [-d] [tests...]\n", argv[0]);
        return;
    }
    bool only_selected_tests = optind < argc;
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
    {
        if (!only_selected_tests || contains(argv + optind, argc - optind, test_names[i]))
            test(tests[i], i);
    }
    size_t total = !only_selected_tests ? no_tests : argc - optind;
    printf("passed %ld/%ld (%.1f%%) test(s)!\n", passed_tests, total, ((double) passed_tests / (double) total) * 100.0);
    return;
}