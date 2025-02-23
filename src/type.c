#include <stdlib.h>
#include <string.h>

#include "ecc.h"

c_type_t* make_basic_type(c_type_class_t class)
{
    c_type_t* ct = calloc(1, sizeof *ct);
    ct->class = class;
    return ct;
}

c_type_t* type_copy(c_type_t* ct)
{
    if (!ct) return NULL;
    c_type_t* nct = calloc(1, sizeof *nct);
    nct->class = ct->class;
    nct->qualifiers = ct->qualifiers;
    switch (nct->class)
    {
        case CTC_ARRAY:
            nct->array.length_expression = ct->array.length_expression;
            break;
        case CTC_STRUCTURE:
        case CTC_UNION:
            if (ct->struct_union.name)
                nct->struct_union.name = strdup(ct->struct_union.name);
            nct->struct_union.member_names = vector_deep_copy(ct->struct_union.member_names, (void* (*)(void*)) strdup);
            nct->struct_union.member_types = vector_deep_copy(ct->struct_union.member_types, (void* (*)(void*)) type_copy);
            nct->struct_union.member_bitfields = vector_copy(ct->struct_union.member_bitfields);
            break;
        case CTC_FUNCTION:
            nct->function.param_types = vector_deep_copy(ct->function.param_types, (void* (*)(void*)) type_copy);
            nct->function.variadic = ct->function.variadic;
            break;
        case CTC_ENUMERATED:
            if (ct->enumerated.name)
                nct->enumerated.name = strdup(ct->enumerated.name);
            nct->enumerated.constant_names = vector_deep_copy(ct->enumerated.constant_names, (void* (*)(void*)) strdup);
            nct->enumerated.constant_expressions = vector_copy(ct->enumerated.constant_expressions);
            break;
        default:
            break;
    }
    nct->derived_from = type_copy(ct->derived_from);
    return nct;
}

bool type_is_compatible(c_type_t* t1, c_type_t* t2)
{
    if (!t1 && !t2) return true;
    if (!t1) return false;
    if (!t2) return false;
    if (t1->class != t2->class)
        return false;
    if (t1->qualifiers != t2->qualifiers)
        return false;
    if ((t1->derived_from || t2->derived_from) && !type_is_compatible(t1->derived_from, t2->derived_from))
        return false;
    switch (t1->class)
    {
        case CTC_ARRAY:
            if (t1->array.length_expression != t2->array.length_expression)
                return false;
            break;
        case CTC_STRUCTURE:
            if (!streq(t1->struct_union.name, t2->struct_union.name))
                return false;
            if (t1->struct_union.member_names && t2->struct_union.member_names)
            {
                if (!vector_equals(t1->struct_union.member_names, t2->struct_union.member_names, (bool (*)(void*, void*)) streq))
                    return false;
                if (!vector_equals(t1->struct_union.member_types, t2->struct_union.member_types, (bool (*)(void*, void*)) type_is_compatible))
                    return false;
                // ensure the bitfield widths, if any, are equal
                VECTOR_FOR(syntax_component_t*, bitexpr1, t1->struct_union.member_bitfields)
                {
                    syntax_component_t* bitexpr2 = vector_get(t2->struct_union.member_bitfields, i);
                    if (!bitexpr1 && !bitexpr2)
                        continue;
                    if (!bitexpr1 || !bitexpr2)
                        return false;
                    c_type_class_t c1 = CTC_ERROR;
                    unsigned long long v1 = evaluate_constant_expression(bitexpr1, &c1, CE_INTEGER);
                    c_type_class_t c2 = CTC_ERROR;
                    unsigned long long v2 = evaluate_constant_expression(bitexpr2, &c2, CE_INTEGER);
                    if (c1 == CTC_ERROR || c2 == CTC_ERROR)
                        return false;
                    if (v1 != v2)
                        return false;
                }
            }
            break;
        case CTC_UNION:
        {
            // ensure tag names are equal
            if (!streq(t1->struct_union.name, t2->struct_union.name))
                return false;
            // if it is a complete type, we have to check members
            if (t1->struct_union.member_names && t2->struct_union.member_names)
            {
                if (t1->struct_union.member_names->size != t2->struct_union.member_names->size)
                    return false;
                // loop over all members of one type
                VECTOR_FOR(char*, mname, t1->struct_union.member_names)
                {
                    // try to find a member with the same name in the other type
                    int j = vector_contains(t2->struct_union.member_names, mname, (int (*)(void*, void*)) strcmp);
                    // if it doesn't exist, the types aren't compatible
                    if (j == -1)
                        return false;
                    // the types of the members have to be compatible as well
                    c_type_t* mtype1 = vector_get(t1->struct_union.member_types, i);
                    c_type_t* mtype2 = vector_get(t2->struct_union.member_types, j);
                    if (!type_is_compatible(mtype1, mtype2))
                        return false;
                    // if the members have bitfield widths, they have to be equal
                    syntax_component_t* bitexpr1 = vector_get(t1->struct_union.member_bitfields, i);
                    syntax_component_t* bitexpr2 = vector_get(t2->struct_union.member_bitfields, j);
                    if (!bitexpr1 && !bitexpr2)
                        continue;
                    if (!bitexpr1 || !bitexpr2)
                        return false;
                    c_type_class_t c1 = CTC_ERROR;
                    unsigned long long v1 = evaluate_constant_expression(bitexpr1, &c1, CE_INTEGER);
                    c_type_class_t c2 = CTC_ERROR;
                    unsigned long long v2 = evaluate_constant_expression(bitexpr2, &c2, CE_INTEGER);
                    if (c1 == CTC_ERROR || c2 == CTC_ERROR)
                        return false;
                    if (v1 != v2)
                        return false;
                }
            }
            break;
        }
        case CTC_FUNCTION:
            if (!vector_equals(t1->function.param_types, t2->function.param_types, (bool (*)(void*, void*)) type_is_compatible))
                return false;
            if (t1->function.variadic != t2->function.variadic)
                return false;
            break;
        case CTC_ENUMERATED:
            if (!streq(t1->enumerated.name, t2->enumerated.name))
                return false;
            // if the type is complete, we have to check to see if they have the same constants with same values
            if (t1->enumerated.constant_names && t2->enumerated.constant_names)
            {
                // this is pretty much the same dealio as with unions, just dealing with enum constants and their values
                if (t1->enumerated.constant_names->size != t2->enumerated.constant_names->size)
                    return false;
                VECTOR_FOR(char*, cname, t1->enumerated.constant_names)
                {
                    int j = vector_contains(t2->enumerated.constant_names, cname, (int (*)(void*, void*)) strcmp);
                    if (j == -1)
                        return false;
                    syntax_component_t* cexpr1 = vector_get(t1->enumerated.constant_expressions, i);
                    syntax_component_t* cexpr2 = vector_get(t2->enumerated.constant_expressions, j);
                    if (!cexpr1 && !cexpr2)
                        continue;
                    if (!cexpr1 || !cexpr2)
                        return false;
                    if (evaluate_enumeration_constant(cexpr1) != evaluate_enumeration_constant(cexpr2))
                        return false;
                }
            }
            break;
        default:
            break;
    }
    return true;
}

