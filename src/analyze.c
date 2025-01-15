#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cc.h"

#define MAX_ERROR_LEN 512

typedef struct analysis_syntax_traverser
{
    syntax_traverser_t base;
    analysis_error_t* errors;
} analysis_syntax_traverser_t;

analysis_error_t* error_init(syntax_component_t* syn, bool warning, char* fmt, ...)
{
    analysis_error_t* err = calloc(1, sizeof *err);
    if (syn)
    {
        err->row = syn->row;
        err->col = syn->col;
    }
    err->message = malloc(MAX_ERROR_LEN);
    err->warning = warning;
    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, MAX_ERROR_LEN, fmt, args);
    va_end(args);
    return err;
}

void error_delete(analysis_error_t* err)
{
    if (!err) return;
    free(err->message);
    free(err);
}

void error_delete_all(analysis_error_t* errors)
{
    if (!errors) return;
    error_delete_all(errors->next);
    error_delete(errors);
}

analysis_error_t* error_list_add(analysis_error_t* errors, analysis_error_t* err)
{
    if (!errors) return err;
    analysis_error_t* head = errors;
    for (; errors->next; errors = errors->next);
    errors->next = err;
    return head;
}

size_t error_list_size(analysis_error_t* errors, bool include_warnings)
{
    size_t i = 0;
    for(; errors; errors = errors->next)
        if (include_warnings || !errors->warning)
            ++i;
    return i;
}

void dump_errors(analysis_error_t* errors)
{
    for (; errors; errors = errors->next)
        (errors->warning ? warnf : errorf)("[%d:%d] %s\n", errors->row, errors->col, errors->message);
}

#define ANALYSIS_TRAVERSER ((analysis_syntax_traverser_t*) trav)
#define ADD_ERROR(syn, fmt, ...) ANALYSIS_TRAVERSER->errors = error_list_add(ANALYSIS_TRAVERSER->errors, error_init(syn, false, fmt, ## __VA_ARGS__ ))
#define ADD_WARNING(syn, fmt, ...) ANALYSIS_TRAVERSER->errors = error_list_add(ANALYSIS_TRAVERSER->errors, error_init(syn, true, fmt, ## __VA_ARGS__ ))

// you should assign this to a variable if you plan on using it a lot.
#define SYMBOL_TABLE (syntax_get_translation_unit(syn)->tlu_st)

static size_t syntax_no_specifiers(vector_t* declspecs, syntax_component_type_t type)
{
    size_t i = 0;
    VECTOR_FOR(syntax_component_t*, declspec, declspecs)
    {
        if (declspec->type != type) continue;
        ++i;
    }
    return i;
}

static bool syntax_is_null_ptr_constant(syntax_component_t* expr, c_type_class_t* class)
{
    if (!expr) return false;
    if (class) *class = CTC_ERROR;
    if (expr->type == SC_CAST_EXPRESSION)
    {
        syntax_component_t* tn = expr->caexpr_type_name;
        if (!tn->tn_specifier_qualifier_list)
            return false;
        if (tn->tn_specifier_qualifier_list->size != 1)
            return false;
        syntax_component_t* ts = vector_get(tn->tn_specifier_qualifier_list, 0);
        if (ts->type != SC_BASIC_TYPE_SPECIFIER)
            return false;
        if (ts->bts != BTS_VOID)
            return false;
        if (!tn->tn_declarator)
            return false;
        syntax_component_t* abdeclr = tn->tn_declarator;
        if (abdeclr->type != SC_ABSTRACT_DECLARATOR)
            return false;
        if (!abdeclr->abdeclr_pointers)
            return false;
        if (abdeclr->abdeclr_pointers->size != 1)
            return false;
        syntax_component_t* ptr = vector_get(abdeclr->abdeclr_pointers, 0);
        if (ptr->ptr_type_qualifiers && ptr->ptr_type_qualifiers->size != 0)
            return false;
        expr = expr->caexpr_operand;
    }
    c_type_class_t c = CTC_ERROR;
    unsigned long long value = evaluate_constant_expression(expr, &c, CE_INTEGER);
    if (c == CTC_ERROR)
        return false;
    if (value != 0)
        return false;
    if (class) *class = c;
    return true;
}

/*

DEFINITIONS:

A **definition** of an identifier is a declaration for that identifier that:
    — for an object, causes storage to be reserved for that object;
    — for a function, includes the function body;
    — for an enumeration constant or typedef name, is the (only) declaration of the identifier

*/

/*

UNRESOLVED ISO SPECIFICATION REQUIREMENTS

6.5.3.4 (1)
6.5.15 (3)
6.5.15 (6)
6.5.16.1 (3): this one is interesting, idk if there are things to resolve for it

*/

