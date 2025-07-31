#include "../include/stdio.h"
#include "../include/stdint.h"
#include "../include/string.h"
#include "../include/stdlib.h"

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
            flags = 0;
        else if (mode[0] == 'w')
            flags = 1;
    }

    int fd = __ecc_lsys_open(filename, flags, 0);
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
