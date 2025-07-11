#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#define _DEFAULT_SOURCE 1

#include <libgen.h>

#elif defined(_WIN32) || defined(__CYGWIN__)

#endif

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <wchar.h>

#include "ecc.h"

#define max(x, y) ((x) > (y) ? (x) : (y))

const bool debug_m = true;

// identical malloc'd copy of the parameter
char* strdup(const char* str)
{
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    dup[len] = '\0';
    strncpy(dup, str, len + 1);
    return dup;
}

// identical malloc'd copy widened to int
int* strdup_widen(const char* str)
{
    size_t len = strlen(str);
    int* dup = malloc(sizeof(int) * (len + 1));
    dup[len] = '\0';
    for (size_t i = 0; i < len; ++i)
        dup[i] = (int) str[i];
    return dup;
}

int* strdup_wide(const int* str)
{
    size_t len = wcslen(str);
    int* dup = malloc(sizeof(int) * (len + 1));
    dup[len] = '\0';
    for (size_t i = 0; i < len; ++i)
        dup[i] = str[i];
    return dup;
}

// farting
// 0123456
// substrdup("farting", 2, 4);

char* substrdup(const char* str, size_t start, size_t end)
{
    if (start > end)
        return NULL;
    size_t length = end - start;
    char* dup = malloc(length + 1);
    dup[length] = '\0';
    strncpy(dup, str + start, length + 1);
    return dup;
}

bool contains_substr(char* str, char* substr)
{
    size_t len_substr = strlen(substr);
    if (!len_substr)
        return true;
    size_t found = 0;
    for (; *str; ++str)
    {
        if (*str == substr[found])
            ++found;
        else
            found = 0;
        if (found == len_substr)
            return true;
    }
    return false;
}

bool contains_char(char* str, char c)
{
    for (; *str; ++str)
        if (*str == c)
            return true;
    return false;
}

bool streq(char* s1, char* s2)
{
    if (s1 == NULL && s2 == NULL)
        return true;
    return !strcmp(s1, s2);
}

bool streq_ignore_case(char* s1, char* s2)
{
    if (!s1 && !s2) return true;
    if (!s1 || !s2) return false;
    for (; *s1 && *s2; ++s1, ++s2)
        if (tolower(*s1) != tolower(*s2))
            return false;
    return *s1 == '\0' && *s2 == '\0';
}

bool in_debug(void)
{
    return debug_m;
}

int int_array_index_max(int* array, size_t length)
{
    if (!length)
        return -1;
    int mi = -1, max_value = -0x80000000;
    for (size_t i = 0; i < length; ++i)
    {
        if (array[i] > max_value)
        {
            max_value = array[i];
            mi = i;
        }
    }
    return mi;
}

void print_int_array(int* array, size_t length)
{
    printf("[");
    for (int i = 0; i < length; ++i)
    {
        if (i != 0)
            printf(", ");
        printf("%d", array[i]);
    }
    printf("]\n");
}

bool starts_ends_with_ignore_case(char* str, char* substr, bool ends)
{
    if (!str && !substr)
        return true;
    if (!str || !substr)
        return false;
    size_t str_len = strlen(str);
    size_t substr_len = strlen(substr);
    if (substr_len > str_len)
        return false;
    if (!substr_len)
        return true;
    size_t i = 0;
    for (; i < substr_len; ++i)
    {
        if (tolower(str[ends ? (str_len - 1 - i) : i]) != tolower(substr[ends ? (substr_len - 1 - i) : i]))
            return false;
    }
    return true;
}

// not finished but idrc
void repr_print(char* str, int (*printer)(const char* fmt, ...))
{
    for (; *str; ++str)
    {
        switch (*str)
        {
            case '\n':
                printer("\\n");
                break;
            case '\t':
                printer("\\t");
                break;
            case '\b':
                printer("\\b");
                break;
            default:
                printer("%c", *str);
                break;
        }
    }
}

