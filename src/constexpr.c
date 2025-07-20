#include <stdlib.h>
#include <string.h>

#include "ecc.h"

#define SYMBOL_TABLE (syntax_get_translation_unit(expr)->tlu_st)
#define SET_ERROR(syn, fmt, ...) (ce->error = malloc(MAX_ERROR_LENGTH), snerrorf(ce->error, MAX_ERROR_LENGTH, "[%u:%u] " fmt, syn->row, syn->col, ## __VA_ARGS__))

void constexpr_delete(constexpr_t* ce)
{
    if (!ce) return;
    type_delete(ce->ct);
    switch (ce->type)
    {
        case CE_ADDRESS:
            break;
        case CE_ARITHMETIC:
        case CE_INTEGER:
            free(ce->content.data);
            break;
    }
    free(ce->error);
    free(ce);
}

constexpr_t* constexpr_copy(constexpr_t* ce)
{
    constexpr_t* n = calloc(1, sizeof *n);
    n->type = ce->type;
    n->ct = type_copy(ce->ct);
    if (ce->error)
        n->error = strdup(ce->error);
    switch (n->type)
    {
        case CE_INTEGER:
        case CE_ARITHMETIC:
            memcpy(n->content.data, ce->content.data, type_size(n->ct));
            break;
        case CE_ADDRESS:
            n->content.addr.sy = ce->content.addr.sy;
            n->content.addr.negative_offset = ce->content.addr.negative_offset;
            n->content.addr.offset = ce->content.addr.offset;
            break;
    }
    return n;
}

#define data_as(data, type) (*((type*) (data)))
#define to_data(obj) ((uint8_t*) &(obj))

void constexpr_print_value(constexpr_t* ce, int (*printer)(const char*, ...))
{
    if (!ce) return;

    if (ce->type == CE_ADDRESS)
    {
        if (ce->content.addr.sy)
            printer("&%s + %lld", symbol_get_name(ce->content.addr.sy), ce->content.addr.offset);
        else
            printer("(void*) 0x%X", ce->content.addr.offset);
        return;
    }

    switch (ce->ct->class)
    {
        case CTC_BOOL:
            printer("%s", data_as(ce->content.data, bool) ? "true" : "false");
            break;
        case CTC_CHAR:
            printer("%hhd", data_as(ce->content.data, char));
            break;
        case CTC_SIGNED_CHAR:
            printer("%hhd", data_as(ce->content.data, signed char));
            break;
        case CTC_SHORT_INT:
            printer("%hd", data_as(ce->content.data, short));
            break;
        case CTC_INT:
            printer("%d", data_as(ce->content.data, int));
            break;
        case CTC_LONG_INT:
            printer("%ld", data_as(ce->content.data, long));
            break;
        case CTC_LONG_LONG_INT:
            printer("%lld", data_as(ce->content.data, long long));
            break;
        case CTC_UNSIGNED_CHAR:
            printer("%hhu", data_as(ce->content.data, unsigned char));
            break;
        case CTC_UNSIGNED_SHORT_INT:
            printer("%hu", data_as(ce->content.data, unsigned short));
            break;
        case CTC_UNSIGNED_INT:
            printer("%u", data_as(ce->content.data, unsigned));
            break;
        case CTC_UNSIGNED_LONG_INT:
            printer("%lu", data_as(ce->content.data, unsigned long));
            break;
        case CTC_UNSIGNED_LONG_LONG_INT:
            printer("%llu", data_as(ce->content.data, unsigned long long));
            break;
        case CTC_FLOAT:
            printer("%f", (double) data_as(ce->content.data, float));
            break;
        case CTC_DOUBLE:
            printer("%f", data_as(ce->content.data, double));
            break;
        case CTC_LONG_DOUBLE:
            printer("%Lf", data_as(ce->content.data, long double));
            break;
        case CTC_POINTER:
            printer("%p", data_as(ce->content.data, void*));
            break;
        case CTC_VOID:
            printer("void");
            break;
        case CTC_FLOAT_COMPLEX:
        case CTC_DOUBLE_COMPLEX:
        case CTC_LONG_DOUBLE_COMPLEX:
        case CTC_FLOAT_IMAGINARY:
        case CTC_DOUBLE_IMAGINARY:
        case CTC_LONG_DOUBLE_IMAGINARY:
        case CTC_ENUMERATED:
        case CTC_ARRAY:
        case CTC_STRUCTURE:
        case CTC_UNION:
        case CTC_FUNCTION:
        case CTC_LABEL:
            printer("(unprintable value)");
            break;
        case CTC_ERROR:
            printer("(error)");
            break;
    }
}

static void copy_typed_data(constexpr_t* ce, c_type_t* ct, uint8_t* data)
{
    if (ce->ct) type_delete(ce->ct);
    ce->ct = ct;
    int64_t size = type_size(ct);
    if (ce->content.data)
        ce->content.data = realloc(ce->content.data, size);
    else
        ce->content.data = malloc(size);
    memcpy(ce->content.data, data, size);
}

static bool constexpr_move(constexpr_t* dest, constexpr_t* src)
{
    if (src->type != dest->type)
        return false;
    if (src->type == CE_ADDRESS)
    {
        dest->ct = type_copy(src->ct);
        dest->content.addr.sy = src->content.addr.sy;
        dest->content.addr.negative_offset = src->content.addr.negative_offset;
        dest->content.addr.offset = src->content.addr.offset;
        return true;
    }
    copy_typed_data(dest, type_copy(src->ct), src->content.data);
    return true;
}

#define convert(t1, t2, c1, c2) \
    if (from->class == (c1) && to->class == (c2)) \
    { \
        t2 value = (t2) data_as(ce->content.data, t1); \
        copy_typed_data(ce, type_copy(to), to_data(value)); \
        return; \
    }

