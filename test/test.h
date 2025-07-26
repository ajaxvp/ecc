#ifndef TEST_H
#define TEST_H

#include "../libc/include/stdio.h"

#define ASSERT(x, msg) if (!(x)) { puts(msg "\n"); }

#define ASSERT_EQUALS(x, y) if ((x) != (y)) { puts("'" #x "' should equal '" #y "'\n"); }

#define ASSERT_NOT_EQUALS(x, y) if ((x) == (y)) { puts("'" #x "' should not equal '" #y "'\n"); }

#endif