/*

RESOLVED ISO SPECIFICATION REQUIREMENTS

6.5.3.3 (1)
6.5.3.3 (2)
6.5.3.3 (3)
6.5.3.3 (4)
6.5.3.3 (5)
6.5.3.4 (4)
6.5.4 (2)
6.5.4 (4)
6.5.5 (2)
6.5.5 (3)
6.5.6 (2)
6.5.6 (3)
6.5.6 (4)
6.5.6 (8)
6.5.6 (9)
6.5.7 (2)
6.5.7 (3)
6.5.8 (2)
6.5.8 (6)
6.5.9 (2)
6.5.9 (3)
6.5.10 (2)
6.5.10 (3)
6.5.11 (2)
6.5.11 (3)
6.5.12 (2)
6.5.12 (3)
6.5.13 (2)
6.5.13 (3)
6.5.14 (2)
6.5.14 (3)
6.5.15 (2)
6.5.15 (5)
6.5.16 (2)
6.5.16 (3)
6.5.16.1 (1)
6.5.16.2 (1)
6.5.16.2 (2)
6.5.17 (2)
6.7 (2)
6.7.2 (2)
6.9 (2)
6.9.1 (4)

*/

// syn: SC_DECLARATION | SC_FUNCTION_DEFINITION
static void enforce_6_9_para_2(syntax_traverser_t* trav, syntax_component_t* syn)
{
    vector_t* declspecs = NULL;
    switch (syn->type)
    {
        case SC_FUNCTION_DEFINITION: declspecs = syn->fdef_declaration_specifiers; break;
        case SC_DECLARATION: declspecs = syn->decl_declaration_specifiers; break;
        default: report_return;
    }
    VECTOR_FOR(syntax_component_t*, declspec, declspecs)
    {
        if (!declspec) report_continue;
        if (declspec->type == SC_STORAGE_CLASS_SPECIFIER &&
            (declspec->scs == SCS_AUTO || declspec->scs == SCS_REGISTER))
            // ISO: 6.9 (2)
            ADD_ERROR(declspec, "'%s' not allowed in external declaration", STORAGE_CLASS_NAMES[declspec->scs]);
    }
}

// syn: SC_FUNCTION_DEFINITION
static void enforce_6_9_1_para_4(syntax_traverser_t* trav, syntax_component_t* syn)
{
    size_t no_scs = syntax_no_specifiers(syn->fdef_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER);
    if (no_scs > 1)
        // ISO: 6.9.1 (4)
        ADD_ERROR(syn, "function definition should not have more than one storage class specifier");
    if (no_scs == 1 &&
        !syntax_has_specifier(syn->fdef_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER, SCS_EXTERN) &&
        !syntax_has_specifier(syn->fdef_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER, SCS_STATIC))
        // ISO: 6.9.1 (4)
        ADD_ERROR(syn, "'static' and 'extern' are the only allowed storage class specifiers for function definitions");
}

// syn: SC_DECLARATION
static void enforce_6_7_para_2(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (syn->decl_init_declarators->size)
        return;
    VECTOR_FOR(syntax_component_t*, s, syn->decl_declaration_specifiers)
    {
        if (s->type == SC_STRUCT_UNION_SPECIFIER && s->sus_id)
            return;
        if (s->type == SC_ENUM_SPECIFIER)
        {
            if (s->enums_id)
                return;
            if (s->enums_enumerators && s->enums_enumerators->size)
                return;
        }
    }
    // ISO: 6.7 (2)
    ADD_ERROR(syn, "a declaration must declare an identifier, struct/union/enum tag, or an enumeration constant");
}

// void type_struct_union_specifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
// {
//     c_type_t* ct = calloc(1, sizeof *ct);
//     setup_struct_type(syn, &ct);
//     syn->sus_id->ctype = ct;
// }

// void type_enumerator_after(syntax_traverser_t* trav, syntax_component_t* syn)
// {
//     // ISO: 6.4.4.3 (2)
//     syn->enumr_constant->ctype = make_basic_type(CTC_INT);
// }

// void type_enum_specifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
// {
//     c_type_t* ct = calloc(1, sizeof *ct);
//     setup_enum_type(syn, &ct);
//     syn->enums_id->ctype = ct;
// }

// void type_floating_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
// {
//     // ISO: 6.4.4.2 (4)
//     if (contains_substr(syn->con, "f") ||
//         contains_substr(syn->con, "F"))
//         syn->ctype = make_basic_type(CTC_FLOAT);
//     else if (contains_substr(syn->con, "L") ||
//         contains_substr(syn->con, "l"))
//         syn->ctype = make_basic_type(CTC_LONG_DOUBLE);
//     else
//         syn->ctype = make_basic_type(CTC_DOUBLE);
// }