// a beautiful function
void constexpr_convert(constexpr_t* ce, c_type_t* to)
{
    c_type_t* from = ce->ct;
    if (type_is_compatible(from, to))
        return;

    /* signed integer -> larger integer */
    
    // char -> larger integer
    convert(char, short, CTC_CHAR, CTC_SHORT_INT);
    convert(char, int, CTC_CHAR, CTC_INT);
    convert(char, long, CTC_CHAR, CTC_LONG_INT);
    convert(char, long long, CTC_CHAR, CTC_LONG_LONG_INT);
    convert(char, unsigned short, CTC_CHAR, CTC_UNSIGNED_SHORT_INT);
    convert(char, unsigned int, CTC_CHAR, CTC_UNSIGNED_INT);
    convert(char, unsigned long, CTC_CHAR, CTC_UNSIGNED_LONG_INT);
    convert(char, unsigned long long, CTC_CHAR, CTC_UNSIGNED_LONG_LONG_INT);

    // signed char -> larger integer
    convert(signed char, short, CTC_SIGNED_CHAR, CTC_SHORT_INT);
    convert(signed char, int, CTC_SIGNED_CHAR, CTC_INT);
    convert(signed char, long, CTC_SIGNED_CHAR, CTC_LONG_INT);
    convert(signed char, long long, CTC_SIGNED_CHAR, CTC_LONG_LONG_INT);
    convert(signed char, unsigned short, CTC_SIGNED_CHAR, CTC_UNSIGNED_SHORT_INT);
    convert(signed char, unsigned int, CTC_SIGNED_CHAR, CTC_UNSIGNED_INT);
    convert(signed char, unsigned long, CTC_SIGNED_CHAR, CTC_UNSIGNED_LONG_INT);
    convert(signed char, unsigned long long, CTC_SIGNED_CHAR, CTC_UNSIGNED_LONG_LONG_INT);

    // short -> larger integer
    convert(short, int, CTC_SHORT_INT, CTC_INT);
    convert(short, long, CTC_SHORT_INT, CTC_LONG_INT);
    convert(short, long long, CTC_SHORT_INT, CTC_LONG_LONG_INT);
    convert(short, unsigned int, CTC_SHORT_INT, CTC_UNSIGNED_INT);
    convert(short, unsigned long, CTC_SHORT_INT, CTC_UNSIGNED_LONG_INT);
    convert(short, unsigned long long, CTC_SHORT_INT, CTC_UNSIGNED_LONG_LONG_INT);

    // int -> larger integer
    convert(int, long, CTC_INT, CTC_LONG_INT);
    convert(int, long long, CTC_INT, CTC_LONG_LONG_INT);
    convert(int, unsigned long, CTC_INT, CTC_UNSIGNED_LONG_INT);
    convert(int, unsigned long long, CTC_INT, CTC_UNSIGNED_LONG_LONG_INT);

    /* signed integer -> smaller integer or same */

    // long long -> smaller integer or same
    convert(long long, long, CTC_LONG_LONG_INT, CTC_LONG_INT);
    convert(long long, int, CTC_LONG_LONG_INT, CTC_INT);
    convert(long long, short, CTC_LONG_LONG_INT, CTC_SHORT_INT);
    convert(long long, signed char, CTC_LONG_LONG_INT, CTC_SIGNED_CHAR);
    convert(long long, char, CTC_LONG_LONG_INT, CTC_CHAR);
    convert(long long, unsigned long long, CTC_LONG_LONG_INT, CTC_UNSIGNED_LONG_LONG_INT);
    convert(long long, unsigned long, CTC_LONG_LONG_INT, CTC_UNSIGNED_LONG_INT);
    convert(long long, unsigned int, CTC_LONG_LONG_INT, CTC_UNSIGNED_INT);
    convert(long long, unsigned short, CTC_LONG_LONG_INT, CTC_UNSIGNED_SHORT_INT);
    convert(long long, unsigned char, CTC_LONG_LONG_INT, CTC_UNSIGNED_CHAR);

    // long -> smaller integer or same
    convert(long, int, CTC_LONG_INT, CTC_INT);
    convert(long, short, CTC_LONG_INT, CTC_SHORT_INT);
    convert(long, signed char, CTC_LONG_INT, CTC_SIGNED_CHAR);
    convert(long, char, CTC_LONG_INT, CTC_CHAR);
    convert(long, unsigned long, CTC_LONG_INT, CTC_UNSIGNED_LONG_INT);
    convert(long, unsigned int, CTC_LONG_INT, CTC_UNSIGNED_INT);
    convert(long, unsigned short, CTC_LONG_INT, CTC_UNSIGNED_SHORT_INT);
    convert(long, unsigned char, CTC_LONG_INT, CTC_UNSIGNED_CHAR);

    // int -> smaller integer or same
    convert(int, short, CTC_INT, CTC_SHORT_INT);
    convert(int, signed char, CTC_INT, CTC_SIGNED_CHAR);
    convert(int, char, CTC_INT, CTC_CHAR);
    convert(int, unsigned int, CTC_INT, CTC_UNSIGNED_INT);
    convert(int, unsigned short, CTC_INT, CTC_UNSIGNED_SHORT_INT);
    convert(int, unsigned char, CTC_INT, CTC_UNSIGNED_CHAR);

    // short -> smaller integer or same
    convert(short, signed char, CTC_SHORT_INT, CTC_SIGNED_CHAR);
    convert(short, char, CTC_SHORT_INT, CTC_CHAR);
    convert(short, unsigned short, CTC_SHORT_INT, CTC_UNSIGNED_SHORT_INT);
    convert(short, unsigned char, CTC_SHORT_INT, CTC_UNSIGNED_CHAR);

    // signed char -> smaller integer or same
    convert(signed char, char, CTC_SIGNED_CHAR, CTC_CHAR);
    convert(signed char, unsigned char, CTC_SIGNED_CHAR, CTC_UNSIGNED_CHAR);

    // char -> smaller integer or same
    convert(char, signed char, CTC_CHAR, CTC_SIGNED_CHAR);
    convert(char, unsigned char, CTC_CHAR, CTC_UNSIGNED_CHAR);

    /* unsigned integer -> larger integer */
    
    // unsigned char -> larger integer
    convert(unsigned char, short, CTC_UNSIGNED_CHAR, CTC_SHORT_INT);
    convert(unsigned char, int, CTC_UNSIGNED_CHAR, CTC_INT);
    convert(unsigned char, long, CTC_UNSIGNED_CHAR, CTC_LONG_INT);
    convert(unsigned char, long long, CTC_UNSIGNED_CHAR, CTC_LONG_LONG_INT);
    convert(unsigned char, unsigned short, CTC_UNSIGNED_CHAR, CTC_UNSIGNED_SHORT_INT);
    convert(unsigned char, unsigned int, CTC_UNSIGNED_CHAR, CTC_UNSIGNED_INT);
    convert(unsigned char, unsigned long, CTC_UNSIGNED_CHAR, CTC_UNSIGNED_LONG_INT);
    convert(unsigned char, unsigned long long, CTC_UNSIGNED_CHAR, CTC_UNSIGNED_LONG_LONG_INT);

    // unsigned short -> larger integer
    convert(unsigned short, int, CTC_UNSIGNED_SHORT_INT, CTC_INT);
    convert(unsigned short, long, CTC_UNSIGNED_SHORT_INT, CTC_LONG_INT);
    convert(unsigned short, long long, CTC_UNSIGNED_SHORT_INT, CTC_LONG_LONG_INT);
    convert(unsigned short, unsigned int, CTC_UNSIGNED_SHORT_INT, CTC_UNSIGNED_INT);
    convert(unsigned short, unsigned long, CTC_UNSIGNED_SHORT_INT, CTC_UNSIGNED_LONG_INT);
    convert(unsigned short, unsigned long long, CTC_UNSIGNED_SHORT_INT, CTC_UNSIGNED_LONG_LONG_INT);

    // unsigned int -> larger integer
    convert(unsigned int, long, CTC_UNSIGNED_INT, CTC_LONG_INT);
    convert(unsigned int, long long, CTC_UNSIGNED_INT, CTC_LONG_LONG_INT);
    convert(unsigned int, unsigned long, CTC_UNSIGNED_INT, CTC_UNSIGNED_LONG_INT);
    convert(unsigned int, unsigned long long, CTC_UNSIGNED_INT, CTC_UNSIGNED_LONG_LONG_INT);

    /* unsigned signed integer -> smaller integer or same */

    // unsigned long long -> smaller integer or same
    convert(unsigned long long, long long, CTC_UNSIGNED_LONG_LONG_INT, CTC_LONG_LONG_INT);
    convert(unsigned long long, long, CTC_UNSIGNED_LONG_LONG_INT, CTC_LONG_INT);
    convert(unsigned long long, int, CTC_UNSIGNED_LONG_LONG_INT, CTC_INT);
    convert(unsigned long long, short, CTC_UNSIGNED_LONG_LONG_INT, CTC_SHORT_INT);
    convert(unsigned long long, signed char, CTC_UNSIGNED_LONG_LONG_INT, CTC_SIGNED_CHAR);
    convert(unsigned long long, char, CTC_UNSIGNED_LONG_LONG_INT, CTC_CHAR);
    convert(unsigned long long, unsigned long, CTC_UNSIGNED_LONG_LONG_INT, CTC_UNSIGNED_LONG_INT);
    convert(unsigned long long, unsigned int, CTC_UNSIGNED_LONG_LONG_INT, CTC_UNSIGNED_INT);
    convert(unsigned long long, unsigned short, CTC_UNSIGNED_LONG_LONG_INT, CTC_UNSIGNED_SHORT_INT);
    convert(unsigned long long, unsigned char, CTC_UNSIGNED_LONG_LONG_INT, CTC_UNSIGNED_CHAR);

    // unsigned long -> smaller integer or same
    convert(unsigned long, long, CTC_UNSIGNED_LONG_INT, CTC_LONG_INT);
    convert(unsigned long, int, CTC_UNSIGNED_LONG_INT, CTC_INT);
    convert(unsigned long, short, CTC_UNSIGNED_LONG_INT, CTC_SHORT_INT);
    convert(unsigned long, signed char, CTC_UNSIGNED_LONG_INT, CTC_SIGNED_CHAR);
    convert(unsigned long, char, CTC_UNSIGNED_LONG_INT, CTC_CHAR);
    convert(unsigned long, unsigned int, CTC_UNSIGNED_LONG_INT, CTC_UNSIGNED_INT);
    convert(unsigned long, unsigned short, CTC_UNSIGNED_LONG_INT, CTC_UNSIGNED_SHORT_INT);
    convert(unsigned long, unsigned char, CTC_UNSIGNED_LONG_INT, CTC_UNSIGNED_CHAR);

    // unsigned int -> smaller integer or same
    convert(unsigned int, int, CTC_UNSIGNED_INT, CTC_INT);
    convert(unsigned int, short, CTC_UNSIGNED_INT, CTC_SHORT_INT);
    convert(unsigned int, signed char, CTC_UNSIGNED_INT, CTC_SIGNED_CHAR);
    convert(unsigned int, char, CTC_UNSIGNED_INT, CTC_CHAR);
    convert(unsigned int, unsigned short, CTC_UNSIGNED_INT, CTC_UNSIGNED_SHORT_INT);
    convert(unsigned int, unsigned char, CTC_UNSIGNED_INT, CTC_UNSIGNED_CHAR);

    // unsigned short -> smaller integer or same
    convert(unsigned short, short, CTC_UNSIGNED_SHORT_INT, CTC_SHORT_INT);
    convert(unsigned short, signed char, CTC_UNSIGNED_SHORT_INT, CTC_SIGNED_CHAR);
    convert(unsigned short, char, CTC_UNSIGNED_SHORT_INT, CTC_CHAR);
    convert(unsigned short, unsigned char, CTC_UNSIGNED_SHORT_INT, CTC_UNSIGNED_CHAR);

    // unsigned char -> smaller integer or same
    convert(unsigned char, signed char, CTC_UNSIGNED_CHAR, CTC_SIGNED_CHAR);
    convert(unsigned char, char, CTC_UNSIGNED_CHAR, CTC_CHAR);

    // float -> other floating type
    convert(float, double, CTC_FLOAT, CTC_DOUBLE);
    convert(float, long double, CTC_FLOAT, CTC_LONG_DOUBLE);

    // double -> other floating type
    convert(double, float, CTC_DOUBLE, CTC_FLOAT);
    convert(double, long double, CTC_DOUBLE, CTC_LONG_DOUBLE);

    // long double -> other floating type
    convert(long double, float, CTC_LONG_DOUBLE, CTC_FLOAT);
    convert(long double, double, CTC_LONG_DOUBLE, CTC_DOUBLE);

    /* float -> signed integer type */

    convert(float, char, CTC_FLOAT, CTC_CHAR);
    convert(float, signed char, CTC_FLOAT, CTC_SIGNED_CHAR);
    convert(float, short, CTC_FLOAT, CTC_SHORT_INT);
    convert(float, int, CTC_FLOAT, CTC_INT);
    convert(float, long, CTC_FLOAT, CTC_LONG_INT);
    convert(float, long long, CTC_FLOAT, CTC_LONG_LONG_INT);

    /* float -> unsigned integer type */

    convert(float, unsigned char, CTC_FLOAT, CTC_UNSIGNED_CHAR);
    convert(float, unsigned short, CTC_FLOAT, CTC_UNSIGNED_SHORT_INT);
    convert(float, unsigned int, CTC_FLOAT, CTC_UNSIGNED_INT);
    convert(float, unsigned long, CTC_FLOAT, CTC_UNSIGNED_LONG_INT);
    convert(float, unsigned long long, CTC_FLOAT, CTC_UNSIGNED_LONG_LONG_INT);

    /* double -> signed integer type */

    convert(double, char, CTC_DOUBLE, CTC_CHAR);
    convert(double, signed char, CTC_DOUBLE, CTC_SIGNED_CHAR);
    convert(double, short, CTC_DOUBLE, CTC_SHORT_INT);
    convert(double, int, CTC_DOUBLE, CTC_INT);
    convert(double, long, CTC_DOUBLE, CTC_LONG_INT);
    convert(double, long long, CTC_DOUBLE, CTC_LONG_LONG_INT);

    /* double -> unsigned integer type */

    convert(double, unsigned char, CTC_DOUBLE, CTC_UNSIGNED_CHAR);
    convert(double, unsigned short, CTC_DOUBLE, CTC_UNSIGNED_SHORT_INT);
    convert(double, unsigned int, CTC_DOUBLE, CTC_UNSIGNED_INT);
    convert(double, unsigned long, CTC_DOUBLE, CTC_UNSIGNED_LONG_INT);
    convert(double, unsigned long long, CTC_DOUBLE, CTC_UNSIGNED_LONG_LONG_INT);

    /* long double -> signed integer type */

    convert(long double, char, CTC_LONG_DOUBLE, CTC_CHAR);
    convert(long double, signed char, CTC_LONG_DOUBLE, CTC_SIGNED_CHAR);
    convert(long double, short, CTC_LONG_DOUBLE, CTC_SHORT_INT);
    convert(long double, int, CTC_LONG_DOUBLE, CTC_INT);
    convert(long double, long, CTC_LONG_DOUBLE, CTC_LONG_INT);
    convert(long double, long long, CTC_LONG_DOUBLE, CTC_LONG_LONG_INT);

    /* long double -> unsigned integer type */

    convert(long double, unsigned char, CTC_LONG_DOUBLE, CTC_UNSIGNED_CHAR);
    convert(long double, unsigned short, CTC_LONG_DOUBLE, CTC_UNSIGNED_SHORT_INT);
    convert(long double, unsigned int, CTC_LONG_DOUBLE, CTC_UNSIGNED_INT);
    convert(long double, unsigned long, CTC_LONG_DOUBLE, CTC_UNSIGNED_LONG_INT);
    convert(long double, unsigned long long, CTC_LONG_DOUBLE, CTC_UNSIGNED_LONG_LONG_INT);

    /* signed integer type -> float */

    convert(char, float, CTC_CHAR, CTC_FLOAT);
    convert(signed char, float, CTC_SIGNED_CHAR, CTC_FLOAT);
    convert(short, float, CTC_SHORT_INT, CTC_FLOAT);
    convert(int, float, CTC_INT, CTC_FLOAT);
    convert(long, float, CTC_LONG_INT, CTC_FLOAT);
    convert(long long, float, CTC_LONG_LONG_INT, CTC_FLOAT);

    /* unsigned integer type -> float */

    convert(unsigned char, float, CTC_UNSIGNED_CHAR, CTC_FLOAT);
    convert(unsigned short, float, CTC_UNSIGNED_SHORT_INT, CTC_FLOAT);
    convert(unsigned int, float, CTC_UNSIGNED_INT, CTC_FLOAT);
    convert(unsigned long, float, CTC_UNSIGNED_LONG_INT, CTC_FLOAT);
    convert(unsigned long long, float, CTC_UNSIGNED_LONG_LONG_INT, CTC_FLOAT);

    /* signed integer type -> double */

    convert(char, double, CTC_CHAR, CTC_DOUBLE);
    convert(signed char, double, CTC_SIGNED_CHAR, CTC_DOUBLE);
    convert(short, double, CTC_SHORT_INT, CTC_DOUBLE);
    convert(int, double, CTC_INT, CTC_DOUBLE);
    convert(long, double, CTC_LONG_INT, CTC_DOUBLE);
    convert(long long, double, CTC_LONG_LONG_INT, CTC_DOUBLE);

    /* unsigned integer type -> double */

    convert(unsigned char, double, CTC_UNSIGNED_CHAR, CTC_DOUBLE);
    convert(unsigned short, double, CTC_UNSIGNED_SHORT_INT, CTC_DOUBLE);
    convert(unsigned int, double, CTC_UNSIGNED_INT, CTC_DOUBLE);
    convert(unsigned long, double, CTC_UNSIGNED_LONG_INT, CTC_DOUBLE);
    convert(unsigned long long, double, CTC_UNSIGNED_LONG_LONG_INT, CTC_DOUBLE);

    /* signed integer type -> long double */

    convert(char, long double, CTC_CHAR, CTC_LONG_DOUBLE);
    convert(signed char, long double, CTC_SIGNED_CHAR, CTC_LONG_DOUBLE);
    convert(short, long double, CTC_SHORT_INT, CTC_LONG_DOUBLE);
    convert(int, long double, CTC_INT, CTC_LONG_DOUBLE);
    convert(long, long double, CTC_LONG_INT, CTC_LONG_DOUBLE);
    convert(long long, long double, CTC_LONG_LONG_INT, CTC_LONG_DOUBLE);

    /* unsigned integer type -> long double */

    convert(unsigned char, long double, CTC_UNSIGNED_CHAR, CTC_LONG_DOUBLE);
    convert(unsigned short, long double, CTC_UNSIGNED_SHORT_INT, CTC_LONG_DOUBLE);
    convert(unsigned int, long double, CTC_UNSIGNED_INT, CTC_LONG_DOUBLE);
    convert(unsigned long, long double, CTC_UNSIGNED_LONG_INT, CTC_LONG_DOUBLE);
    convert(unsigned long long, long double, CTC_UNSIGNED_LONG_LONG_INT, CTC_LONG_DOUBLE);

    // unsupported conversion
    if (get_program_options()->iflag)
    {
        printf("unsupported conversion attempted:\n");
        printf("    from: ");
        type_humanized_print(from, printf);
        printf(", to: ");
        type_humanized_print(to, printf);
        printf("\n");
    }
}