// ISO: 6.2.7 (3)
// creates the corresponding composite type of t1 and t2 (if any, otherwise returns NULL) 
c_type_t* type_compose(c_type_t* t1, c_type_t* t2)
{
    if (!type_is_compatible(t1, t2))
        return NULL;
    
    c_type_t* composed = NULL;
    
    // arrays of known constant size are composite
    if (t1->class == CTC_ARRAY && can_evaluate(t1->array.length_expression, CE_INTEGER))
        composed = type_copy(t1);
    else if (t2->class == CTC_ARRAY && can_evaluate(t2->array.length_expression, CE_INTEGER))
        composed = type_copy(t2);
    
    // VLAs are composite
    else if (t1->class == CTC_ARRAY && (t1->array.unspecified_size || !can_evaluate(t1->array.length_expression, CE_INTEGER)))
        composed = type_copy(t1);
    else if (t2->class == CTC_ARRAY && (t2->array.unspecified_size || !can_evaluate(t2->array.length_expression, CE_INTEGER)))
        composed = type_copy(t2);
    
    else if (t1->class == CTC_FUNCTION && t2->class == CTC_FUNCTION)
    {
        // functions where one is w/o a type list and one has one has a composite of the one with the type list
        if (t1->function.param_types && !t2->function.param_types)
            composed = type_copy(t1);
        else if (!t1->function.param_types && t2->function.param_types)
            composed = type_copy(t2);
        
        // functions who both have type lists have a composite type with the composites of all their parameters
        else if (t1->function.param_types && t2->function.param_types)
        {
            // compose the parameters
            composed = make_basic_type(CTC_FUNCTION);
            composed->derived_from = type_copy(t1->derived_from);
            composed->qualifiers = t1->qualifiers;
            composed->function.param_types = vector_init();
            
            for (unsigned i = 0; i < t1->function.param_types->size; ++i)
            {
                c_type_t* t1_pt = vector_get(t1->function.param_types, i);
                c_type_t* t2_pt = vector_get(t2->function.param_types, i);
                c_type_t* composed_pt = type_compose(t1_pt, t2_pt);
                if (!composed_pt) report_return_value(NULL);
                vector_add(composed->function.param_types, composed_pt);
            }
        }
    }
    if (composed)
    {
        c_type_t* d_composed = type_compose(t1->derived_from, t2->derived_from);
        if (d_composed)
        {
            type_delete(composed->derived_from);
            composed->derived_from = d_composed;
        }
    }
    return composed ? composed : type_copy(t1);
}

bool type_is_compatible_ignore_qualifiers(c_type_t* t1, c_type_t* t2)
{
    c_type_t* nq1 = type_copy(t1);
    c_type_t* nq2 = type_copy(t2);
    nq1->qualifiers = nq2->qualifiers = 0;
    bool result = type_is_compatible(nq1, nq2);
    type_delete(nq1);
    type_delete(nq2);
    return result;
}

unsigned char qualifiers_to_bitfield(vector_t* quals)
{
    if (!quals) return 0;
    unsigned char bf = 0;
    VECTOR_FOR(syntax_component_t*, qual, quals)
    {
        if (qual->type != SC_TYPE_QUALIFIER) continue;
        switch (qual->tq)
        {
            case TQ_CONST: bf |= TQ_B_CONST; break;
            case TQ_VOLATILE: bf |= TQ_B_VOLATILE; break;
            case TQ_RESTRICT: bf |= TQ_B_RESTRICT; break;
        }
    }
    return bf;
}

bool type_is_signed_integer_type(c_type_class_t class)
{
    return class == CTC_SIGNED_CHAR ||
            class == CTC_SHORT_INT ||
            class == CTC_INT ||
            class == CTC_LONG_INT ||
            class == CTC_LONG_LONG_INT;
}

bool type_is_signed_integer(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_signed_integer_type(ct->class);
}

bool type_is_unsigned_integer_type(c_type_class_t class)
{
    return class == CTC_BOOL ||
            class == CTC_UNSIGNED_CHAR ||
            class == CTC_UNSIGNED_SHORT_INT ||
            class == CTC_UNSIGNED_INT ||
            class == CTC_UNSIGNED_LONG_INT ||
            class == CTC_UNSIGNED_LONG_LONG_INT;
}

bool type_is_unsigned_integer(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_unsigned_integer_type(ct->class);
}

bool type_is_integer_type(c_type_class_t class)
{
    return class == CTC_CHAR ||
            type_is_signed_integer_type(class) ||
            type_is_unsigned_integer_type(class);
}

