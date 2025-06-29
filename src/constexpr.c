#include <stdlib.h>

#include "ecc.h"

void constexpr_delete(constexpr_t* ce)
{
    if (!ce) return;
    type_delete(ce->ct);
    free(ce);
}

void constexpr_print(constexpr_t* ce, int (*printer)(const char* fmt, ...))
{
    if (!ce)
    {
        printer("(null)");
        return;
    }
    switch (ce->type)
    {
        case CE_INTEGER:
            printer("%llu", ce->ivalue);
            break;
        case CE_ARITHMETIC:
            printer("%Lf", ce->fvalue);
            break;
        case CE_ADDRESS:
            // TODO: print address constexprs
            printer("(address expression)");
            break;
        default:
            printer("(unknown)");
            break;
    }
}

constexpr_t* constexpr_copy(constexpr_t* ce)
{
    if (!ce) return NULL;
    constexpr_t* n = calloc(1, sizeof *n);
    n->type = ce->type;
    n->ct = type_copy(ce->ct);
    switch (ce->type)
    {
        case CE_INTEGER:
            n->ivalue = ce->ivalue;
            break;
        case CE_ARITHMETIC:
            n->fvalue = ce->fvalue;
            break;
        case CE_ADDRESS:
            n->addrexpr = ce->addrexpr;
            break;
        default:
            break;
    }
    return n;
}

bool constexpr_equals(constexpr_t* ce1, constexpr_t* ce2)
{
    if (!ce1 && !ce2) return true;
    if (!ce1 || !ce2) return false;
    if (ce1->type != ce2->type) return false;
    if (!type_is_compatible(ce1->ct, ce2->ct)) return false;
    switch (ce1->type)
    {
        case CE_INTEGER:
            if (ce1->ivalue != ce2->ivalue)
                return false;
            break;
        case CE_ARITHMETIC:
            if (ce1->fvalue != ce2->fvalue)
                return false;
            break;
        case CE_ADDRESS:
            // kinda bad
            if (ce1->addrexpr != ce2->addrexpr)
                return false;
            break;
        default:
            break;
    }
    return true;
}

designation_t* designation_copy(designation_t* desig)
{
    if (!desig) return NULL;
    designation_t* n = calloc(1, sizeof *n);
    n->index = constexpr_copy(desig->index);
    n->member = desig->member;
    n->next = designation_copy(desig->next);
    return n;
}

void designation_delete(designation_t* desig)
{
    if (!desig) return;
    constexpr_delete(desig->index);
}

void designation_delete_all(designation_t* desig)
{
    if (!desig) return;
    designation_delete_all(desig->next);
    designation_delete(desig);
}

designation_t* syntax_to_designation(syntax_component_t* d)
{
    if (!d) return NULL;
    designation_t* desig = calloc(1, sizeof *desig);
    designation_t* curr = desig;
    VECTOR_FOR(syntax_component_t*, designator, d->desig_designators)
    {
        if (i != 0) curr = curr->next = calloc(1, sizeof *curr->next);
        if (syntax_is_identifier(designator->type))
        {
            c_namespace_t* ns = syntax_get_namespace(designator);
            symbol_t* sy = symbol_table_lookup(syntax_get_symbol_table(d), designator, ns);
            if (!sy)
            {
                designation_delete_all(desig);
                return NULL;
            }
            curr->member = sy;
        }
        else
        {
            constexpr_t* ce = ce_evaluate(designator, CE_ANY);
            if (!ce)
            {
                designation_delete_all(desig);
                return NULL;
            }
            curr->index = ce;
        }
    }
    return desig;
}

designation_t* designation_concat(designation_t* d1, designation_t* d2)
{
    if (!d1 && !d2) return NULL;
    if (!d1) return designation_copy(d2);
    if (!d2) return designation_copy(d1);
    designation_t* n1 = designation_copy(d1);
    designation_t* d = n1;
    designation_t* n2 = designation_copy(d2);
    for (; n1->next; n1 = n1->next);
    n1->next = n2;
    return d;
}

designation_t* get_full_designation(syntax_component_t* d)
{
    if (!d) return NULL;
    syntax_component_t* inlist = d->parent;
    if (!inlist || inlist->type != SC_INITIALIZER_LIST) return NULL;
    syntax_component_t* enclosing = inlist->parent;
    if (!enclosing) return NULL;
    if (enclosing->type == SC_INIT_DECLARATOR || enclosing->type == SC_COMPOUND_LITERAL)
        return syntax_to_designation(d);
    if (enclosing->type != SC_INITIALIZER_LIST)
        return NULL;
    syntax_component_t* ud = NULL;
    VECTOR_FOR(syntax_component_t*, ed, enclosing->inlist_designations)
    {
        syntax_component_t* hinlist = vector_get(enclosing->inlist_initializers, i);
        if (inlist != hinlist)
            continue;
        ud = ed;
        break;
    }
    designation_t* upper = get_full_designation(ud);
    designation_t* desig = designation_concat(upper, syntax_to_designation(d));
    designation_delete_all(upper);
    return desig;
}

void designation_print(designation_t* desig, int (*printer)(const char* fmt, ...))
{
    if (!desig)
    {
        printer("(null)");
        return;
    }
    if (desig->index)
    {
        printer("[");
        constexpr_print(desig->index, printer);
        printer("]");
    }
    else
        printer(".%s", symbol_get_name(desig->member));
    if (desig->next)
        designation_print(desig->next, printer);
}