#undef convert

void constexpr_convert_class(constexpr_t* ce, c_type_class_t class)
{
    c_type_t* ct = make_basic_type(class);
    constexpr_convert(ce, ct);
    type_delete(ct);
}

static constexpr_t* constexpr_evaluate_type(syntax_component_t* expr, constexpr_type_t type);
static void evaluate(syntax_component_t* expr, constexpr_t* ce);

#define unop_case(rt, t, c, op) \
    case c: \
    { \
        t value = op data_as(operand->content.data, t); \
        copy_typed_data(ce, type_copy(rt), to_data(value)); \
        break; \
    }

#define arithmetic_unop_switch(rt, op) \
    switch ((rt)->class) \
    { \
        unop_case((rt), char, CTC_CHAR, op) \
        unop_case((rt), signed char, CTC_SIGNED_CHAR, op) \
        unop_case((rt), short, CTC_SHORT_INT, op) \
        unop_case((rt), int, CTC_INT, op) \
        unop_case((rt), long, CTC_LONG_INT, op) \
        unop_case((rt), long long, CTC_LONG_LONG_INT, op) \
        unop_case((rt), unsigned char, CTC_UNSIGNED_CHAR, op) \
        unop_case((rt), unsigned short, CTC_UNSIGNED_SHORT_INT, op) \
        unop_case((rt), unsigned, CTC_UNSIGNED_INT, op) \
        unop_case((rt), unsigned long, CTC_UNSIGNED_LONG_INT, op) \
        unop_case((rt), unsigned long long, CTC_UNSIGNED_LONG_LONG_INT, op) \
        unop_case((rt), float, CTC_FLOAT, op) \
        unop_case((rt), double, CTC_DOUBLE, op) \
        unop_case((rt), long double, CTC_LONG_DOUBLE, op) \
        default: report_return; \
    }

