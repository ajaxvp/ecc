#include "../include/stdarg.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"

static int _ecc_printf_int(int i)
{
    if (!i)
    {
        putchar('0');
        return 1;
    }
    
    int count = 0;

    if (i < 0)
    {
        putchar('-');
        ++count;
    }

    i = abs(i);

    int j = i / 10;

    if (j)
        count += _ecc_printf_int(j);
    
    putchar((i % 10) + '0');
    return count + 1;
}

int printf(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);

    for (; *fmt; ++fmt)
    {
        char c = *fmt;
        if (c == '%')
        {
            c = *++fmt;
            if (c == 'd')
                _ecc_printf_int(va_arg(list, int));
        }
        else
            putchar(c);
    }

    va_end(list);
    return 0;
}