bool type_is_integer(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_integer_type(ct->class);
}

int get_integer_type_conversion_rank(c_type_class_t class)
{
    if (!type_is_integer_type(class))
        return -1;
    if (class == CTC_BOOL)
        return 0;
    if (class == CTC_CHAR || class == CTC_SIGNED_CHAR || class == CTC_UNSIGNED_CHAR)
        return 1;
    if (class == CTC_SHORT_INT || class == CTC_UNSIGNED_SHORT_INT)
        return 2;
    if (class == CTC_INT || class == CTC_UNSIGNED_INT)
        return 3;
    if (class == CTC_LONG_INT || class == CTC_UNSIGNED_LONG_INT)
        return 4;
    if (class == CTC_LONG_LONG_INT || class == CTC_UNSIGNED_LONG_LONG_INT)
        return 5; 
    return -1;
}

int get_integer_conversion_rank(c_type_t* ct)
{
    if (!ct) return -1;
    return get_integer_type_conversion_rank(ct->class);
}

bool type_is_complete(c_type_t* ct)
{
    if (!ct) return false;
    if (ct->class == CTC_ARRAY && !ct->array.length_expression)
        return false;
    if (ct->class == CTC_VOID)
        return false;
    if ((ct->class == CTC_STRUCTURE || ct->class == CTC_UNION) && (!ct->struct_union.member_names || !ct->struct_union.member_types))
        return false;
    return true;
}

bool type_is_object_type(c_type_t* ct)
{
    if (!ct) return false;
    if (!type_is_complete(ct))
        return false;
    if (ct->class == CTC_FUNCTION)
        return false;
    return true;
}

bool type_is_real_floating_type(c_type_class_t class)
{
    return class == CTC_FLOAT ||
        class == CTC_DOUBLE ||
        class == CTC_LONG_DOUBLE;
}

bool type_is_real_floating(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_real_floating_type(ct->class);
}

bool type_is_real_type(c_type_class_t class)
{
    return type_is_real_floating_type(class) ||
        type_is_integer_type(class);
}

bool type_is_real(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_real_type(ct->class);
}

bool type_is_complex_type(c_type_class_t class)
{
    return class == CTC_FLOAT_COMPLEX ||
        class == CTC_DOUBLE_COMPLEX ||
        class == CTC_LONG_DOUBLE_COMPLEX;
}

bool type_is_complex(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_complex_type(ct->class);
}

bool type_is_floating_type(c_type_class_t class)
{
    return type_is_real_floating_type(class) ||
        type_is_complex_type(class);
}

bool type_is_floating(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_real_floating_type(ct->class);
}

bool type_is_arithmetic_type(c_type_class_t class)
{
    return type_is_integer_type(class) ||
        type_is_floating_type(class);
}

bool type_is_arithmetic(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_arithmetic_type(ct->class);
}

bool type_is_scalar_type(c_type_class_t class)
{
    return type_is_arithmetic_type(class) ||
        class == CTC_POINTER;
}

bool type_is_scalar(c_type_t* ct)
{
    if (!ct) return false;
    return type_is_scalar_type(ct->class);
}

c_type_t* integer_promotions(c_type_t* ct)
{
    if (!ct) return NULL;
    if (!type_is_integer(ct)) return NULL;
    c_type_t* int_type = make_basic_type(CTC_INT);
    if (get_integer_conversion_rank(ct) < get_integer_conversion_rank(int_type))
        return int_type;
    type_delete(int_type);
    return type_copy(ct);
}

// recursively strips the qualifiers of a type
c_type_t* strip_qualifiers(c_type_t* ct)
{
    if (!ct) return NULL;
    ct = type_copy(ct);
    ct->qualifiers = 0;
    for (c_type_t* d = ct->derived_from; d; d = d->derived_from)
        d->qualifiers = 0;
    return ct;
}

c_type_class_t retain_type_domain(c_type_class_t old, c_type_class_t new_real)
{
    if (old == CTC_FLOAT_COMPLEX ||
        old == CTC_DOUBLE_COMPLEX ||
        old == CTC_LONG_DOUBLE_COMPLEX)
    {
        if (new_real == CTC_FLOAT)
            return CTC_FLOAT_COMPLEX;
        if (new_real == CTC_DOUBLE)
            return CTC_DOUBLE_COMPLEX;
        if (new_real == CTC_LONG_DOUBLE)
            return CTC_LONG_DOUBLE_COMPLEX;
    }
    return new_real;
}

c_type_class_t get_real_type(c_type_class_t class)
{
    if (class == CTC_FLOAT_COMPLEX)
        return CTC_FLOAT;
    if (class == CTC_DOUBLE_COMPLEX)
        return CTC_DOUBLE;
    if (class == CTC_LONG_DOUBLE_COMPLEX)
        return CTC_LONG_DOUBLE;
    return class;
}

c_type_class_t get_unsigned_equivalent(c_type_class_t class)
{
    if (class == CTC_SIGNED_CHAR)
        return CTC_UNSIGNED_CHAR;
    if (class == CTC_SHORT_INT)
        return CTC_UNSIGNED_SHORT_INT;
    if (class == CTC_INT)
        return CTC_UNSIGNED_INT;
    if (class == CTC_LONG_INT)
        return CTC_UNSIGNED_LONG_INT;
    if (class == CTC_LONG_LONG_INT)
        return CTC_UNSIGNED_LONG_LONG_INT;
    return class;
}