bool designation_equals(designation_t* d1, designation_t* d2)
{
    if (!d1 && !d2) return true;
    if (!d1 || !d2) return false;
    if (d1->member != d2->member) return false;
    if (!constexpr_equals(d1->index, d2->index)) return false;
    return designation_equals(d1->next, d2->next);
}

constexpr_t* ce_make_integer(c_type_t* ct, unsigned long long value)
{
    constexpr_t* ce = calloc(1, sizeof *ce);
    ce->type = CE_INTEGER;
    ce->ct = ct;
    ce->ivalue = value;
    return ce;
}

#define errret { *class = CTC_ERROR; return; }

// static bool representable(unsigned long long value, c_type_class_t class)
// {
//     #define representable_case(ctc, type) case ctc: return value == (type) value;
//     switch (class)
//     {
//         representable_case(CTC_BOOL, bool)
//         representable_case(CTC_CHAR, char)
//         representable_case(CTC_SIGNED_CHAR, signed char)
//         representable_case(CTC_SHORT_INT, short int)
//         representable_case(CTC_INT, int)
//         representable_case(CTC_LONG_INT, long int)
//         representable_case(CTC_LONG_LONG_INT, long long int)
//         representable_case(CTC_UNSIGNED_CHAR, unsigned char)
//         representable_case(CTC_UNSIGNED_SHORT_INT, unsigned short int)
//         representable_case(CTC_UNSIGNED_INT, unsigned int)
//         representable_case(CTC_UNSIGNED_LONG_INT, unsigned long int)
//         representable_case(CTC_UNSIGNED_LONG_LONG_INT, unsigned long long int)
//         representable_case(CTC_FLOAT, float)
//         representable_case(CTC_DOUBLE, double)
//         representable_case(CTC_LONG_DOUBLE, long double)
//         representable_case(CTC_FLOAT_COMPLEX, float _Complex)
//         representable_case(CTC_DOUBLE_COMPLEX, double _Complex)
//         representable_case(CTC_LONG_DOUBLE_COMPLEX, long double _Complex)
//         default: return false;
//     }
// }

static void ce_evaluate_integer_value(syntax_component_t* expr, unsigned long long* value, c_type_class_t* class)
{
    if (!expr)
        errret;
    switch (expr->type)
    {
        case SC_INTEGER_CONSTANT:
        {
            *value = expr->intc;
            *class = expr->ctype->class;
            break;
        }
        case SC_ENUMERATION_CONSTANT:
        {
            c_namespace_t* ns = syntax_get_namespace(expr);
            symbol_t* sy = symbol_table_lookup(syntax_get_symbol_table(expr), expr, ns);
            if (!sy) errret;
            namespace_delete(ns);
            if (!sy->declarer->parent || sy->declarer->parent->type != SC_ENUMERATOR) errret;
            constexpr_t* ce = vector_get(sy->initial_values, 0);
            if (!ce) report_return; // enum constant initializer isn't added yet
            *value = ce->ivalue;
            *class = CTC_INT;
            break;
        }
        case SC_CHARACTER_CONSTANT:
        {
            *value = expr->charc_value;
            *class = CTC_INT;
            break;
        }
        case SC_SIZEOF_TYPE_EXPRESSION:
        {
            c_type_t* ct = create_type(expr->uexpr_operand, expr->uexpr_operand->tn_declarator);
            if (!ct) errret;
            long long size = type_size(ct);
            if (size == -1)
            {
                type_delete(ct);
                errret;
            }
            *value = size;
            *class = C_TYPE_SIZE_T;
            break;
        }
        case SC_SIZEOF_EXPRESSION:
        {
            c_type_t* ct = expr->uexpr_operand->ctype;
            if (!ct) errret;
            long long size = type_size(ct);
            if (size == -1) errret;
            *value = size;
            *class = C_TYPE_SIZE_T;
            break;
        }
        case SC_CAST_EXPRESSION:
        {
            c_type_t* ct = create_type(expr->caexpr_type_name, expr->caexpr_type_name->tn_declarator);
            if (!ct) errret;
            if (!type_is_integer(ct))
            {
                type_delete(ct);
                return;
            }
            // TODO
            break;
        }
        default:
        {
            *value = 0;
            *class = CTC_ERROR;
            break;
        }
    }
}

#undef errret

constexpr_t* ce_evaluate_integer(syntax_component_t* expr)
{
    if (!expr)
        return NULL;
    constexpr_t* ce = calloc(1, sizeof *ce);
    ce->type = CE_INTEGER;
    c_type_t* ct = calloc(1, sizeof *ct);
    ce_evaluate_integer_value(expr, &ce->ivalue, &ct->class);
    if (ct->class == CTC_ERROR)
    {
        constexpr_delete(ce);
        return NULL;
    }
    return ce;
}

constexpr_t* ce_evaluate_address(syntax_component_t* expr)
{
    return NULL;
}

constexpr_t* ce_evaluate_arithmetic(syntax_component_t* expr)
{
    return NULL;
}

constexpr_t* ce_evaluate(syntax_component_t* expr, constexpr_type_t type)
{
    if (!expr)
        return NULL;
    if (type == CE_ANY)
    {
        constexpr_t* ce = ce_evaluate_integer(expr);
        if (ce) return ce;
        ce = ce_evaluate_address(expr);
        if (ce) return ce;
        ce = ce_evaluate_arithmetic(expr);
        return ce;
    }
    if (type == CE_INTEGER)
        return ce_evaluate_integer(expr);
    if (type == CE_ADDRESS)
        return ce_evaluate_address(expr);
    if (type == CE_ARITHMETIC)
        return ce_evaluate_arithmetic(expr);
    return NULL;
}
