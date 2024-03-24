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
    int i = lf(stdout, "warn", fmt, args);
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