void usual_arithmetic_conversions(c_type_t* t1, c_type_t* t2, c_type_t** conv_t1, c_type_t** conv_t2)
{
    *conv_t1 = *conv_t2 = NULL;
    if (!type_is_arithmetic(t1))
        return;
    if (!type_is_arithmetic(t2))
        return;
    if (get_real_type(t1->class) == CTC_LONG_DOUBLE || get_real_type(t2->class) == CTC_LONG_DOUBLE)
    {
        *conv_t1 = make_basic_type(retain_type_domain(t1->class, CTC_LONG_DOUBLE));
        *conv_t2 = make_basic_type(retain_type_domain(t2->class, CTC_LONG_DOUBLE));
        return;
    }
    if (get_real_type(t1->class) == CTC_DOUBLE || get_real_type(t2->class) == CTC_DOUBLE)
    {
        *conv_t1 = make_basic_type(retain_type_domain(t1->class, CTC_DOUBLE));
        *conv_t2 = make_basic_type(retain_type_domain(t2->class, CTC_DOUBLE));
        return;
    }
    if (get_real_type(t1->class) == CTC_FLOAT || get_real_type(t2->class) == CTC_FLOAT)
    {
        *conv_t1 = make_basic_type(retain_type_domain(t1->class, CTC_FLOAT));
        *conv_t2 = make_basic_type(retain_type_domain(t2->class, CTC_FLOAT));
        return;
    }
    c_type_t* p1 = integer_promotions(t1);
    c_type_t* p2 = integer_promotions(t2);
    if (p1->class == p2->class)
    {
        *conv_t1 = p1;
        *conv_t2 = p2;
        return;
    }
    if ((type_is_signed_integer(p1) && type_is_signed_integer(p2)) ||
        (type_is_unsigned_integer(p1) && type_is_unsigned_integer(p2)))
    {
        if (get_integer_conversion_rank(p1) > get_integer_conversion_rank(p2))
        {
            *conv_t1 = p1;
            *conv_t2 = type_copy(p1);
            type_delete(p2);
            return;
        }
        else
        {
            *conv_t1 = type_copy(p2);
            *conv_t2 = p2;
            type_delete(p1);
            return;
        }
    }
    if (type_is_unsigned_integer(p1) && get_integer_conversion_rank(p1) >= get_integer_conversion_rank(p2))
    {
        *conv_t1 = p1;
        *conv_t2 = type_copy(p1);
        type_delete(p2);
        return;
    }
    if (type_is_unsigned_integer(p2) && get_integer_conversion_rank(p2) >= get_integer_conversion_rank(p1))
    {
        *conv_t1 = type_copy(p2);
        *conv_t2 = p2;
        type_delete(p1);
        return;
    }
    if (type_is_signed_integer(p1) && get_integer_conversion_rank(p2) <= get_integer_conversion_rank(p1) - 1)
    {
        *conv_t1 = p1;
        *conv_t2 = type_copy(p1);
        type_delete(p2);
        return;
    }
    if (type_is_signed_integer(p2) && get_integer_conversion_rank(p1) <= get_integer_conversion_rank(p2) - 1)
    {
        *conv_t1 = type_copy(p2);
        *conv_t2 = p2;
        type_delete(p1);
        return;
    }
    if (type_is_signed_integer(p1))
    {
        c_type_t* u = make_basic_type(get_unsigned_equivalent(p1->class));
        *conv_t1 = u;
        *conv_t2 = type_copy(u);
    }
    else
    {
        c_type_t* u = make_basic_type(get_unsigned_equivalent(p2->class));
        *conv_t1 = u;
        *conv_t2 = type_copy(u);
    }
    type_delete(p1);
    type_delete(p2);
}

c_type_t* usual_arithmetic_conversions_result_type(c_type_t* t1, c_type_t* t2)
{
    c_type_t *conv_t1, *conv_t2;
    usual_arithmetic_conversions(t1, t2, &conv_t1, &conv_t2);
    if (!conv_t1 || !conv_t2)
        return NULL;
    if (type_is_complex(conv_t1))
    {
        type_delete(conv_t2);
        return conv_t1;
    }
    type_delete(conv_t1);
    return conv_t2;
}

// returns -1 if size is unknown
long long type_size(c_type_t* ct)
{
    if (!ct) return -1;
    switch (ct->class)
    {
        case CTC_BOOL:
        case CTC_CHAR:
        case CTC_SIGNED_CHAR:
        case CTC_UNSIGNED_CHAR:
            return 1;
        case CTC_SHORT_INT:
        case CTC_UNSIGNED_SHORT_INT:
            return 2;
        case CTC_INT:
        case CTC_UNSIGNED_INT:
        case CTC_FLOAT:
        case CTC_FLOAT_IMAGINARY:
        case CTC_ENUMERATED:
            return 4;
        case CTC_LONG_INT:
        case CTC_UNSIGNED_LONG_INT:
        case CTC_LONG_LONG_INT:
        case CTC_UNSIGNED_LONG_LONG_INT:
        case CTC_DOUBLE:
        case CTC_FLOAT_COMPLEX:
        case CTC_DOUBLE_IMAGINARY:
            return 8;
        case CTC_LONG_DOUBLE:
        case CTC_DOUBLE_COMPLEX:
        case CTC_LONG_DOUBLE_IMAGINARY:
            return 16;
        case CTC_LONG_DOUBLE_COMPLEX:
            return 32;
        case CTC_POINTER:
        // technically any type you'd have to deal with the "size" of an object with function type
        // is if you're dealing with a function designator, which gets converted to a pointer anyways
        // but for safety this is here anyway
        case CTC_FUNCTION:
            return 8;
        case CTC_ARRAY:
        {
            c_type_class_t c = CTC_ERROR;
            long long length = evaluate_constant_expression(ct->array.length_expression, &c, CE_INTEGER);
            if (c == CTC_ERROR)
                return -1;
            long long dsize = type_size(ct->derived_from);
            if (dsize == -1)
                return -1;
            return length * dsize;
        }
        case CTC_STRUCTURE:
        case CTC_UNION:
        {
            long long size = 0;
            VECTOR_FOR(c_type_t*, mct, ct->struct_union.member_types)
            {
                long long msize = type_size(mct);
                if (msize == -1)
                {
                    if (i == ct->struct_union.member_types->size - 1 && mct->class == CTC_ARRAY)
                        continue;
                    return -1;
                }
                size += msize;
            }
            return size + (STRUCT_UNION_ALIGNMENT - (size % STRUCT_UNION_ALIGNMENT));
        }
        case CTC_VOID:
        case CTC_ERROR:
        case CTC_LABEL:
            return 0;
        default:
            return -1;
    }
}

