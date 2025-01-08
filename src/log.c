#include <stdio.h>
#include <stdarg.h>

static int lf(FILE* file, char* form, char* fmt, va_list args)
{
    int i = 0;
    i += fprintf(file, "cc: ");
    i += fprintf(file, "%s: ", form);
    i += vfprintf(file, fmt, args);
    return i;
}

static int snlf(char* buffer, size_t maxlen, char* form, char* fmt, va_list args)
{
    int i = 0;
    i += snprintf(buffer, maxlen, "cc: %s: ", form);
    i += vsnprintf(buffer, maxlen -= i, fmt, args);
    return i;
}

int infof(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = lf(stdout, "info", fmt, args);
    va_end(args);
    return i;
}

int warnf(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = lf(stderr, "warn", fmt, args);
    va_end(args);
    return i;
}

int errorf(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = lf(stderr, "error", fmt, args);
    va_end(args);
    return i;
}

int sninfof(char* buffer, size_t maxlen, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = snlf(buffer, maxlen, "info", fmt, args);
    va_end(args);
    return i;
}

int snwarnf(char* buffer, size_t maxlen, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = snlf(buffer, maxlen, "warn", fmt, args);
    va_end(args);
    return i;
}

int snerrorf(char* buffer, size_t maxlen, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int i = snlf(buffer, maxlen, "error", fmt, args);
    va_end(args);
    return i;
}