// not mine
unsigned long hash(char* str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

// result is malloc'd
char* get_directory_path(char* path)
{
    if (!path)
        return strdup("");

    #if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

    char* fullpath = realpath(path, NULL);
    char* dir = strdup(dirname(fullpath));
    free(fullpath);
    return dir;

    #elif defined(_WIN32) || defined(__CYGWIN__)

    return NULL;

    #else

    return NULL;

    #endif
}

// m determines whether or not the filename uses an internal buffer (overwritten with successive calls) or gets malloc'd
char* get_file_name(char* path, bool m)
{
    static char result[LINUX_MAX_PATH_LENGTH];

    if (!path)
        return m ? strdup("") : (result[0] = '\0', result);

    #if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

    char* fullpath = realpath(path, NULL);
    char* name = m ? strdup(basename(fullpath)) : strncpy(result, basename(fullpath), LINUX_MAX_PATH_LENGTH);
    free(fullpath);
    return name;

    #elif defined(_WIN32) || defined(__CYGWIN__)

    return NULL;

    #else

    return NULL;

    #endif
}

// returns index if contains, otherwise -1
int contains(void** array, unsigned length, void* el, int (*c)(void*, void*))
{
    for (unsigned i = 0; i < length; ++i)
    {
        if (!c(array[i], el))
            return i;
    }
    return -1;
}

char* qb = NULL;
size_t qb_size = 0;
size_t qb_offset = 0;

int quickbuffer_printf(const char* fmt, ...)
{
    if (!qb) return -1;
    va_list args;
    va_start(args, fmt);
    int i = qb_offset += vsnprintf(qb + qb_offset, qb_size - qb_offset, fmt, args);
    va_end(args);
    return i;
}

void quickbuffer_setup(size_t size)
{
    qb = malloc(qb_size = size);
    qb_offset = 0;
}

void quickbuffer_release(void)
{
    free(qb);
    qb = NULL;
    qb_size = 0;
    qb_offset = 0;
}

char* quickbuffer(void)
{
    return qb;
}

// gets the actual value hex value for a char 0-9 or A-F or a-f
int hexadecimal_digit_value(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return (c - 'A') + 10;
    if (c >= 'a' && c <= 'f')
        return (c - 'a') + 10;
    return -1;
}

// expects a string of form "\uXXXX" or "\uXXXXXXXX"
// length should be either 6 or 10
unsigned get_universal_character_hex_value(char* unichar, size_t length)
{
    long long value = 0;
    for (int i = 0; i < (length == 6 ? 4 : 8); ++i)
        value = (value << 4) | hexadecimal_digit_value(unichar[i + 2]);
    return value;
}

unsigned get_universal_character_utf8_encoding(unsigned value)
{
    if (value < 0x80)
        return value;
    unsigned first = (value & 0xF) | (((value >> 4) & 0x3) << 4) | (0x2 << 6);
    if (value < 0x800)
        return first | (((value >> 6) & 0x3) << 8) | (((value >> 8) & 0xF) << 10) | (0x3 << 14);
    unsigned second = (((value >> 6) & 0x3) << 8) | (((value >> 8) & 0xF) << 10) | (0x2 << 14);
    if (value < 0x10000)
        return first | second | (0xE << 20) | (((value >> 12) & 0xF) << 16);
    unsigned third = ((value >> 12) & 0xF) << 16 | (((value >> 16) & 0x3) << 20) | (0x2 << 22);
    if (value < 0x110000)
        return first | second | third | (0xF << 28) | (((value >> 18) & 0x3) << 24) | (((value >> 20) & 0x1) << 26);
    return 0;
}

char* temp_filepath_gen(char* ext)
{
    char* cs = "1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";
    size_t extlen = strlen(ext);
    size_t pathlen = 5 + 32 + extlen + 1;
    char* filepath = malloc(pathlen);
    filepath[0] = '/';
    filepath[1] = 't';
    filepath[2] = 'm';
    filepath[3] = 'p';
    filepath[4] = '/';
    for (int i = 0; i < 32; ++i)
        filepath[i + 5] = cs[rand() % 62];
    for (int i = 0; i < extlen; ++i)
        filepath[37 + i] = ext[i];
    filepath[pathlen - 1] = '\0';
    return filepath;
}

int regid_comparator(regid_t r1, regid_t r2)
{
    return r2 - r1;
}

unsigned long regid_hash(regid_t x)
{
    return (unsigned long) x;
}

int regid_print(regid_t reg, int (*printer)(const char*, ...))
{
    if (reg > NO_PHYSICAL_REGISTERS)
        return printer("_%llu", reg - NO_PHYSICAL_REGISTERS);
    else if (reg == INVALID_VREGID)
        return printer("(invalid register)");
    else
        return printer("R%llu", reg);
}

char* replace_extension(char* filepath, char* ext)
{
    long long length = 0;
    long long ext_separator_index = -1;
    for (char* str = filepath; *str; ++str, ++length)
    {
        if (*str == '.')
            ext_separator_index = length;
    }
    if (ext_separator_index == -1)
        ext_separator_index = length;

    long long extlen = strlen(ext);

    long long finallen = ext_separator_index + extlen;

    char* str = malloc(finallen + 1);
    snprintf(str, finallen + 1, "%.*s%s", (int) ext_separator_index, filepath, ext);
    return str;
}
