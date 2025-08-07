#include "../include/stdio.h"
#include "../include/stdint.h"
#include "../include/string.h"
#include "../include/stdlib.h"
#include "../../libecc/include/lsys.h"

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR 02

#define O_CREAT 0100
#define O_TRUNC	01000

static FILE files[FOPEN_MAX];

static uint8_t istack[] = {
    15, 14, 13, 12, 11, 10, 9, 8,
    7, 6, 5, 4, 3, 2, 1, 0
};

static size_t istackloc = 16;

FILE* fopen(const char* filename, const char* mode)
{
    if (istackloc == 0)
        return NULL;

    size_t len = strlen(mode);
    int flags = 0;
    if (len == 1)
    {
        if (mode[0] == 'r')
            flags = O_RDONLY;
        else if (mode[0] == 'w')
            flags = O_WRONLY | O_CREAT | O_TRUNC;
    }

    int fd = __ecc_lsys_open(filename, flags, 0644);
    if (fd == -1)
        return NULL;

    uint8_t i = istack[--istackloc];
    
    FILE* file = files + i;
    file->__fd = fd;
    file->__loc = i;

    return file;
}

int fclose(FILE* stream)
{
    if (!stream)
        return EOF;
    istack[istackloc++] = stream->__loc;
    return __ecc_lsys_close(stream->__fd);
}

int fgetc(FILE* stream)
{
    if (!stream)
        return EOF;
    char c;
    if (__ecc_lsys_read(stream->__fd, &c, 1) == 0)
        return EOF;
    return (int) (unsigned char) c;
}

int fputc(int ch, FILE* stream)
{
    if (!stream)
        return EOF;
    unsigned char c = (unsigned char) ch;
    if (__ecc_lsys_write(stream->__fd, (char*) &c, 1) == -1)
        return EOF;
    return ch;
}
