#ifndef _STDIO_H
#define _STDIO_H

#include "stdint.h"

#define FOPEN_MAX 16
#define EOF (-1)

typedef struct __ecc_file
{
    int __fd;
    uint8_t __loc;
} FILE;

FILE* fopen(const char* filename, const char* mode);

int fclose(FILE* stream);

int fgetc(FILE* stream);

int putchar(int c);

int puts(const char* str);

int printf(const char* fmt, ...);

#endif