#define integer_unop_switch(rt, op) \
    switch ((rt)->class) \
    { \
        unop_case((rt), char, CTC_CHAR, op) \
        unop_case((rt), signed char, CTC_SIGNED_CHAR, op) \
        unop_case((rt), short, CTC_SHORT_INT, op) \
        unop_case((rt), int, CTC_INT, op) \
        unop_case((rt), long, CTC_LONG_INT, op) \
        unop_case((rt), long long, CTC_LONG_LONG_INT, op) \
        unop_case((rt), unsigned char, CTC_UNSIGNED_CHAR, op) \
        unop_case((rt), unsigned short, CTC_UNSIGNED_SHORT_INT, op) \
        unop_case((rt), unsigned, CTC_UNSIGNED_INT, op) \
        unop_case((rt), unsigned long, CTC_UNSIGNED_LONG_INT, op) \
        unop_case((rt), unsigned long long, CTC_UNSIGNED_LONG_LONG_INT, op) \
        default: report_return; \
    }

#define binop_case(rt, t, c, op) \
    case c: \
    { \
        t value = data_as(lhs->content.data, t) op data_as(rhs->content.data, t); \
        copy_typed_data(ce, type_copy(rt), to_data(value)); \
        break; \
    }

#define arithmetic_binop_switch(rt, op) \
    switch ((rt)->class) \
    { \
        binop_case((rt), char, CTC_CHAR, op) \
        binop_case((rt), signed char, CTC_SIGNED_CHAR, op) \
        binop_case((rt), short, CTC_SHORT_INT, op) \
        binop_case((rt), int, CTC_INT, op) \
        binop_case((rt), long, CTC_LONG_INT, op) \
        binop_case((rt), long long, CTC_LONG_LONG_INT, op) \
        binop_case((rt), unsigned char, CTC_UNSIGNED_CHAR, op) \
        binop_case((rt), unsigned short, CTC_UNSIGNED_SHORT_INT, op) \
        binop_case((rt), unsigned, CTC_UNSIGNED_INT, op) \
        binop_case((rt), unsigned long, CTC_UNSIGNED_LONG_INT, op) \
        binop_case((rt), unsigned long long, CTC_UNSIGNED_LONG_LONG_INT, op) \
        binop_case((rt), float, CTC_FLOAT, op) \
        binop_case((rt), double, CTC_DOUBLE, op) \
        binop_case((rt), long double, CTC_LONG_DOUBLE, op) \
        default: break; \
    }

