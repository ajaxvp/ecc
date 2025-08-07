#include "../include/stdlib.h"
#include "../include/stdio.h"
#include "../include/stdint.h"
#include "../../libecc/include/lsys.h"

// guys please don't end me for this i swear i'm not evil
#define PAGE_SIZE 4096

// sys/mman.h macro constants
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_ANONYMOUS 0x20
#define MAP_PRIVATE 0x02

static uint8_t* __base = NULL;
static uint64_t __top = 0;

void* malloc(size_t size)
{
    if (__top + size >= PAGE_SIZE)
        return NULL;

    if (!__base)
        __base = __ecc_lsys_mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (!__base || __base == (uint8_t*) (-1))
    {
        puts("libc: memory could not be mapped");
        return NULL;
    }

    void* ptr = __base + __top;
    __top += size;
    return ptr;
}

void free(void* ptr)
{
    // freeing nd stuf
}
