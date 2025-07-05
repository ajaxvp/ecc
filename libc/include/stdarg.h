#ifndef _STDARG_H
#define _STDARG_H

#include <stdint.h>

typedef struct _ecc_va_list
{
    uint32_t intpos;
    uint32_t ssepos;
    uintptr_t stackpos;
} va_list;

#define va_arg(ap, type)
#define va_copy(dest, src)
#define va_end(ap)
#define va_start(ap, parmN)

#endif
