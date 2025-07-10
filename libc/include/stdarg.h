#ifndef _STDARG_H
#define _STDARG_H

#include "stdint.h"

typedef struct __ecc_va_list
{
    uintptr_t intpos;
    uintptr_t ssepos;
    uintptr_t stackpos;
} va_list;

#define va_arg(ap, type) __ecc_va_arg(ap, type)
#define va_copy(dest, src) __ecc_va_copy(dest, src)
#define va_end(ap) __ecc_va_end(ap)
#define va_start(ap, parmN) __ecc_va_start(ap, parmN)

#endif