// void type_character_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
// {
//     if (strlen(syn->con) > 0 && syn->con[0] == 'L')
//         syn->ctype = make_basic_type(CTC_UNSIGNED_INT);
//     else
//         syn->ctype = make_basic_type(CTC_INT);
// }

void analyze_subscript_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    syntax_component_t* array = syn->subsexpr_expression;
    syntax_component_t* index = syn->subsexpr_index_expression;
    if (index->ctype->class == CTC_ARRAY || index->ctype->class == CTC_POINTER)
    {
        syntax_component_t* tmp = array;
        array = index;
        index = tmp;
        pass = true;
    }
    else if (array->ctype->class != CTC_ARRAY && array->ctype->class != CTC_POINTER)
        // ISO: 6.5.2.1 (1)
        ADD_ERROR(syn, "subscript can only be applied to array and pointer types");
    else
        pass = true;
    
    if (pass)
    {
        pass = false;
        if (type_is_integer(index->ctype))
            pass = true;
        else
        {
            // ISO: 6.5.2.1 (1)
            ADD_ERROR(syn, "subscript index expression can only be of integer type");
        }
    }

    if (pass)
        // ISO: 6.5.2.1 (1)
        syn->ctype = type_copy(array->ctype->derived_from);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_function_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    
}

void analyze_dereference_member_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* tlhs = syn->memexpr_expression->ctype;
    syntax_component_t* id = syn->memexpr_id;
    int mem_idx = -1;
    if (tlhs->class != CTC_POINTER || (tlhs->derived_from->class != CTC_STRUCTURE && tlhs->derived_from->class != CTC_UNION))
    {
        // ISO: 6.5.2.3 (2)
        pass = false;
        ADD_ERROR(syn, "left hand side of dereferencing member access expression must be of struct/union type");
    }
    else if (!tlhs->derived_from->struct_union.member_names ||
        (mem_idx = vector_contains(tlhs->derived_from->struct_union.member_names, id->id, (int (*)(void*, void*)) strcmp)) == -1)
    {
        // ISO: 6.5.2.3 (2)
        pass = false;
        ADD_ERROR(syn, "struct/union has no such member '%s'", id->id);
    }

    if (pass)
    {
        // ISO: 6.5.2.3 (4)
        c_type_t* rt = type_copy(vector_get(tlhs->derived_from->struct_union.member_types, mem_idx));
        rt->qualifiers = rt->qualifiers | tlhs->derived_from->qualifiers;
        syn->ctype = rt;
    }
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_member_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* tlhs = syn->memexpr_expression->ctype;
    syntax_component_t* id = syn->memexpr_id;
    int mem_idx = -1;
    if (tlhs->class != CTC_STRUCTURE && tlhs->class != CTC_UNION)
    {
        // ISO: 6.5.2.3 (1)
        pass = false;
        ADD_ERROR(syn, "left hand side of member access expression must be of struct/union type");
    }
    else if (!tlhs->struct_union.member_names || (mem_idx = vector_contains(tlhs->struct_union.member_names, id->id, (int (*)(void*, void*)) strcmp)) == -1)
    {
        // ISO: 6.5.2.3 (1)
        pass = false;
        ADD_ERROR(syn, "struct/union has no such member '%s'", id->id);
    }

    if (pass)
    {
        // ISO: 6.5.2.3 (3)
        c_type_t* rt = type_copy(vector_get(tlhs->struct_union.member_types, mem_idx));
        rt->qualifiers = rt->qualifiers | tlhs->qualifiers;
        syn->ctype = rt;
    }
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_compound_literal_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    // TODO
}