#define integer_binop_switch(rt, op) \
    switch ((rt)->class) \
    { \
        binop_case((rt), char, CTC_CHAR, op) \
        binop_case((rt), signed char, CTC_SIGNED_CHAR, op) \
        binop_case((rt), short, CTC_SHORT_INT, op) \
        binop_case((rt), int, CTC_INT, op) \
        binop_case((rt), long, CTC_LONG_INT, op) \
        binop_case((rt), long long, CTC_LONG_LONG_INT, op) \
        binop_case((rt), unsigned char, CTC_UNSIGNED_CHAR, op) \
        binop_case((rt), unsigned short, CTC_UNSIGNED_SHORT_INT, op) \
        binop_case((rt), unsigned, CTC_UNSIGNED_INT, op) \
        binop_case((rt), unsigned long, CTC_UNSIGNED_LONG_INT, op) \
        binop_case((rt), unsigned long long, CTC_UNSIGNED_LONG_LONG_INT, op) \
        default: report_return; \
    }

#define generate_constexpr_as_function(name, ty) \
    ty constexpr_as_##name(constexpr_t* ce) \
    { \
        if (ce->type == CE_ADDRESS) \
            return 0; \
        return data_as(ce->content.data, ty); \
    }

generate_constexpr_as_function(i64, int64_t)
generate_constexpr_as_function(u64, uint64_t)
generate_constexpr_as_function(i32, int32_t)

bool constexpr_equals_zero(constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
        // TODO
        report_return_value(false);
    
    if (type_is_floating(ce->ct))
    {
        switch (ce->ct->class)
        {
            case CTC_FLOAT: return data_as(ce->content.data, float) == 0.0f;
            case CTC_DOUBLE: return data_as(ce->content.data, double) == 0.0;
            case CTC_LONG_DOUBLE: return data_as(ce->content.data, long double) == 0.0L;
            default: report_return_value(false);
        }
    }
    long long size = type_size(ce->ct);
    for (long long i = 0; i < size; ++i)
        if (ce->content.data[i])
            return false;
    return true;
}

bool constexpr_equals(constexpr_t* ce1, constexpr_t* ce2)
{
    c_type_t* rt = usual_arithmetic_conversions_result_type(ce1->ct, ce2->ct);

    constexpr_t* lhs = constexpr_copy(ce1);
    constexpr_t* rhs = constexpr_copy(ce2);
    constexpr_t* ce = calloc(1, sizeof *ce);
    ce->type = ce1->type;

    constexpr_convert(lhs, rt);
    constexpr_convert(rhs, rt);

    arithmetic_binop_switch(rt, ==)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
    type_delete(rt);
    bool zero = constexpr_equals_zero(ce);
    constexpr_delete(ce);
    return !zero;
}

void evaluate_plus_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* operand = constexpr_evaluate_type(expr->uexpr_operand, ce->type);
    if (!constexpr_evaluation_succeeded(operand))
    {
        ce->error = operand->error;
        operand->error = NULL;
        constexpr_delete(operand);
        return;
    }

    constexpr_convert(operand, expr->ctype);

    arithmetic_unop_switch(expr->ctype, +);

    constexpr_delete(operand);
}

void evaluate_minus_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* operand = constexpr_evaluate_type(expr->uexpr_operand, ce->type);
    if (!constexpr_evaluation_succeeded(operand))
    {
        ce->error = operand->error;
        operand->error = NULL;
        constexpr_delete(operand);
        return;
    }

    constexpr_convert(operand, expr->ctype);

    arithmetic_unop_switch(expr->ctype, -);

    constexpr_delete(operand);
}

void evaluate_addition_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    if (type_is_arithmetic(expr->bexpr_lhs->ctype) && type_is_arithmetic(expr->bexpr_rhs->ctype))
    {
        constexpr_convert(lhs, expr->ctype);
        constexpr_convert(rhs, expr->ctype);

        arithmetic_binop_switch(expr->ctype, +)
    }
    else
        // TODO: handle ptr arithmetic
        report_return;

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_subtraction_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    if (type_is_arithmetic(expr->bexpr_lhs->ctype) && type_is_arithmetic(expr->bexpr_rhs->ctype))
    {
        constexpr_convert(lhs, expr->ctype);
        constexpr_convert(rhs, expr->ctype);

        arithmetic_binop_switch(expr->ctype, -)
    }
    else
        // TODO: handle ptr arithmetic
        report_return;

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_multiplication_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, *)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_division_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, /)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_modular_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    integer_binop_switch(expr->ctype, %)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_complement_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* operand = constexpr_evaluate_type(expr->uexpr_operand, ce->type);
    if (!constexpr_evaluation_succeeded(operand))
    {
        ce->error = operand->error;
        operand->error = NULL;
        constexpr_delete(operand);
        return;
    }

    constexpr_convert(operand, expr->ctype);

    integer_unop_switch(expr->ctype, ~);

    constexpr_delete(operand);
}

void evaluate_bitwise_and_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    integer_binop_switch(expr->ctype, &)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_bitwise_or_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    integer_binop_switch(expr->ctype, |)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_bitwise_xor_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    integer_binop_switch(expr->ctype, ^)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_bitwise_left_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    integer_binop_switch(expr->ctype, <<)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_bitwise_right_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    integer_binop_switch(expr->ctype, >>)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_not_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* operand = constexpr_evaluate_type(expr->uexpr_operand, ce->type);
    if (!constexpr_evaluation_succeeded(operand))
    {
        ce->error = operand->error;
        operand->error = NULL;
        constexpr_delete(operand);
        return;
    }

    constexpr_convert(operand, expr->ctype);

    arithmetic_unop_switch(expr->ctype, !);

    constexpr_delete(operand);
}