void type_humanized_print(c_type_t* ct, int (*printer)(const char*, ...))
{
    if (!ct) return;
    printer("%s", C_TYPE_CLASS_NAMES[ct->class]);
    switch (ct->class)
    {
        case CTC_ENUMERATED:
            if (ct->enumerated.name)
                printer(" %s", ct->enumerated.name);
            break;
        case CTC_ARRAY:
            printer(" of ");
            break;
        case CTC_STRUCTURE:
        case CTC_UNION:
            if (ct->struct_union.name)
                printer(" %s", ct->struct_union.name);
            break;
        case CTC_FUNCTION:
            printer("(");
            if (ct->function.param_types)
            {
                VECTOR_FOR(c_type_t*, param_ct, ct->function.param_types)
                {
                    if (i) printer(", ");
                    type_humanized_print(param_ct, printer);
                }
                if (ct->function.variadic)
                    printer(", ...");
            }
            printer(") returning ");
            break;
        case CTC_POINTER:
            printer(" to ");
            break;
        default:
            break;
    }
    type_humanized_print(ct->derived_from, printer);
}

void type_delete(c_type_t* ct)
{
    if (!ct) return;
    type_delete(ct->derived_from);
    switch (ct->class)
    {
        case CTC_STRUCTURE:
        case CTC_UNION:
            vector_deep_delete(ct->struct_union.member_types, (void (*)(void*)) type_delete);
            vector_deep_delete(ct->struct_union.member_names, free);
            vector_delete(ct->struct_union.member_bitfields);
            break;
        case CTC_FUNCTION:
            vector_deep_delete(ct->function.param_types, (void (*)(void*)) type_delete);
            break;
        case CTC_ENUMERATED:
            vector_deep_delete(ct->enumerated.constant_names, free);
            vector_delete(ct->enumerated.constant_expressions);
            break;
        default:
            break;
    }
    free(ct);
}

#define ADD_ERROR(syn, fmt, ...) errors = error_list_add(errors, error_init(syn, false, fmt, ## __VA_ARGS__ ))
#define ADD_WARNING(syn, fmt, ...) errors = error_list_add(errors, error_init(syn, true, fmt, ## __VA_ARGS__ ))

c_namespace_t* make_basic_namespace(c_namespace_class_t class)
{
    c_namespace_t* ns = calloc(1, sizeof *ns);
    ns->class = class;
    return ns;
}

void assign_type(analysis_error_t* errors, symbol_t* sy);

c_type_t* create_struct_type(analysis_error_t* errors, syntax_component_t* sus)
{
    symbol_table_t* st = syntax_get_symbol_table(sus);
    c_type_t* ct = calloc(1, sizeof *ct);
    switch (sus->sus_sou)
    {
        case SOU_STRUCT: ct->class = CTC_STRUCTURE; break;
        case SOU_UNION: ct->class = CTC_UNION; break;
    }
    if (sus->sus_id)
        ct->struct_union.name = strdup(sus->sus_id->id);
    if (sus->sus_declarations)
    {
        ct->struct_union.member_names = vector_init();
        ct->struct_union.member_types = vector_init();
        ct->struct_union.member_bitfields = vector_init();
        VECTOR_FOR(syntax_component_t*, sdecl, sus->sus_declarations)
        {
            VECTOR_FOR(syntax_component_t*, sdeclr, sdecl->sdecl_declarators)
            {
                syntax_component_t* id = syntax_get_declarator_identifier(sdeclr);
                if (!id) continue; // TODO: unnamed bitfields (ugh)

                symbol_t* member_sy = symbol_table_get_syn_id(st, id);
                if (!member_sy) report_return_value(NULL);

                assign_type(errors, member_sy);

                vector_add(ct->struct_union.member_names, strdup(id->id));
                vector_add(ct->struct_union.member_types, type_copy(member_sy->type));
                vector_add(ct->struct_union.member_bitfields, sdeclr->sdeclr_bits_expression);
            }
        }
        {
            VECTOR_FOR(syntax_component_t*, sdecl, sus->sus_declarations)
            {
                VECTOR_FOR(syntax_component_t*, sdeclr, sdecl->sdecl_declarators)
                {
                    syntax_component_t* id = syntax_get_declarator_identifier(sdeclr);
                    if (!id) continue; // TODO: unnamed bitfields (ugh)
    
                    symbol_t* member_sy = symbol_table_get_syn_id(st, id);
                    if (!member_sy) report_return_value(NULL);
    
                    if (member_sy->ns)
                        namespace_delete(member_sy->ns);
                    member_sy->ns = calloc(1, sizeof *member_sy->ns);
                    member_sy->ns->class = ct->class == CTC_STRUCTURE ? NSC_STRUCT_MEMBER : NSC_UNION_MEMBER;
                    member_sy->ns->struct_union_type = type_copy(ct);
                }
            }
        }
    }
    return ct;
}

void assign_sus_type(analysis_error_t* errors, symbol_t* sy)
{
    if (!sy) return;
    if (sy->type) return;
    syntax_component_t* sus = sy->declarer->parent;
    sy->type = create_struct_type(errors, sus);
    sy->ns = make_basic_namespace(sy->type->class == CTC_STRUCTURE ? NSC_STRUCT : NSC_UNION);
}