void analyze_inc_dec_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (syntax_is_modifiable_lvalue(syn->uexpr_operand))
    {
        if (type_is_real(otype))
            // ISO: 6.5.2.4 (1), 6.5.3.1 (1)
            pass = true;
        else if (otype->class == CTC_POINTER)
            // ISO: 6.5.2.4 (1), 6.5.3.1 (1)
            pass = true;
    }
    if (pass)
        // ISO: 6.5.2.4 (2), 6.5.3.1 (2)
        syn->ctype = type_copy(otype);
    else
    {
        ADD_ERROR(syn, "invalid operand to increment/decrement operator");
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_dereference_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (otype->class != CTC_POINTER)
    {
        // ISO: 6.5.3.2 (2)
        ADD_ERROR(syn, "dereference operand must be of pointer type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.3.2 (4)
        syn->ctype = type_copy(otype->derived_from);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_reference_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (otype->class == CTC_FUNCTION)
        // ISO: 6.5.3.2 (1)
        pass = true;
    else if (syn->uexpr_operand->type == SC_SUBSCRIPT_EXPRESSION)
        // ISO: 6.5.3.2 (1)
        pass = true;
    else if (syn->uexpr_operand->type == SC_DEREFERENCE_EXPRESSION)
        // ISO: 6.5.3.2 (1)
        pass = true;
    // TODO: 6.5.3.2 (1) requires lvalues to be handled here as well
    if (pass)
    {
        c_type_t* ct = calloc(1, sizeof *ct);
        ct->class = CTC_POINTER;
        ct->derived_from = type_copy(otype);
        // ISO: 6.5.3.2 (3)
        syn->ctype = ct;
    }
    else
    {
        ADD_ERROR(syn, "invalid operand to address operator");
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_plus_minus_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (!type_is_arithmetic(otype))
    {
        // ISO: 6.5.3.3 (1)
        ADD_ERROR(syn, "plus/minus operand must be of arithmetic type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.3.3 (2), 6.5.3.3 (3)
        syn->ctype = integer_promotions(syn->uexpr_operand->ctype);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_complement_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (!type_is_integer(otype))
    {
        // ISO: 6.5.3.3 (1)
        ADD_ERROR(syn, "complement operand must of integer type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.3.3 (4)
        syn->ctype = integer_promotions(syn->uexpr_operand->ctype);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_not_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (!type_is_scalar(otype))
    {
        // ISO: 6.5.3.3 (1)
        ADD_ERROR(syn, "NOT operand must be of scalar type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.3.3 (5)
        syn->ctype = make_basic_type(CTC_INT);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_sizeof_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* otype = syn->uexpr_operand->ctype;
    if (otype->class == CTC_FUNCTION)
    {
        // ISO: 6.5.3.4 (1)
        ADD_ERROR(syn, "sizeof operand cannot be of function type");
        pass = false;
    }
    if (!type_is_complete(otype))
    {
        // ISO: 6.5.3.4 (1)
        ADD_ERROR(syn, "sizeof operand cannot be of incomplete type");
        pass = false;
    }
    // TODO: 6.5.3.4 (1) specifies sizeof cannot take a bitfield member
    if (pass)
        // ISO: 6.3.5.4 (4)
        syn->ctype = make_basic_type(C_TYPE_SIZE_T);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_cast_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    if (syn->caexpr_type_name->ctype->class != CTC_VOID && !type_is_scalar(syn->caexpr_operand->ctype))
    {
        // ISO: 6.5.4 (2)
        ADD_ERROR(syn, "operand of cast expression must be of scalar type");
        pass = false;
    }
    if (syn->caexpr_type_name->ctype->class != CTC_VOID && !type_is_scalar(syn->caexpr_type_name->ctype))
    {
        // ISO: 6.5.4 (2)
        ADD_ERROR(syn, "type name of cast expression must be of scalar type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.4 (4)
        syn->ctype = type_copy(syn->caexpr_type_name->ctype);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_modular_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (!type_is_integer(tlhs))
    {
        // ISO: 6.5.5 (2)
        ADD_ERROR(syn, "left hand side of modular expression must have an integer type");
        pass = false;
    }
    if (!type_is_integer(trhs))
    {
        // ISO: 6.5.5 (2)
        ADD_ERROR(syn, "right hand side of modular expression must have an integer type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.5 (3)
        syn->ctype = usual_arithmetic_conversions_result_type(tlhs, trhs);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_mult_div_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (!type_is_arithmetic(tlhs))
    {
        // ISO: 6.5.5 (2)
        ADD_ERROR(syn, "left hand side of multiplication/division expression must have an arithmetic type");
        pass = false;
    }
    if (!type_is_arithmetic(trhs))
    {
        // ISO: 6.5.5 (2)
        ADD_ERROR(syn, "right hand side of multiplication/division expression must have an arithmetic type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.5 (3)
        syn->ctype = usual_arithmetic_conversions_result_type(tlhs, trhs);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_addition_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_type_t* ct = NULL;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (type_is_arithmetic(tlhs) && type_is_arithmetic(trhs))
        // ISO: 6.5.6 (2), 6.5.6 (4)
        ct = usual_arithmetic_conversions_result_type(tlhs, trhs);
    else if (type_is_integer(tlhs) && trhs->class == CTC_POINTER)
        // ISO: 6.5.6 (2), 6.5.6 (8)
        ct = type_copy(trhs);
    if (!ct)
    {
        ADD_ERROR(syn, "invalid operands of addition expression");
        ct = make_basic_type(CTC_ERROR);
    }
    syn->ctype = ct;
}

void analyze_subtraction_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_type_t* ct = NULL;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (type_is_arithmetic(tlhs) && type_is_arithmetic(trhs))
        // ISO: 6.5.6 (3), 6.5.6 (4)
        ct = usual_arithmetic_conversions_result_type(tlhs, trhs);
    else if (type_is_integer(tlhs) && trhs->class == CTC_POINTER)
        // ISO: 6.5.6 (3), 6.5.6 (8)
        ct = type_copy(trhs);
    else if (tlhs->class == CTC_POINTER && trhs->class == CTC_POINTER &&
        type_is_compatible_ignore_qualifiers(tlhs, trhs))
        // ISO: 6.5.6 (3), 6.5.6 (9)
        ct = make_basic_type(C_TYPE_PTRSIZE_T);
    if (!ct)
    {
        ADD_ERROR(syn, "invalid operands of subtraction expression");
        ct = make_basic_type(CTC_ERROR);
    }
    syn->ctype = ct;
}

void analyze_shift_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (!type_is_integer(tlhs))
    {
        // ISO: 6.5.7 (2)
        ADD_ERROR(syn, "left hand side of shift expression must have an integer type");
        pass = false;
    }
    if (!type_is_integer(trhs))
    {
        // ISO: 6.5.7 (2)
        ADD_ERROR(syn, "right hand side of shift expression must have an integer type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.7 (3)
        syn->ctype = integer_promotions(tlhs);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_relational_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (type_is_real(tlhs) && type_is_real(trhs))
        // ISO: 6.5.8 (2)
        pass = true;
    // "pointers to qualified or unqualified vers. of compatible object or incomplete types"
    else if (tlhs->class == CTC_POINTER && trhs->class == CTC_POINTER &&
        type_is_compatible_ignore_qualifiers(tlhs->derived_from, trhs->derived_from) &&
            ((type_is_object_type(tlhs->derived_from) && type_is_object_type(trhs->derived_from)) || 
            (!type_is_complete(tlhs->derived_from) && !type_is_complete(trhs->derived_from))))
        // ISO: 6.5.8 (2)
        pass = true;
    
    if (pass)
    {
        // ISO: 6.5.8 (6)
        syn->ctype = make_basic_type(CTC_INT);
    }
    else
    {
        ADD_ERROR(syn, "invalid operands of relational expression");
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_equality_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    if (type_is_arithmetic(tlhs) && type_is_arithmetic(trhs))
        // ISO: 6.5.9 (2)
        pass = true;
    else if (tlhs->class == CTC_POINTER && trhs->class == CTC_POINTER &&
        type_is_compatible_ignore_qualifiers(tlhs->derived_from, trhs->derived_from))
        // ISO: 6.5.9 (2)
        pass = true;
    else if (tlhs->class == CTC_POINTER && (type_is_object_type(tlhs) || !type_is_complete(tlhs)) &&
        trhs->class == CTC_POINTER && trhs->derived_from->class == CTC_VOID)
        // ISO: 6.5.9 (2)
        pass = true;
    else if (trhs->class == CTC_POINTER && (type_is_object_type(trhs) || !type_is_complete(trhs)) &&
        tlhs->class == CTC_POINTER && tlhs->derived_from->class == CTC_VOID)
        // ISO: 6.5.9 (2)
        pass = true;
    else if (tlhs->class == CTC_POINTER && syntax_is_null_ptr_constant(syn->bexpr_rhs, NULL))
        // ISO: 6.5.9 (2)
        pass = true;
    else if (trhs->class == CTC_POINTER && syntax_is_null_ptr_constant(syn->bexpr_lhs, NULL))
        // ISO: 6.5.9 (2)
        pass = true;

    if (pass)
    {
        // ISO: 6.5.9 (3)
        syn->ctype = make_basic_type(CTC_INT);
    }
    else
    {
        ADD_ERROR(syn, "invalid operands of equality expression");
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_bitwise_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    if (!type_is_integer(syn->bexpr_lhs->ctype))
    {
        // ISO: 6.5.10 (2), 6.5.11 (2), 6.5.12 (2)
        ADD_ERROR(syn, "left hand side of bitwise expression must have an integer type");
        pass = false;
    }
    if (!type_is_integer(syn->bexpr_rhs->ctype))
    {
        // ISO: 6.5.10 (2), 6.5.11 (2), 6.5.12 (2)
        ADD_ERROR(syn, "right hand side of bitwise expression must have an integer type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.10 (3), 6.5.11 (3), 6.5.12 (3)
        syn->ctype = usual_arithmetic_conversions_result_type(syn->bexpr_lhs->ctype, syn->bexpr_rhs->ctype);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_logical_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    if (!type_is_scalar(syn->bexpr_lhs->ctype))
    {
        // ISO: 6.5.13 (2), 6.5.14 (2)
        ADD_ERROR(syn, "left hand side of logical expression must have a scalar type");
        pass = false;
    }
    if (!type_is_scalar(syn->bexpr_rhs->ctype))
    {
        // ISO: 6.5.13 (2), 6.5.14 (2)
        ADD_ERROR(syn, "right hand side of logical expression must have a scalar type");
        pass = false;
    }
    if (pass)
        // ISO: 6.5.13 (3), 6.5.14 (3)
        syn->ctype = make_basic_type(CTC_INT);
    else
        syn->ctype = make_basic_type(CTC_ERROR);
}

void analyze_conditional_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_type_t* ft = NULL;
    if (!type_is_scalar(syn->cexpr_condition->ctype))
    {
        // ISO: 6.5.15 (2)
        ADD_ERROR(syn, "condition of a conditional expression must have a scalar type");
        ft = make_basic_type(CTC_ERROR);
    }
    
    c_type_t* op2_type = syn->cexpr_if->ctype;
    c_type_t* op3_type = syn->cexpr_else->ctype;
    if (type_is_arithmetic(op2_type) && type_is_arithmetic(op3_type))
    {
        c_type_t* result_type = usual_arithmetic_conversions_result_type(op2_type, op3_type);
        if (!result_type) report_return;
        // ISO: 6.5.15 (5)
        if (!ft) ft = result_type;
    }
    else if ((op2_type->class == CTC_STRUCTURE || op2_type->class == CTC_UNION) &&
            (op3_type->class == CTC_STRUCTURE || op3_type->class == CTC_UNION) &&
            type_is_compatible(op2_type, op3_type))
    {
        // ISO: 6.5.15 (5)
        if (!ft) ft = type_copy(op2_type);
    }
    else if (op2_type->class == CTC_VOID && op3_type->class == CTC_VOID)
    {
        // ISO: 6.5.15 (5)
        if (!ft) ft = make_basic_type(CTC_VOID);
    }
    else if (op2_type->class == CTC_POINTER && op3_type->class == CTC_POINTER &&
            type_is_compatible_ignore_qualifiers(op2_type->derived_from, op3_type->derived_from))
    {
        if (!ft)
        {
            // ISO: 6.5.15 (6)
            c_type_t* rt = make_basic_type(CTC_POINTER);
            rt->derived_from = type_compose(op2_type->derived_from, op3_type->derived_from);
            rt->derived_from->qualifiers = op2_type->derived_from->qualifiers | op3_type->derived_from->qualifiers;
            ft = rt;
        }
    }
    // 6.5.15 (6) is so hard to read omgggggggggggggggggggggggggggggggggg
    else if (op2_type->class == CTC_POINTER && syntax_is_null_ptr_constant(syn->cexpr_else, NULL))
    {
        // ISO: 6.5.15 (6)
        //c_type_t* rt = make_basic_type(CTC_POINTER);
    }
    else if (op3_type->class == CTC_POINTER && syntax_is_null_ptr_constant(syn->cexpr_if, NULL))
    {

    }
        
    // TODO: ISO: 6.5.15 (6)
}

void analyze_simple_assignment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    // ISO: 6.5.16.1 (1)
    // condition 1
    if (type_is_arithmetic(tlhs) &&
        type_is_arithmetic(trhs))
        pass = true;
    // ISO: 6.5.16.1 (1)
    // condition 2
    else if ((tlhs->class == CTC_STRUCTURE || tlhs->class == CTC_UNION) &&
        type_is_compatible(tlhs, trhs))
        pass = true;
    // ISO: 6.5.16.1 (1)
    // condition 3
    else if (tlhs->class == CTC_POINTER && tlhs->class == CTC_POINTER &&
        type_is_compatible(tlhs->derived_from, trhs->derived_from))
        pass = true;
    // ISO: 6.5.16.1 (1)
    // condition 4
    else if (tlhs->class == CTC_POINTER && (type_is_object_type(tlhs->derived_from) || !type_is_complete(tlhs->derived_from)) &&
        trhs->class == CTC_POINTER && trhs->derived_from->class == CTC_VOID && tlhs->derived_from->qualifiers == trhs->derived_from->qualifiers)
        pass = true;
    else if (trhs->class == CTC_POINTER && (type_is_object_type(trhs->derived_from) || !type_is_complete(trhs->derived_from)) &&
        tlhs->class == CTC_POINTER && tlhs->derived_from->class == CTC_VOID && tlhs->derived_from->qualifiers == trhs->derived_from->qualifiers)
        pass = true;
    // ISO: 6.5.16.1 (1)
    // condition 5
    else if (tlhs->class == CTC_POINTER && syntax_is_null_ptr_constant(syn->bexpr_rhs, NULL))
        pass = true;
    // ISO: 6.5.16.1 (1)
    // condition 6
    else if (tlhs->class == CTC_BOOL && trhs->class == CTC_POINTER)
        pass = true;
    if (!pass)
    {
        // TODO: come back and make this more useful of an error message
        ADD_ERROR(syn, "simple assignment operation is invalid");
        if (syn->ctype) type_delete(syn->ctype);
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_compound_assignment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = false;
    c_type_t* tlhs = syn->bexpr_lhs->ctype;
    c_type_t* trhs = syn->bexpr_rhs->ctype;
    switch (syn->type)
    {
        case SC_ADDITION_ASSIGNMENT_EXPRESSION:
        case SC_SUBTRACTION_ASSIGNMENT_EXPRESSION:
        {
            // ISO: 6.5.16.2 (1)
            if (tlhs->class == CTC_POINTER && type_is_object_type(tlhs->derived_from) && type_is_integer(trhs))
                pass = true;
            else if (type_is_arithmetic(tlhs) && type_is_arithmetic(trhs))
                pass = true;
            break;
        }
        case SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION:
        case SC_DIVISION_ASSIGNMENT_EXPRESSION:
        case SC_MODULAR_ASSIGNMENT_EXPRESSION:
        {
            // ISO: 6.5.16.2 (2), 6.5.5 (2)
            if (type_is_arithmetic(tlhs) && type_is_arithmetic(trhs))
                pass = true;
            break;
        }
        case SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_AND_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_OR_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION:
        {
            // ISO: 6.5.16.2 (2), 6.5.7 (2), 6.5.10 (2), 6.5.11 (2), 6.5.12 (2)
            if (type_is_integer(tlhs) && type_is_integer(trhs))
                pass = true;
            break;
        }
        default: report_return;
    }
    if (!pass)
    {
        // TODO: come back and make this more useful of an error message
        ADD_ERROR(syn, "compound assignment operation is invalid");
        if (syn->ctype) type_delete(syn->ctype);
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_assignment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_type_t* ft = NULL;
    if (!syn->bexpr_lhs || !syn->bexpr_rhs) report_return;
    if (!syntax_is_modifiable_lvalue(syn->bexpr_lhs))
    {
        // ISO: 6.5.16 (2)
        ADD_ERROR(syn, "left-hand side of assignment expression must be a modifiable lvalue");
        ft = make_basic_type(CTC_ERROR);
    }
    if (!ft)
    {
        // ISO: 6.5.16 (3)
        ft = type_copy(syn->bexpr_lhs->ctype);
        ft->qualifiers = 0;
    }
    syn->ctype = ft;
    if (syn->type == SC_ASSIGNMENT_EXPRESSION)
        analyze_simple_assignment_expression_after(trav, syn);
    else
        analyze_compound_assignment_expression_after(trav, syn);
}

void analyze_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (!syn->expr_expressions) report_return;
    syntax_component_t* last_expr = vector_get(syn->expr_expressions, syn->expr_expressions->size - 1);
    if (!last_expr) report_return;
    // ISO: 6.5.17 (2)
    syn->ctype = type_copy(last_expr->ctype);
}

void analyze_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_namespace_t* ns = syntax_get_namespace(syn);
    if (!ns) report_return;
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, syn, ns);
    if (sy)
    {
        syn->ctype = type_copy(sy->type);
        if (sy->type->class == CTC_FUNCTION && syn->parent &&
            syn->parent->type != SC_REFERENCE_EXPRESSION &&
            syn->parent->type != SC_SIZEOF_EXPRESSION &&
            syn->parent->type != SC_SIZEOF_TYPE_EXPRESSION)
        {
            // ISO: 6.3.2 (4)
            c_type_t* ptr_type = make_basic_type(CTC_POINTER);
            ptr_type->derived_from = syn->ctype;
            syn->ctype = ptr_type;
        }
    }
    else
    {
        // ISO: 6.5.1 (2)
        ADD_ERROR(syn, "symbol '%s' is not defined in the given context", syn->id);
        syn->ctype = make_basic_type(CTC_ERROR);
    }
}

void analyze_integer_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_type_class_t c = CTC_ERROR;
    process_integer_constant(syn, &c);
    syn->ctype = make_basic_type(c);
}

void analyze_function_definition_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    enforce_6_9_para_2(trav, syn);
    enforce_6_9_1_para_4(trav, syn);
}

void analyze_declaration_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    enforce_6_7_para_2(trav, syn);
    enforce_6_9_para_2(trav, syn);
}

void analyze_translation_unit_after(syntax_traverser_t* trav, syntax_component_t* syn)
{

}

/*

for an identifier, check to see if it's declaring or referencing.

if declaring, see if it's in a type specifier, declarator, or labeled statement.
    if a type specifier, create a type based on the struct/union/enum.
    if a declarator, create a type based on the full declarator and the declaration specifiers of its parent declaration.
    if a labeled statement, do not type it.

if referencing, duplicate the type of the declaring identifier.

*/

analysis_error_t* analyze(syntax_component_t* tlu)
{
    syntax_traverser_t* trav = traverse_init(tlu, sizeof(analysis_syntax_traverser_t));

    trav->after[SC_TRANSLATION_UNIT] = analyze_translation_unit_after;
    trav->after[SC_DECLARATION] = analyze_declaration_after;
    trav->after[SC_FUNCTION_DEFINITION] = analyze_function_definition_after;

    // expressions
    trav->after[SC_EXPRESSION] = analyze_expression_after;
    trav->after[SC_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_DIVISION_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_MODULAR_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_ADDITION_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_SUBTRACTION_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_BITWISE_AND_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_BITWISE_OR_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION] = analyze_assignment_expression_after;
    trav->after[SC_CONDITIONAL_EXPRESSION] = analyze_conditional_expression_after;
    trav->after[SC_LOGICAL_OR_EXPRESSION] = analyze_logical_expression_after;
    trav->after[SC_LOGICAL_AND_EXPRESSION] = analyze_logical_expression_after;
    trav->after[SC_BITWISE_OR_EXPRESSION] = analyze_bitwise_expression_after;
    trav->after[SC_BITWISE_XOR_EXPRESSION] = analyze_bitwise_expression_after;
    trav->after[SC_BITWISE_AND_EXPRESSION] = analyze_bitwise_expression_after;
    trav->after[SC_EQUALITY_EXPRESSION] = analyze_equality_expression_after;
    trav->after[SC_INEQUALITY_EXPRESSION] = analyze_equality_expression_after;
    trav->after[SC_GREATER_EQUAL_EXPRESSION] = analyze_relational_expression_after;
    trav->after[SC_GREATER_EXPRESSION] = analyze_relational_expression_after;
    trav->after[SC_LESS_EQUAL_EXPRESSION] = analyze_relational_expression_after;
    trav->after[SC_LESS_EXPRESSION] = analyze_relational_expression_after;
    trav->after[SC_BITWISE_LEFT_EXPRESSION] = analyze_shift_expression_after;
    trav->after[SC_BITWISE_RIGHT_EXPRESSION] = analyze_shift_expression_after;
    trav->after[SC_SUBTRACTION_EXPRESSION] = analyze_subtraction_expression_after;
    trav->after[SC_ADDITION_EXPRESSION] = analyze_addition_expression_after;
    trav->after[SC_MULTIPLICATION_EXPRESSION] = analyze_mult_div_expression_after;
    trav->after[SC_DIVISION_EXPRESSION] = analyze_mult_div_expression_after;
    trav->after[SC_MODULAR_EXPRESSION] = analyze_modular_expression_after;
    trav->after[SC_CAST_EXPRESSION] = analyze_cast_expression_after;
    trav->after[SC_SIZEOF_EXPRESSION] = analyze_sizeof_expression_after;
    trav->after[SC_SIZEOF_TYPE_EXPRESSION] = analyze_sizeof_expression_after;
    trav->after[SC_NOT_EXPRESSION] = analyze_not_expression_after;
    trav->after[SC_COMPLEMENT_EXPRESSION] = analyze_complement_expression_after;
    trav->after[SC_PLUS_EXPRESSION] = analyze_plus_minus_expression_after;
    trav->after[SC_MINUS_EXPRESSION] = analyze_plus_minus_expression_after;
    trav->after[SC_REFERENCE_EXPRESSION] = analyze_reference_expression_after;
    trav->after[SC_DEREFERENCE_EXPRESSION] = analyze_dereference_expression_after;
    trav->after[SC_PREFIX_INCREMENT_EXPRESSION] = analyze_inc_dec_expression_after;
    trav->after[SC_PREFIX_DECREMENT_EXPRESSION] = analyze_inc_dec_expression_after;
    trav->after[SC_POSTFIX_INCREMENT_EXPRESSION] = analyze_inc_dec_expression_after;
    trav->after[SC_POSTFIX_DECREMENT_EXPRESSION] = analyze_inc_dec_expression_after;
    trav->after[SC_INITIALIZER_LIST_EXPRESSION] = analyze_compound_literal_expression_after;
    trav->after[SC_MEMBER_EXPRESSION] = analyze_member_expression_after;
    trav->after[SC_DEREFERENCE_MEMBER_EXPRESSION] = analyze_dereference_member_expression_after;
    trav->after[SC_FUNCTION_CALL_EXPRESSION] = analyze_function_call_expression_after;
    trav->after[SC_SUBSCRIPT_EXPRESSION] = analyze_subscript_expression_after;
    trav->after[SC_INTEGER_CONSTANT] = analyze_integer_constant_after;
    trav->after[SC_IDENTIFIER] = analyze_identifier_after;


    traverse(trav);
    analysis_error_t* errors = ANALYSIS_TRAVERSER->errors;
    traverse_delete(trav);
    return errors;
}