void evaluate_logical_and_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    bool zero = constexpr_equals_zero(lhs);
    constexpr_delete(lhs);

    if (zero)
    {
        int value = 0;
        copy_typed_data(ce, make_basic_type(CTC_INT), to_data(value));
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(rhs);
        return;
    }

    zero = constexpr_equals_zero(rhs);
    constexpr_delete(rhs);

    int value = zero ? 0 : 1;
    copy_typed_data(ce, make_basic_type(CTC_INT), to_data(value));
}

void evaluate_logical_or_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    bool zero = constexpr_equals_zero(lhs);
    constexpr_delete(lhs);

    if (!zero)
    {
        int value = 1;
        copy_typed_data(ce, make_basic_type(CTC_INT), to_data(value));
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(rhs);
        return;
    }

    zero = constexpr_equals_zero(rhs);
    constexpr_delete(rhs);

    int value = zero ? 0 : 1;
    copy_typed_data(ce, make_basic_type(CTC_INT), to_data(value));
}

void evaluate_equality_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, ==)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_inequality_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, !=)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_less_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, <)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_less_equal_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, <=)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_greater_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, >)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_greater_equal_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* lhs = constexpr_evaluate_type(expr->bexpr_lhs, ce->type);
    if (!constexpr_evaluation_succeeded(lhs))
    {
        ce->error = lhs->error;
        lhs->error = NULL;
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->bexpr_rhs, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(lhs);
        constexpr_delete(rhs);
        return;
    }

    constexpr_convert(lhs, expr->ctype);
    constexpr_convert(rhs, expr->ctype);

    arithmetic_binop_switch(expr->ctype, >=)

    constexpr_delete(lhs);
    constexpr_delete(rhs);
}

void evaluate_subscript_expression(syntax_component_t* expr, constexpr_t* ce)
{
    syntax_component_t* sarray = expr->subsexpr_expression;
    syntax_component_t* sindex = expr->subsexpr_index_expression;
    if (type_is_integer(sarray->ctype))
    {
        syntax_component_t* tmp = sarray;
        sarray = sindex;
        sindex = tmp;
    }
    if (ce->type == CE_ADDRESS)
    {
        constexpr_t* cearray = constexpr_evaluate_address(sarray);
        if (!constexpr_evaluation_succeeded(cearray))
        {
            ce->error = cearray->error;
            cearray->error = NULL;
            constexpr_delete(cearray);
            return;
        }

        constexpr_move(ce, cearray);
        constexpr_delete(cearray);

        constexpr_t* ceindex = constexpr_evaluate_integer(sindex);
        if (!constexpr_evaluation_succeeded(ceindex))
        {
            ce->error = ceindex->error;
            ceindex->error = NULL;
            constexpr_delete(ceindex);
            return;
        }
        constexpr_convert_class(ceindex, CTC_UNSIGNED_LONG_LONG_INT);
        uint64_t index = constexpr_as_u64(ceindex);
        ce->content.addr.offset += index * type_size(sarray->ctype->derived_from);
        return;
    }
}

void evaluate_dereference_expression(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        constexpr_t* operand = constexpr_evaluate_address(expr->uexpr_operand);
        if (!constexpr_evaluation_succeeded(operand))
        {
            ce->error = operand->error;
            operand->error = NULL;
            constexpr_delete(operand);
            return;
        }

        constexpr_move(ce, operand);
        constexpr_delete(operand);
        return;
    }
}

void evaluate_reference_expression(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        constexpr_t* operand = constexpr_evaluate_address(expr->uexpr_operand);
        if (!constexpr_evaluation_succeeded(operand))
        {
            ce->error = operand->error;
            operand->error = NULL;
            constexpr_delete(operand);
            return;
        }

        constexpr_move(ce, operand);
        constexpr_delete(operand);
        return;
    }
}

void evaluate_dereference_member_expression(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        constexpr_t* lhs = constexpr_evaluate_address(expr->memexpr_expression);
        if (!constexpr_evaluation_succeeded(lhs))
        {
            ce->error = lhs->error;
            lhs->error = NULL;
            constexpr_delete(lhs);
            return;
        }

        constexpr_move(ce, lhs);
        constexpr_delete(lhs);

        c_type_t* st = expr->memexpr_expression->ctype->derived_from;

        int64_t offset = 0;
        long long index = -1;
        type_get_struct_union_member_info(st, expr->memexpr_id->id, &index, &offset);
        if (index == -1)
            report_return;
        
        ce->content.addr.offset += offset;
        return;
    }
}

void evaluate_member_expression(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        constexpr_t* lhs = constexpr_evaluate_address(expr->memexpr_expression);
        if (!constexpr_evaluation_succeeded(lhs))
        {
            ce->error = lhs->error;
            lhs->error = NULL;
            constexpr_delete(lhs);
            return;
        }

        constexpr_move(ce, lhs);
        constexpr_delete(lhs);

        c_type_t* st = expr->memexpr_expression->ctype;

        int64_t offset = 0;
        long long index = -1;
        type_get_struct_union_member_info(st, expr->memexpr_id->id, &index, &offset);
        if (index == -1)
            report_return;
        
        ce->content.addr.offset += offset;
        return;
    }
}

void evaluate_cast_expression(syntax_component_t* expr, constexpr_t* ce)
{
    c_type_t* to = create_type(expr->caexpr_type_name, expr->caexpr_type_name->tn_declarator);
    if (ce->type == CE_INTEGER && (!type_is_arithmetic(expr->caexpr_operand->ctype) || !type_is_integer(to)))
    {
        // ISO: 6.6 (6)
        SET_ERROR(expr, "casts in an integer constant expression may only convert arithmetic types to integer types");
        type_delete(to);
        return;
    }
    if (ce->type == CE_ARITHMETIC && (!type_is_arithmetic(expr->caexpr_operand->ctype) || !type_is_arithmetic(to)))
    {
        // ISO: 6.6 (8)
        SET_ERROR(expr, "casts in an arithmetic constant expression may only convert arithmetic types to other arithmetic types");
        type_delete(to);
        return;
    }
    constexpr_t* op = constexpr_evaluate_type(expr->caexpr_operand, ce->type);
    constexpr_convert(op, to);
    if (!constexpr_move(ce, op))
        report_return;
    constexpr_delete(op);
    type_delete(to);
}

void evaluate_conditional_expression(syntax_component_t* expr, constexpr_t* ce)
{
    constexpr_t* condition = constexpr_evaluate_type(expr->cexpr_condition, ce->type);
    if (!constexpr_evaluation_succeeded(condition))
    {
        ce->error = condition->error;
        condition->error = NULL;
        constexpr_delete(condition);
        return;
    }

    bool zero = constexpr_equals_zero(condition);
    constexpr_delete(condition);

    if (!zero)
    {
        constexpr_t* lhs = constexpr_evaluate_type(expr->cexpr_if, ce->type);
        if (!constexpr_evaluation_succeeded(lhs))
        {
            ce->error = lhs->error;
            lhs->error = NULL;
            constexpr_delete(lhs);
            return;
        }

        constexpr_move(ce, lhs);
        constexpr_delete(lhs);
        return;
    }

    constexpr_t* rhs = constexpr_evaluate_type(expr->cexpr_else, ce->type);
    if (!constexpr_evaluation_succeeded(rhs))
    {
        ce->error = rhs->error;
        rhs->error = NULL;
        constexpr_delete(rhs);
        return;
    }

    constexpr_move(ce, rhs);
    constexpr_delete(rhs);
}

