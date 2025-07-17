#ifndef _STDIO_H
#define _STDIO_H

#define FOPEN_MAX 16

typedef struct __ecc_file
{
    int __fd;
} FILE;

// FILE* fopen(const char* filename, const char* mode);

int putchar(int c);

int puts(const char* str);

int printf(const char* fmt, ...);

#endif