void assign_es_type(analysis_error_t* errors, symbol_t* sy)
{
    if (!sy) return;
    if (sy->type) return;
    syntax_component_t* es = sy->declarer->parent;
    c_type_t* ct = calloc(1, sizeof *ct);

    ct->class = CTC_ENUMERATED;

    if (es->enums_id)
        ct->enumerated.name = strdup(es->enums_id->id);
    
    if (es->enums_enumerators)
    {
        ct->enumerated.constant_names = vector_init();
        ct->enumerated.constant_expressions = vector_init();
        VECTOR_FOR(syntax_component_t*, enumr, es->enums_enumerators)
        {
            vector_add(ct->enumerated.constant_names, strdup(enumr->enumr_constant->id));
            vector_add(ct->enumerated.constant_expressions, enumr->enumr_expression);
        }
    }

    sy->type = ct;
    sy->ns = make_basic_namespace(NSC_ENUM);
}

static bool bts_counter_check(size_t* bts_counter, size_t* check)
{
    for (size_t i = 0; i < BTS_NO_ELEMENTS; ++i)
    {
        if (bts_counter[i] != check[i])
            return false;
    }
    return true;
}

/*

this function's relatively simple despite its long length:
 - skim the declaration specifiers for type specifiers and count them.
 - based on the number of them, create a type.

*/
// syn can be any declaration, function definition, or a type name
static c_type_t* process_declspecs(analysis_error_t* errors, syntax_component_t* syn)
{
    vector_t* declspecs;
    switch (syn->type)
    {
        case SC_DECLARATION: declspecs = syn->decl_declaration_specifiers; break;
        case SC_STRUCT_DECLARATION: declspecs = syn->sdecl_specifier_qualifier_list; break;
        case SC_PARAMETER_DECLARATION: declspecs = syn->pdecl_declaration_specifiers; break;
        case SC_FUNCTION_DEFINITION: declspecs = syn->fdef_declaration_specifiers; break;
        case SC_TYPE_NAME: declspecs = syn->tn_specifier_qualifier_list; break;
        default: report_return_value(NULL);
    }
    size_t bts_counter[BTS_NO_ELEMENTS];
    memset(bts_counter, 0, BTS_NO_ELEMENTS * sizeof(size_t));
    size_t sus_count = 0, es_count = 0, tn_count = 0;
    syntax_component_t *sus = NULL, *es = NULL, *tn = NULL;

    c_type_t* ct = calloc(1, sizeof *ct);
    VECTOR_FOR(syntax_component_t*, s, declspecs)
    {
        switch (s->type)
        {
            case SC_BASIC_TYPE_SPECIFIER: ++bts_counter[s->bts]; break;
            case SC_STRUCT_UNION_SPECIFIER:
            {
                ++sus_count;
                sus = s;
                break;
            }
            case SC_ENUM_SPECIFIER:
            {
                ++es_count;
                es = s;
                break;
            }
            case SC_IDENTIFIER:
            case SC_TYPEDEF_NAME:
            {
                ++tn_count;
                tn = s;
                break;
            }
            case SC_TYPE_QUALIFIER: ct->qualifiers |= (1 << s->tq); break;
            default:
                break;
        }
    }

    size_t check_array[BTS_NO_ELEMENTS];
    #define check(sus, es, tn) \
        bts_counter_check(bts_counter, check_array) && \
        (sus) == sus_count && \
        (es) == es_count && \
        (tn) == tn_count
    #define zero_check memset(check_array, 0, sizeof(size_t) * BTS_NO_ELEMENTS)

    // void

    zero_check;
    check_array[BTS_VOID] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_VOID;
        return ct;
    }

    // char

    zero_check;
    check_array[BTS_CHAR] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_CHAR;
        return ct;
    }
    
    // signed char

    zero_check;
    check_array[BTS_CHAR] = 1;
    check_array[BTS_SIGNED] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_SIGNED_CHAR;
        return ct;
    }
    
    // unsigned char

    zero_check;
    check_array[BTS_CHAR] = 1;
    check_array[BTS_UNSIGNED] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_CHAR;
        return ct;
    }
    
    // short

    zero_check;
    check_array[BTS_SHORT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_SHORT_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SHORT] = 1;
    check_array[BTS_SIGNED] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_SHORT_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SHORT] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_SHORT_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    check_array[BTS_SHORT] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_SHORT_INT;
        return ct;
    }
    
    // unsigned short

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_SHORT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_SHORT_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_SHORT] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_SHORT_INT;
        return ct;
    }
    
    // int

    zero_check;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_INT;
        return ct;
    }

    // unsigned int

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_INT;
        return ct;
    }

    // long

    zero_check;
    check_array[BTS_LONG] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    check_array[BTS_LONG] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_LONG] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    check_array[BTS_LONG] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_INT;
        return ct;
    }

    // unsigned long

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_LONG] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_LONG] = 1;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_LONG_INT;
        return ct;
    }

    // long long

    zero_check;
    check_array[BTS_LONG] = 2;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    check_array[BTS_LONG] = 2;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_LONG] = 2;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_SIGNED] = 1;
    check_array[BTS_LONG] = 2;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_LONG_INT;
        return ct;
    }

    // unsigned long long

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_LONG] = 2;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_LONG_LONG_INT;
        return ct;
    }

    zero_check;
    check_array[BTS_UNSIGNED] = 1;
    check_array[BTS_LONG] = 2;
    check_array[BTS_INT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_UNSIGNED_LONG_LONG_INT;
        return ct;
    }

    // float

    zero_check;
    check_array[BTS_FLOAT] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_FLOAT;
        return ct;
    }

    // double

    zero_check;
    check_array[BTS_DOUBLE] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_DOUBLE;
        return ct;
    }

    // long double

    zero_check;
    check_array[BTS_DOUBLE] = 1;
    check_array[BTS_LONG] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_DOUBLE;
        return ct;
    }
    
    // _Bool
    
    zero_check;
    check_array[BTS_BOOL] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_BOOL;
        return ct;
    }

    // float _Complex

    zero_check;
    check_array[BTS_FLOAT] = 1;
    check_array[BTS_COMPLEX] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_FLOAT_COMPLEX;
        return ct;
    }

    // double _Complex

    zero_check;
    check_array[BTS_DOUBLE] = 1;
    check_array[BTS_COMPLEX] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_DOUBLE_COMPLEX;
        return ct;
    }

    // long double _Complex

    zero_check;
    check_array[BTS_DOUBLE] = 1;
    check_array[BTS_LONG] = 1;
    check_array[BTS_COMPLEX] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_DOUBLE_COMPLEX;
        return ct;
    }

    // float _Imaginary

    zero_check;
    check_array[BTS_FLOAT] = 1;
    check_array[BTS_IMAGINARY] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_FLOAT_IMAGINARY;
        return ct;
    }

    // double _Imaginary

    zero_check;
    check_array[BTS_DOUBLE] = 1;
    check_array[BTS_IMAGINARY] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_DOUBLE_IMAGINARY;
        return ct;
    }

    // long double _Imaginary

    zero_check;
    check_array[BTS_DOUBLE] = 1;
    check_array[BTS_LONG] = 1;
    check_array[BTS_IMAGINARY] = 1;
    if (check(0, 0, 0))
    {
        ct->class = CTC_LONG_DOUBLE_IMAGINARY;
        return ct;
    }

    symbol_table_t* st = syntax_get_symbol_table(syn);

    // struct/union specifier
    
    zero_check;
    if (check(1, 0, 0))
    {
        // do a symbol table lookup to get the proper of symbol for the struct, whether
        // that be one created by this struct specifier or a specifier elsewhere
        if (sus->sus_id)
        {
            c_namespace_t ns = get_basic_namespace(sus->sus_sou == SOU_STRUCT ? NSC_STRUCT : NSC_UNION);
            symbol_t* struct_sy = symbol_table_lookup(st, sus->sus_id, &ns);
            if (!struct_sy)
            {
                ADD_ERROR(sus, "type '%s %s' is not defined in this context",
                    sus->sus_sou == SOU_STRUCT ? "struct" : "union", sus->sus_id->id);
                return NULL;
            }
            assign_sus_type(errors, struct_sy);
            type_delete(ct);
            return type_copy(struct_sy->type);
        }
        return create_struct_type(errors, sus);
    }

    // enum specifier
    
    if (check(0, 1, 0))
    {
        c_namespace_t ns = get_basic_namespace(NSC_ENUM);
        symbol_t* enum_sy = symbol_table_lookup(st, es->enums_id, &ns);
        if (!enum_sy)
        {
            ADD_ERROR(sus, "type 'enum %s' is not defined in this context", es->enums_id->id);
            return NULL;
        }
        assign_es_type(errors, enum_sy);
        type_delete(ct);
        return type_copy(enum_sy->type);
    }

    // typedef name

    if (check(0, 0, 1))
    {
        c_namespace_t ns = get_basic_namespace(NSC_ORDINARY);
        symbol_t* tn_sy = symbol_table_lookup(st, tn, &ns);
        if (!tn_sy)
        {
            ADD_ERROR(sus, "type '%s' is not defined in this context", tn->id);
            return NULL;
        }
        type_delete(ct);
        return type_copy(tn_sy->type);
    }

    #undef check

    // ISO: 6.7.2 (2)
    ADD_ERROR(syn, "invalid type specifier list");
    return NULL;
}