void evaluate_sizeof_expression(syntax_component_t* expr, constexpr_t* ce)
{
    long long size = type_size(expr->uexpr_operand->ctype);
    if (size == -1)
    {
        // ISO: 6.6 (6), 6.6 (8)
        SET_ERROR(expr, "the 'sizeof' operator may only be applied to non-VLA type expressions in a constant expression");
        return;
    }
    copy_typed_data(ce, type_copy(expr->ctype), to_data(size));
}

void evaluate_sizeof_type_expression(syntax_component_t* expr, constexpr_t* ce)
{
    c_type_t* ct = create_type(expr->uexpr_operand, expr->uexpr_operand->tn_declarator);
    long long size = type_size(ct);
    type_delete(ct);
    if (size == -1)
    {
        // ISO: 6.6 (6), 6.6 (8)
        SET_ERROR(expr, "the 'sizeof' operator may only be applied to non-VLA type expressions in a constant expression");
        return;
    }
    copy_typed_data(ce, type_copy(expr->ctype), to_data(size));
}

void evaluate_integer_constant(syntax_component_t* expr, constexpr_t* ce)
{
    switch (expr->ctype->class)
    {
        case CTC_INT: copy_typed_data(ce, make_basic_type(CTC_INT), to_data(expr->intc)); break;
        case CTC_LONG_INT: copy_typed_data(ce, make_basic_type(CTC_LONG_INT), to_data(expr->intc)); break;
        case CTC_LONG_LONG_INT: copy_typed_data(ce, make_basic_type(CTC_LONG_LONG_INT), to_data(expr->intc)); break;
        case CTC_UNSIGNED_INT: copy_typed_data(ce, make_basic_type(CTC_UNSIGNED_INT), to_data(expr->intc)); break;
        case CTC_UNSIGNED_LONG_INT: copy_typed_data(ce, make_basic_type(CTC_UNSIGNED_LONG_INT), to_data(expr->intc)); break;
        case CTC_UNSIGNED_LONG_LONG_INT: copy_typed_data(ce, make_basic_type(CTC_UNSIGNED_LONG_LONG_INT), to_data(expr->intc)); break;
        // these are integer types, but they are not possible to be stored as integer constants explicitly
        // case CTC_BOOL:
        // case CTC_CHAR:
        // case CTC_SIGNED_CHAR:
        // case CTC_SHORT_INT:
        // case CTC_UNSIGNED_CHAR:
        // case CTC_UNSIGNED_SHORT_INT:
        default:
            SET_ERROR(expr, "integer constant encountered with unexpected type");
            break;
    }
}

void evaluate_primary_expression_identifier(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        c_namespace_t* ns = syntax_get_namespace(expr);
        symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, expr, ns);
        namespace_delete(ns);
        if (!sy) report_return;
        if (symbol_get_storage_duration(sy) != SD_STATIC)
        {
            // ISO: 6.6 (9)
            SET_ERROR(expr, "objects designated in an address constant must have static storage duration");
            return;
        }
        ce->content.addr.sy = sy;
        ce->ct = type_copy(sy->type);
        return;
    }
}

void evaluate_string_literal(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, expr);
        if (!sy) report_return;
        ce->content.addr.sy = sy;
        ce->ct = type_copy(sy->type);
        return;
    }
}

void evaluate_compound_literal(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_ADDRESS)
    {
        symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, expr);
        if (!sy) report_return;
        ce->content.addr.sy = sy;
        ce->ct = type_copy(sy->type);
        return;
    }
}

void evaluate_primary_expression_enumeration_constant(syntax_component_t* expr, constexpr_t* ce)
{
    c_namespace_t* ns = syntax_get_namespace(expr);
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, expr, ns);
    namespace_delete(ns);
    if (!sy) report_return;
    int value = sy->declarer->parent->enumr_value;
    copy_typed_data(ce, make_basic_type(CTC_INT), to_data(value));
}

void evaluate_character_constant(syntax_component_t* expr, constexpr_t* ce)
{
    if (expr->ctype->class != CTC_INT)
    {
        SET_ERROR(expr, "character constant encountered with unexpected type");
        return;
    }
    copy_typed_data(ce, make_basic_type(CTC_INT), to_data(expr->charc_value));
}

void evaluate_floating_constant(syntax_component_t* expr, constexpr_t* ce)
{
    if (ce->type == CE_INTEGER && expr->parent->type != SC_CAST_EXPRESSION)
    {
        SET_ERROR(expr, "floating constants are disallowed in integer constant expressions unless if one is an immediate operand to a cast");
        return;
    }
    switch (expr->ctype->class)
    {
        case CTC_FLOAT:
            float f = (float) expr->floc;
            copy_typed_data(ce, make_basic_type(CTC_FLOAT), to_data(f));
            break;
        case CTC_DOUBLE:
            double d = (double) expr->floc;
            copy_typed_data(ce, make_basic_type(CTC_DOUBLE), to_data(d));
            break;
        case CTC_LONG_DOUBLE:
            long double ld = (long double) expr->floc;
            copy_typed_data(ce, make_basic_type(CTC_LONG_DOUBLE), to_data(ld));
            break;
        default:
            SET_ERROR(expr, "floating constant encountered with unexpected type");
            break;
    }
}

static void evaluate(syntax_component_t* expr, constexpr_t* ce)
{
    if (!expr) return;
    switch (expr->type)
    {
        case SC_PLUS_EXPRESSION: evaluate_plus_expression(expr, ce); break;
        case SC_MINUS_EXPRESSION: evaluate_minus_expression(expr, ce); break;
        case SC_ADDITION_EXPRESSION: evaluate_addition_expression(expr, ce); break;
        case SC_SUBTRACTION_EXPRESSION: evaluate_subtraction_expression(expr, ce); break;
        case SC_MULTIPLICATION_EXPRESSION: evaluate_multiplication_expression(expr, ce); break;
        case SC_DIVISION_EXPRESSION: evaluate_division_expression(expr, ce); break;
        case SC_MODULAR_EXPRESSION: evaluate_modular_expression(expr, ce); break;
        case SC_COMPLEMENT_EXPRESSION: evaluate_complement_expression(expr, ce); break;
        case SC_BITWISE_AND_EXPRESSION: evaluate_bitwise_and_expression(expr, ce); break;
        case SC_BITWISE_OR_EXPRESSION: evaluate_bitwise_or_expression(expr, ce); break;
        case SC_BITWISE_XOR_EXPRESSION: evaluate_bitwise_xor_expression(expr, ce); break;
        case SC_BITWISE_LEFT_EXPRESSION: evaluate_bitwise_left_expression(expr, ce); break;
        case SC_BITWISE_RIGHT_EXPRESSION: evaluate_bitwise_right_expression(expr, ce); break;
        case SC_NOT_EXPRESSION: evaluate_not_expression(expr, ce); break;
        case SC_LOGICAL_AND_EXPRESSION: evaluate_logical_and_expression(expr, ce); break;
        case SC_LOGICAL_OR_EXPRESSION: evaluate_logical_or_expression(expr, ce); break;
        case SC_EQUALITY_EXPRESSION: evaluate_equality_expression(expr, ce); break;
        case SC_INEQUALITY_EXPRESSION: evaluate_inequality_expression(expr, ce); break;
        case SC_LESS_EXPRESSION: evaluate_less_expression(expr, ce); break;
        case SC_LESS_EQUAL_EXPRESSION: evaluate_less_equal_expression(expr, ce); break;
        case SC_GREATER_EXPRESSION: evaluate_greater_expression(expr, ce); break;
        case SC_GREATER_EQUAL_EXPRESSION: evaluate_greater_equal_expression(expr, ce); break;
        case SC_SUBSCRIPT_EXPRESSION: evaluate_subscript_expression(expr, ce); break;
        case SC_DEREFERENCE_EXPRESSION: evaluate_dereference_expression(expr, ce); break;
        case SC_REFERENCE_EXPRESSION: evaluate_reference_expression(expr, ce); break;
        case SC_DEREFERENCE_MEMBER_EXPRESSION: evaluate_dereference_member_expression(expr, ce); break;
        case SC_MEMBER_EXPRESSION: evaluate_member_expression(expr, ce); break;
        case SC_CAST_EXPRESSION: evaluate_cast_expression(expr, ce); break;
        case SC_CONDITIONAL_EXPRESSION: evaluate_conditional_expression(expr, ce); break;
        case SC_SIZEOF_EXPRESSION: evaluate_sizeof_expression(expr, ce); break;
        case SC_SIZEOF_TYPE_EXPRESSION: evaluate_sizeof_type_expression(expr, ce); break;
        case SC_INTEGER_CONSTANT: evaluate_integer_constant(expr, ce); break;
        case SC_PRIMARY_EXPRESSION_IDENTIFIER: evaluate_primary_expression_identifier(expr, ce); break;
        case SC_PRIMARY_EXPRESSION_ENUMERATION_CONSTANT: evaluate_primary_expression_enumeration_constant(expr, ce); break;
        case SC_CHARACTER_CONSTANT: evaluate_character_constant(expr, ce); break;
        case SC_FLOATING_CONSTANT: evaluate_floating_constant(expr, ce); break;
        case SC_STRING_LITERAL: evaluate_string_literal(expr, ce); break;
        case SC_COMPOUND_LITERAL: evaluate_compound_literal(expr, ce); break;
        default: break;
    }
}

bool constexpr_evaluation_succeeded(constexpr_t* ce)
{
    if (!ce) return false;
    if (!ce->ct) return false;
    return ce->ct->class != CTC_ERROR;
}

static constexpr_t* constexpr_evaluate_type(syntax_component_t* expr, constexpr_type_t type)
{
    constexpr_t* ce = calloc(1, sizeof *ce);
    ce->type = type;
    ce->ct = make_basic_type(CTC_ERROR);
    if (!expr->ctype)
    {
        SET_ERROR(expr, "expression is not typed");
        return ce;
    }
    evaluate(expr, ce);
    if (type == CE_INTEGER && !type_is_integer(ce->ct))
        // ISO: 6.6 (6)
        SET_ERROR(expr, "integer constant expression must have an integer type");
    if (type == CE_ARITHMETIC && !type_is_arithmetic(ce->ct))
        // ISO: 6.6 (8)
        SET_ERROR(expr, "arithmetic constant expression must have an arithmetic type");
    return ce;
}

static bool constexpr_can_evaluate_type(syntax_component_t* expr, constexpr_type_t type)
{
    constexpr_t* ce = constexpr_evaluate_type(expr, type);
    bool success = constexpr_evaluation_succeeded(ce);
    constexpr_delete(ce);
    return success;
}

/*

what's allowed:
unary +, unary -, +, -, *, /, %, ~, &, |, ^, <<, >>, !,
&&, ||, ==, !=, <, >, <=, >=, [], unary *, unary &, ->, .,
(type) special case, ?:, non-VLA sizeof,
integer constants, enumeration constants, character constants,
float constants immediately converted to integer constants

*/
constexpr_t* constexpr_evaluate_integer(syntax_component_t* expr)
{
    return constexpr_evaluate_type(expr, CE_INTEGER);
}

constexpr_t* constexpr_evaluate_arithmetic(syntax_component_t* expr)
{
    return constexpr_evaluate_type(expr, CE_ARITHMETIC);
}

constexpr_t* constexpr_evaluate_address(syntax_component_t* expr)
{
    constexpr_t* ce = calloc(1, sizeof *ce);
    ce->type = CE_ADDRESS;
    ce->ct = make_basic_type(CTC_ERROR);
    if (!expr->ctype)
    {
        SET_ERROR(expr, "expression is not typed");
        return ce;
    }
    if (expr->type == SC_CAST_EXPRESSION)
    {
        c_type_t* ct = create_type(expr->caexpr_type_name, expr->caexpr_type_name->tn_declarator);
        if (ct->class != CTC_POINTER)
        {
            type_delete(ct);
            goto after_cast;
        }
        constexpr_t* ice = constexpr_evaluate_integer(expr->caexpr_operand);
        if (!constexpr_evaluation_succeeded(ice))
        {
            type_delete(ct);
            constexpr_delete(ice);
            goto after_cast;
        }
        type_delete(ce->ct);
        ce->ct = ct;
        constexpr_convert_class(ice, CTC_LONG_LONG_INT);
        int64_t value = constexpr_as_i64(ice);
        constexpr_delete(ice);
        ce->content.addr.offset = value;
        return ce;
    }
after_cast:
    evaluate(expr, ce);
    return ce;
}

bool constexpr_can_evaluate_integer(syntax_component_t* expr)
{
    return constexpr_can_evaluate_type(expr, CE_INTEGER);
}

bool constexpr_can_evaluate_arithmetic(syntax_component_t* expr)
{
    return constexpr_can_evaluate_type(expr, CE_ARITHMETIC);
}

bool constexpr_can_evaluate_address(syntax_component_t* expr)
{
    return constexpr_can_evaluate_type(expr, CE_ADDRESS);
}

bool constexpr_can_evaluate(syntax_component_t* expr)
{
    return constexpr_can_evaluate_type(expr, CE_ARITHMETIC) ||
        constexpr_can_evaluate_type(expr, CE_INTEGER) ||
        constexpr_can_evaluate_type(expr, CE_ADDRESS);
}

constexpr_t* constexpr_evaluate(syntax_component_t* expr)
{
    constexpr_t* ce = constexpr_evaluate_arithmetic(expr);
    if (constexpr_evaluation_succeeded(ce)) return ce;

    constexpr_delete(ce);

    ce = constexpr_evaluate_integer(expr);
    if (constexpr_evaluation_succeeded(ce)) return ce;

    constexpr_delete(ce);

    return constexpr_evaluate_address(expr);
}