c_type_t* create_type_with_errors(analysis_error_t* errors, syntax_component_t* specifying, syntax_component_t* declr)
{
    c_type_t* base = process_declspecs(errors, specifying);

    if (!base)
        return make_basic_type(CTC_ERROR);

    if (!declr)
        return base;

    declr = syntax_get_full_declarator(declr);

    if (!declr)
        report_return_value(NULL);

    if (declr->type == SC_INIT_DECLARATOR)
        declr = declr->ideclr_declarator;
    else if (declr->type == SC_STRUCT_DECLARATOR)
        declr = declr->sdeclr_declarator;

    symbol_table_t* st = syntax_get_symbol_table(declr);

    while (declr && declr->type != SC_IDENTIFIER)
    {
        switch (declr->type)
        {
            case SC_ABSTRACT_DECLARATOR:
            {
                if (declr->abdeclr_pointers && declr->abdeclr_pointers->size)
                {
                    VECTOR_FOR(syntax_component_t*, s, declr->declr_pointers)
                    {
                        c_type_t* ct = calloc(1, sizeof *ct);
                        ct->class = CTC_POINTER;
                        ct->qualifiers = qualifiers_to_bitfield(s->ptr_type_qualifiers);
                        ct->derived_from = base;
                        base = ct;
                    }
                }
                declr = declr->abdeclr_direct;
                break;
            }

            case SC_ABSTRACT_ARRAY_DECLARATOR:
            {
                c_type_t* ct = calloc(1, sizeof *ct);
                ct->class = CTC_ARRAY;
                ct->array.length_expression = declr->abadeclr_length_expression;
                ct->array.unspecified_size = declr->abadeclr_unspecified_size;
                ct->derived_from = base;
                base = ct;
                declr = declr->abadeclr_direct;
                break;
            }

            case SC_ABSTRACT_FUNCTION_DECLARATOR:
            {
                c_type_t* ct = calloc(1, sizeof *ct);
                ct->class = CTC_FUNCTION;

                if (declr->abfdeclr_parameter_declarations)
                {
                    ct->function.param_types = vector_init();
                    // loop over every parameter declaration
                    VECTOR_FOR(syntax_component_t*, s, declr->abfdeclr_parameter_declarations)
                    {
                        // if the parameter declarator is abstract, find the type of it
                        if (syntax_is_abstract_declarator_type(s->pdecl_declr->type))
                            vector_add(ct->function.param_types, create_type_with_errors(errors, s, s->pdecl_declr));
                        else
                        {
                            // otherwise, we make sure the identifier-type declarator is typed and we add that type to the vector
                            syntax_component_t* id = syntax_get_declarator_identifier(s->pdecl_declr);
                            if (!id) report_return_value(NULL);

                            symbol_t* param_sy = symbol_table_get_syn_id(st, id);
                            if (!param_sy) report_return_value(NULL);

                            assign_type(errors, param_sy);

                            vector_add(ct->function.param_types, type_copy(param_sy->type));
                        }
                    }
                }
                ct->function.variadic = declr->abfdeclr_ellipsis;
                ct->derived_from = base;
                base = ct;
                declr = declr->abfdeclr_direct;
                break;
            }
        
            case SC_DECLARATOR:
            {
                if (declr->declr_pointers && declr->declr_pointers->size)
                {
                    VECTOR_FOR(syntax_component_t*, s, declr->declr_pointers)
                    {
                        c_type_t* ct = calloc(1, sizeof *ct);
                        ct->class = CTC_POINTER;
                        ct->qualifiers = qualifiers_to_bitfield(s->ptr_type_qualifiers);
                        ct->derived_from = base;
                        base = ct;
                    }
                }
                declr = declr->declr_direct;
                break;
            }

            case SC_ARRAY_DECLARATOR:
            {
                c_type_t* ct = calloc(1, sizeof *ct);
                ct->class = CTC_ARRAY;
                ct->array.length_expression = declr->adeclr_length_expression;
                ct->array.unspecified_size = declr->adeclr_unspecified_size;
                ct->derived_from = base;
                base = ct;
                declr = declr->adeclr_direct;
                break;
            }

            case SC_FUNCTION_DECLARATOR:
            {
                c_type_t* ct = calloc(1, sizeof *ct);
                ct->class = CTC_FUNCTION;

                // only hand param types if the function uses new-style param declarations
                if (declr->fdeclr_parameter_declarations)
                {
                    ct->function.param_types = vector_init();
                    
                    // loop over every parameter declaration
                    VECTOR_FOR(syntax_component_t*, s, declr->fdeclr_parameter_declarations)
                    {
                        // if the parameter declarator is abstract, find the type of it
                        if (!s->pdecl_declr || syntax_is_abstract_declarator_type(s->pdecl_declr->type))
                            vector_add(ct->function.param_types, create_type_with_errors(errors, s, s->pdecl_declr));
                        else
                        {
                            // otherwise, we make sure the identifier-type declarator is typed and we add that type to the vector
                            syntax_component_t* id = syntax_get_declarator_identifier(s->pdecl_declr);
                            if (!id) report_return_value(NULL);

                            symbol_t* param_sy = symbol_table_get_syn_id(st, id);
                            if (!param_sy) report_return_value(NULL);

                            assign_type(errors, param_sy);

                            vector_add(ct->function.param_types, type_copy(param_sy->type));
                        }
                    }
                }
                ct->derived_from = base;
                ct->function.variadic = declr->fdeclr_ellipsis;
                base = ct;
                declr = declr->fdeclr_direct;
                break;
            }

            default:
                printf("%s\n", SYNTAX_COMPONENT_NAMES[declr->type]);
                report_return_value(NULL);
        }
    }
    return base;
}

c_type_t* create_type(syntax_component_t* specifying, syntax_component_t* declr)
{
    return create_type_with_errors(NULL, specifying, declr);
}

/*

identifier created for
	object: in generic/array/pointer declarator
	function: in function declarator
	tag of struct: in struct specifier
	tag of enum: in enum specifier
	member of struct: in declarator (struct declarator)
	member of enum: in enumerator
	typedef name: in declarator
	label name: in labeled statement (handled)
	macro name: irrelevant here (handled)
	macro parameter: irrelevant here (handled)

*/

void assign_type(analysis_error_t* errors, symbol_t* sy)
{
    if (!sy) return;
    if (sy->type) return;
    syntax_component_t* id = sy->declarer;
    if (!id->parent) report_return;

    syntax_component_t* parent = id->parent;

    // struct/union tag identifier
    if (parent->type == SC_STRUCT_UNION_SPECIFIER)
    {
        assign_sus_type(errors, sy);
        return;
    }

    // enum tag identifier
    if (parent->type == SC_ENUM_SPECIFIER)
    {
        assign_es_type(errors, sy);
        return;
    }

    // enumeration constant identifier
    if (parent->type == SC_ENUMERATOR)
    {
        sy->type = make_basic_type(CTC_INT);
        sy->ns = make_basic_namespace(NSC_ORDINARY);
        return;
    }

    // label identifier
    if (parent->type == SC_LABELED_STATEMENT)
    {
        sy->type = make_basic_type(CTC_LABEL); // fake type for label to avoid null ptr issues
        sy->ns = make_basic_namespace(NSC_LABEL);
        return;
    }

    // declarator identifier
    sy->ns = make_basic_namespace(NSC_ORDINARY);
    syntax_component_t* specifying = syntax_get_full_declarator(id);
    if (!specifying) report_return;
    specifying = specifying->parent;
    sy->type = create_type_with_errors(errors, specifying, id);
}

analysis_error_t* type(syntax_component_t* tlu)
{
    analysis_error_t* errors = NULL;
    ADD_ERROR(tlu, "dummy error");
    SYMBOL_TABLE_FOR_ENTRIES_START(k, sylist, tlu->tlu_st)
    {
        (void) k;
        for (; sylist; sylist = sylist->next)
            assign_type(errors, sylist);
    }
    SYMBOL_TABLE_FOR_ENTRIES_END

    analysis_error_t* start_errors = errors->next;
    error_delete(errors);
    return start_errors;
}