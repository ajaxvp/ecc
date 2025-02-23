#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "ecc.h"

#define MAX_ERROR_LEN 512

typedef struct analysis_syntax_traverser
{
    syntax_traverser_t base;
    analysis_error_t* errors;
    unsigned long long next_compound_literal;
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
#define SYMBOL_TABLE (trav->tlu->tlu_st)

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

6.5.2.5 (1-8)
6.5.3.2 (1)
6.5.3.4 (1)
6.5.15 (3)
6.5.15 (6)
6.5.16.1 (3): this one is interesting, idk if there are things to resolve for it
6.7 (4)
6.7 (7)
6.7.1 (6)
6.8.4.2 (2)
6.8.4.2 (3)
6.8.4.2 (5)
6.8.6.1 (1)

*/

/*

RESOLVED ISO SPECIFICATION REQUIREMENTS

6.3.2 (4)
6.5.1 (2)
6.5.2.2 (1)
6.5.2.2 (2)
6.5.2.2 (4)
6.5.2.2 (5)
6.5.2.3 (1)
6.5.2.3 (2)
6.5.2.3 (3): check up on after lvalue changes
6.5.2.3 (4): check up on after lvalue changes
6.5.2.4 (1): check up on after lvalue changes
6.5.2.4 (2)
6.5.3.1 (1): check up on after lvalue changes
6.5.3.2 (2)
6.5.3.2 (3)
6.5.3.2 (4)
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
6.7.1 (2)
6.7.2 (2)
6.8.1 (2)
6.8.1 (3)
6.9 (2)
6.9.1 (2)
6.9.1 (3)
6.9.1 (4)
6.9.1 (5)
6.9.1 (6)
6.9.2 (3)

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

static bool can_assign(c_type_t* tlhs, c_type_t* trhs, syntax_component_t* rhs)
{
    bool pass = false;
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
    else if (tlhs->class == CTC_POINTER && syntax_is_null_ptr_constant(rhs, NULL))
        pass = true;
    // ISO: 6.5.16.1 (1)
    // condition 6
    else if (tlhs->class == CTC_BOOL && trhs->class == CTC_POINTER)
        pass = true;
    return pass;
}

void analyze_function_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool pass = true;
    c_type_t* called_type = syn->fcallexpr_expression->ctype;
    if (called_type->class != CTC_POINTER || called_type->derived_from->class != CTC_FUNCTION)
    {
        // ISO: 6.5.2.2 (1)
        ADD_ERROR(syn, "calling expression in function call must be of function or function pointer type");
        pass = false;
    }
    else if (called_type->derived_from->derived_from->class != CTC_VOID && (!type_is_object_type(called_type->derived_from->derived_from) ||
        called_type->derived_from->derived_from->class == CTC_ARRAY))
    {
        // ISO: 6.5.2.2 (1)
        ADD_ERROR(syn, "function to be called must have a return type of void or an object type besides an array type");
        pass = false;
    }

    if (pass && called_type->derived_from->function.param_types)
    {
        if (called_type->derived_from->function.variadic && syn->fcallexpr_args->size < called_type->derived_from->function.param_types->size)
        {
            // ISO: ?
            ADD_ERROR(syn, "function to be called expected %u or more argument(s), got %u",
                called_type->derived_from->function.param_types->size,
                syn->fcallexpr_args->size);
        }
        else if (!called_type->derived_from->function.variadic && called_type->derived_from->function.param_types->size != syn->fcallexpr_args->size)
        {
            // ISO: 6.5.2.2 (2)
            ADD_ERROR(syn, "function to be called expected %u argument(s), got %u",
                called_type->derived_from->function.param_types->size,
                syn->fcallexpr_args->size);
            pass = false;
        }
        else
        {
            VECTOR_FOR(syntax_component_t*, rhs, syn->fcallexpr_args)
            {
                c_type_t* tlhs = vector_get(called_type->derived_from->function.param_types, i);
                if (!tlhs) // variadic arguments aren't going to have a type attached to them
                    break;
                if (!can_assign(tlhs, rhs->ctype, rhs))
                {
                    // ISO: 6.5.2.2 (2)
                    type_humanized_print(tlhs, printf);
                    printf("\n");
                    type_humanized_print(rhs->ctype, printf);
                    printf("\n");
                    ADD_ERROR(rhs, "invalid type of argument for this function call");
                    pass = false;
                }
            }
        }
    }

    VECTOR_FOR(syntax_component_t*, arg, syn->fcallexpr_args)
    {
        if (!type_is_object_type(arg->ctype))
        {
            // ISO: 6.5.2.2 (4)
            ADD_ERROR(arg, "argument in function call needs to be of object type");
            pass = false;
        }
    }

    if (pass)
    {
        if (type_is_object_type(called_type->derived_from->derived_from))
        {
            // ISO: 6.5.2.2 (5)
            syn->ctype = type_copy(called_type->derived_from->derived_from);
        }
        else
            // ISO: 6.5.2.2 (5)
            syn->ctype = make_basic_type(CTC_VOID);
    }
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

void analyze_compound_literal_expression_before(syntax_traverser_t* trav, syntax_component_t* syn)
{
    const size_t len = 4 + MAX_STRINGIFIED_INTEGER_LENGTH + 1; // __cl(number)(null)
    char name[len];
    snprintf(name, len, "__cl%llu", ANALYSIS_TRAVERSER->next_compound_literal);
    syn->cl_id = strdup(name);
    symbol_t* sy = symbol_table_add(SYMBOL_TABLE, name, symbol_init(syn));
    sy->ns = make_basic_namespace(NSC_ORDINARY);
    sy->type = create_type(syn->cl_type_name, syn->cl_type_name->tn_declarator);
    syn->ctype = type_copy(sy->type);
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
    if (!can_assign(syn->bexpr_lhs->ctype, syn->bexpr_rhs->ctype, syn->bexpr_rhs))
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

// syn: SC_TRANSLATION_UNIT
void enforce_6_9_para_3_clause_1(syntax_traverser_t* trav, syntax_component_t* syn)
{
    // TODO
}

void analyze_enumeration_constant_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy)
{
    syntax_component_t* enumr = sy->declarer->parent;
    if (!enumr)
        report_return;
    // if enumerator has a constant value associated with it, use that value
    if (enumr->enumr_expression)
    {
        constexpr_t* ce = ce_evaluate(enumr->enumr_expression, CE_INTEGER);
        if (!ce)
        {
            // ISO: 6.7.2.2 (2)
            ADD_ERROR(enumr->enumr_expression, "enumeration constant value must be specified by an integer constant expression");
            constexpr_delete(ce);
            return;
        }
        if (ce->ivalue != (int) ce->ivalue)
        {
            // ISO: 6.7.2.2 (2)
            ADD_ERROR(enumr->enumr_expression, "enumeration constant value must be representable by type 'int'");
            constexpr_delete(ce);
            return;
        }
        symbol_init_initializer(sy);
        vector_add(sy->designations, NULL);
        vector_add(sy->initial_values, ce);
        return;
    }
    // otherwise...
    syntax_component_t* enums = enumr->parent;
    if (!enums)
        report_return;
    // find the last enumerator that does have one before the current enum constant
    int last = -1;
    int idx = 0;
    VECTOR_FOR(syntax_component_t*, er, enums->enums_enumerators)
    {
        idx = i;
        if (er == enumr)
            break;
        if (er->enumr_expression)
            last = i;
    }
    // if there is no last one, take 0 to be the constant value of the first enum constant (go by placement index)
    if (last == -1)
    {
        symbol_init_initializer(sy);
        vector_add(sy->designations, NULL);
        vector_add(sy->initial_values, ce_make_integer(make_basic_type(CTC_INT), idx));
        return;
    }
    // if there is, evaluate it
    syntax_component_t* last_er = vector_get(enums->enums_enumerators, last);
    constexpr_t* lce = ce_evaluate(last_er->enumr_expression, CE_INTEGER);
    if (!lce)
    {
        // ISO: 6.7.2.2 (2)
        ADD_ERROR(last_er->enumr_expression, "enumeration constant value must be specified by an integer constant expression");
        constexpr_delete(lce);
        return;
    }
    if (lce->ivalue != (int) lce->ivalue)
    {
        // ISO: 6.7.2.2 (2)
        ADD_ERROR(last_er->enumr_expression, "enumeration constant value must be representable by type 'int'");
        constexpr_delete(lce);
        return;
    }
    // make this enum constant have the value of the found enum constant + the distance between the two
    constexpr_t* ce = ce_make_integer(make_basic_type(CTC_INT), lce->ivalue + (idx - last));
    constexpr_delete(lce);
    if (ce->ivalue != (int) ce->ivalue)
    {
        // ISO: 6.7.2.2 (2)
        ADD_ERROR(enumr->enumr_expression, "enumeration constant value must be representable by type 'int'");
        constexpr_delete(ce);
        return;
    }
    symbol_init_initializer(sy);
    vector_add(sy->designations, NULL);
    vector_add(sy->initial_values, ce);
}

void analyze_declaring_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy, bool first, size_t dupes)
{
    if (sy && sy->declarer->parent && sy->declarer->parent->type == SC_ENUMERATOR)
        analyze_enumeration_constant_after(trav, syn, sy);
    linkage_t lk = symbol_get_linkage(sy);
    syntax_component_t* scope = symbol_get_scope(sy);
    if (lk == LK_NONE && dupes > 1)
        // TODO: something to think about here: tags...
        // ISO: 6.7 (3)
        ADD_ERROR(syn, "symbol with no linkage may not be declared twice with the same scope and namespace");
    if ((lk == LK_EXTERNAL || lk == LK_INTERNAL) && syntax_has_initializer(syn) && scope_is_block(scope))
        // ISO: 6.7.8 (5)
        ADD_ERROR(syn, "symbol declared with external or internal linkage at block scope may not be initialized");
    syntax_component_t* decl = NULL;
    if ((decl = syntax_get_declarator_declaration(syn)) &&
        scope_is_block(scope) &&
        sy->type->class == CTC_FUNCTION &&
        !syntax_has_specifier(decl->decl_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER, SCS_EXTERN) &&
        syntax_no_specifiers(decl->decl_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER) > 0)
        // NOTE: GCC on -std=c99 -pedantic-errors does not complain about typedef (which seems fine semantically, but appears to break this rule)
        // ISO: 6.7.1 (5)
        ADD_ERROR(syn, "function declarations at block scope may only have the 'extern' storage class specifier");
    if (syntax_is_tentative_definition(syn))
    {
        vector_t* declspecs = syntax_get_declspecs(syn);
        if (declspecs && syntax_has_specifier(declspecs, SC_STORAGE_CLASS_SPECIFIER, SCS_STATIC) &&
            !type_is_complete(sy->type))
        {
            // ISO: 6.9.2 (3)
            ADD_ERROR(syn, "tentative definitions with internal linkage may not have an incomplete type");
        }
    }
    if (sy->type->class == CTC_LABEL && !first && dupes > 1)
    {
        if (!scope) report_return;
        if (scope->type != SC_FUNCTION_DEFINITION) report_return;
        syntax_component_t* func_id = syntax_get_declarator_identifier(scope->fdef_declarator);
        if (!func_id) report_return;
        // ISO: 6.8.1 (3)
        ADD_ERROR(syn, "duplicate label name '%s' in function '%s'", syn->id, func_id->id);
    }
}

void analyze_designating_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy)
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

void analyze_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_namespace_t* ns = syntax_get_namespace(syn);
    if (!ns)
    {
        switch (syn->parent->type)
        {
            case SC_DESIGNATION:
                ADD_ERROR(syn, "cannot find member '%s' for designation", syn->id);
                break;
            case SC_MEMBER_EXPRESSION:
            case SC_DEREFERENCE_MEMBER_EXPRESSION:
                ADD_ERROR(syn, "struct has no member '%s'", syn->id);
                break;
            default:
                ADD_ERROR(syn, "could not determine name space of identifier '%s'", syn->id);
                break;
        }
        syn->ctype = make_basic_type(CTC_ERROR);
        return;
    }
    bool first = false;
    size_t count = 0;
    symbol_table_t* st = SYMBOL_TABLE;
    symbol_t* sy = symbol_table_count(st, syn, ns, &count, &first);
    namespace_delete(ns);
    if (!sy)
    {
        // ISO: 6.5.1 (2)
        ADD_ERROR(syn, "symbol '%s' is not defined in the given context", syn->id);
        syn->ctype = make_basic_type(CTC_ERROR);
        return;
    }
    if (sy->declarer == syn)
        analyze_declaring_identifier_after(trav, syn, sy, first, count);
    else
        analyze_designating_identifier_after(trav, syn, sy);
}

static void enforce_6_9_1_para_2(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* id = syntax_get_declarator_identifier(syn->fdef_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    if (sy->type->class == CTC_FUNCTION)
        return;
    // ISO: 6.9.1 (2)
    ADD_ERROR(syn, "declarator of function must be of function type");
}

static void enforce_6_9_1_para_3(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* id = syntax_get_declarator_identifier(syn->fdef_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    if (sy->type->class != CTC_FUNCTION)
        return; // this is handled in enforce_6_9_1_para_2
    c_type_t* ct = sy->type;
    if (ct->derived_from->class == CTC_VOID || (type_is_object_type(ct->derived_from) && ct->derived_from->class != CTC_ARRAY))
        return;
    // ISO: 6.9.1 (3)
    ADD_ERROR(syn, "function may only have a void or object (other than array) return type");
}

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

static void enforce_6_9_1_para_5(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* declr = syn->fdef_declarator;
    if (!declr) report_return;
    if (declr->type != SC_FUNCTION_DECLARATOR)
        return; // this is handled in enforce_6_9_1_para_2
    if (!declr->fdeclr_parameter_declarations)
        return;
    if (syn->fdef_knr_declarations && syn->fdef_knr_declarations->size > 0)
        // ISO: 6.9.1 (5)
        ADD_ERROR(syn, "declaration list in function definition not allowed if there is a parameter list");
    if (declr->fdeclr_parameter_declarations->size == 1)
    {
        syntax_component_t* pdecl = vector_get(declr->fdeclr_parameter_declarations, 0);
        if (!pdecl->pdecl_declr && pdecl->pdecl_declaration_specifiers->size == 1 &&
            syntax_has_specifier(pdecl->pdecl_declaration_specifiers, SC_BASIC_TYPE_SPECIFIER, BTS_VOID))
            // ISO: 6.9.1 (5)
            // note: special case for function definitions to have (void) in their declarator and nothing else
            return;
    }
    VECTOR_FOR(syntax_component_t*, pdecl, declr->fdeclr_parameter_declarations)
    {
        if (!syntax_get_declarator_identifier(pdecl->pdecl_declr))
        {
            // ISO: 6.9.1 (5)
            ADD_ERROR(syn, "all parameters in a function definition must have identifiers");
            break;
        }
    }
}

static void enforce_6_9_1_para_6(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* declr = syn->fdef_declarator;
    vector_t* knr_decls = syn->fdef_knr_declarations;
    if (!declr) report_return;
    if (declr->type != SC_FUNCTION_DECLARATOR)
        return; // this is handled in enforce_6_9_1_para_2
    if (!declr->fdeclr_knr_identifiers)
        return;
    unsigned found = 0;
    VECTOR_FOR(syntax_component_t*, knr_decl, knr_decls)
    {
        VECTOR_FOR(syntax_component_t*, declspec, knr_decl->decl_declaration_specifiers)
        {
            if (declspec->type == SC_STORAGE_CLASS_SPECIFIER && declspec->scs != SCS_REGISTER)
                // ISO: 6.9.1 (6)
                ADD_ERROR(declspec, "declarations in the function declaration list may only have the storage class specifier 'register'");
        }
        if (knr_decl->decl_init_declarators->size < 1)
        {
            // ISO: 6.9.1 (6)
            ADD_ERROR(knr_decl, "declarations in the function declaration list must include at least one declarator");
            continue;
        }
        VECTOR_FOR(syntax_component_t*, ideclr, knr_decl->decl_init_declarators)
        {
            if (ideclr->ideclr_initializer)
                // ISO: 6.9.1 (6)
                ADD_ERROR(ideclr->ideclr_initializer, "declarations in the function declaration list cannot have initializers");
            syntax_component_t* id = syntax_get_declarator_identifier(ideclr->ideclr_declarator);
            if (!id) report_return;
            if (vector_contains(declr->fdeclr_knr_identifiers, id, (int (*)(void*, void*)) strcmp) == -1)
                // ISO: 6.9.1 (6)
                ADD_ERROR(syn, "declaration of '%s' does not have a corresponding identifier in the identifier list", id->id);
            else 
                ++found;
        }
    }

    if (found != declr->fdeclr_knr_identifiers->size)
        // ISO: 6.9.1 (6)
        ADD_ERROR(syn, "each identifier must have exactly one declaration in the declaration list");
    
    // "An identifier declared as a typedef name shall not be redeclared as a parameter"
    // ^ this is another requirement specified by this paragraph, and i'm not exactly sure what it entails tbh
}

// doesn't enforced main to be defined (that's the linker's job)
// looks at the prototype (or lack thereof) of the function and determines whether it is valid or not
static void enforce_main_definition(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* id = syntax_get_declarator_identifier(syn->fdef_declarator);
    if (!id) report_return;
    if (strcmp(id->id, "main"))
        return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    if (sy->type->class != CTC_FUNCTION)
        return; // this is handled in enforce_6_9_1_para_2
    c_type_t* ct = sy->type;
    if (ct->derived_from->class != CTC_INT)
        ADD_ERROR(syn, "'main' should have an int return type");
    // check for (void), (int, char**), or (int, char*[]), or no prototype
    bool good_prototype = false;
    if (ct->function.param_types)
    {
        // enforce (void)
        if (ct->function.param_types->size == 1)
        {
            c_type_t* pt = vector_get(ct->function.param_types, 0);
            if (pt->class == CTC_VOID)
                good_prototype = true;
        }
        // enforce (int, char**) or (int, char*[])
        else if (ct->function.param_types->size == 2)
        {
            c_type_t* pt0 = vector_get(ct->function.param_types, 0);
            c_type_t* pt1 = vector_get(ct->function.param_types, 1);
            if (pt0->class == CTC_INT && ((pt1->class == CTC_POINTER || pt1->class == CTC_ARRAY) &&
                pt1->derived_from->class == CTC_POINTER &&
                pt1->derived_from->derived_from->class == CTC_CHAR))
                good_prototype = true;
        }
    }
    else
        good_prototype = true;
    if (!good_prototype)
        ADD_ERROR(syn, "function prototype for 'main', if any, should be either 'int main(void)' or 'int main(int argc, char *argv[])'");
}

void analyze_function_definition_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    enforce_6_9_para_2(trav, syn);
    enforce_6_9_1_para_2(trav, syn);
    enforce_6_9_1_para_3(trav, syn);
    enforce_6_9_1_para_4(trav, syn);
    enforce_6_9_1_para_5(trav, syn);
    enforce_6_9_1_para_6(trav, syn);
    enforce_main_definition(trav, syn);
}

void enforce_6_7_1_para_2(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool found = false;
    VECTOR_FOR(syntax_component_t*, declspec, syn->decl_declaration_specifiers)
    {
        if (declspec->type == SC_STORAGE_CLASS_SPECIFIER)
        {
            if (found)
            {
                // ISO: 6.7.1 (2)
                ADD_ERROR(syn, "only one storage class specifier allowed in declaration");
                break;
            }
            found = true;
        }
    }
}

void analyze_declaration_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    enforce_6_7_para_2(trav, syn);
    enforce_6_7_1_para_2(trav, syn);
    enforce_6_9_para_2(trav, syn);
}

void analyze_translation_unit_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    // TODO
}

void enforce_6_8_1_para_2(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (syn->lstmt_id)
        return; // this constraint does not apply to regular labels, only case/default
    
    syntax_component_t* parent = syn;
    for (; parent && parent->type != SC_SWITCH_STATEMENT; parent = parent->parent);
    if (!parent)
        // ISO: 6.8.1 (2)
        ADD_ERROR(syn, "case and default labels may only exist within a switch statement");
}

void analyze_labeled_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    enforce_6_8_1_para_2(trav, syn);
}

void analyze_if_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (!type_is_scalar(syn->ifstmt_condition->ctype))
        // ISO: 6.8.4.1 (1)
        ADD_ERROR(syn->ifstmt_condition, "controlling expression of an if statement must be of scalar type");
}

void analyze_switch_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (!type_is_integer(syn->swstmt_condition->ctype))
        // ISO: 6.8.4.2 (1)
        ADD_ERROR(syn->swstmt_condition, "controlling expression of a switch statement must be of integer type");
}

void analyze_iteration_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* controlling = NULL;
    switch (syn->type)
    {
        case SC_WHILE_STATEMENT:
            controlling = syn->whstmt_condition;
            break;
        case SC_DO_STATEMENT:
            controlling = syn->dostmt_condition;
            break;
        case SC_FOR_STATEMENT:
            controlling = syn->forstmt_condition;
            if (syn->forstmt_init && syn->forstmt_init->type == SC_DECLARATION)
            {
                syntax_component_t* decl = syn->forstmt_init;
                bool bad = false;
                VECTOR_FOR(syntax_component_t*, declspec, decl->decl_declaration_specifiers)
                {
                    if (declspec->type != SC_STORAGE_CLASS_SPECIFIER) continue;
                    if (declspec->scs == SCS_AUTO) continue;
                    if (declspec->scs == SCS_REGISTER) continue;
                    bad = true;
                    break;
                }
                if (bad)
                    // ISO: 6.8.5 (3)
                    ADD_ERROR(decl, "for loop initializing declaration may only have storage class specifiers of 'auto' or 'register'");
            }
            break;
        default:
            report_return;
    }
    if (controlling && !type_is_scalar(controlling->ctype))
        // ISO: 6.8.5 (2)
        ADD_ERROR(controlling, "controlling expression of a loop must be of scalar type");
}

void analyze_continue_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* loop = syn;
    for (; loop && loop->type != SC_FOR_STATEMENT && loop->type != SC_WHILE_STATEMENT && loop->type != SC_DO_STATEMENT; loop = loop->parent);
    if (!loop)
        // ISO: 6.8.6.2 (1)
        ADD_ERROR(syn, "continue statements are only allowed within loops");
}

void analyze_break_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* parent = syn;
    for (; parent &&
        parent->type != SC_FOR_STATEMENT &&
        parent->type != SC_WHILE_STATEMENT &&
        parent->type != SC_DO_STATEMENT &&
        parent->type != SC_SWITCH_STATEMENT; parent = parent->parent);
    if (!parent)
        // ISO: 6.8.6.3 (1)
        ADD_ERROR(syn, "break statements are only allowed within loops and switch statements");
}

void analyze_return_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* fdef = syntax_get_function_definition(syn);
    if (!fdef) report_return;
    syntax_component_t* id = syntax_get_declarator_identifier(fdef->fdef_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    if (!sy->type) report_return;
    if (sy->type->derived_from->class == CTC_VOID && syn->retstmt_expression)
        // ISO: 6.8.6.4 (1)
        ADD_ERROR(syn, "return values are not allowed for return statements if their function has a void return type");
    if (sy->type->derived_from->class != CTC_VOID && !syn->retstmt_expression)
        // ISO: 6.8.6.4 (1)
        ADD_ERROR(syn, "return values are required for return statements if their function has a non-void return type");
}

void analyze_static_initializer_list_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy, designation_t* prefix)
{
    symbol_init_initializer(sy);
    VECTOR_FOR(syntax_component_t*, d, syn->inlist_designations)
    {
        syntax_component_t* init = vector_get(syn->inlist_initializers, i);
        if (init->type != SC_INITIALIZER_LIST)
        {
            constexpr_t* ce = ce_evaluate(init, CE_ANY);
            if (!ce)
            {
                // ISO: 6.7.8 (4)
                ADD_ERROR(init, "initializer for object with static storage duration must be constant");
                return;
            }
            designation_t* suffix = syntax_to_designation(d);
            designation_t* desig = designation_concat(prefix, suffix);
            designation_delete_all(suffix);
            vector_add(sy->designations, desig);
            vector_add(sy->initial_values, ce);
            continue;
        }
        designation_t* desig = syntax_to_designation(d);
        analyze_static_initializer_list_after(trav, init, sy, desig);
        designation_delete_all(desig);
    }
}

void analyze_init_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* init = syn->ideclr_initializer;
    if (!init) return;
    syntax_component_t* id = syntax_get_declarator_identifier(syn->ideclr_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    storage_duration_t sd = symbol_get_storage_duration(sy);
    if (sd != SD_STATIC) return;
    if (init->type != SC_INITIALIZER_LIST)
    {
        constexpr_t* ce = ce_evaluate(init, CE_ANY);
        if (!ce)
        {
            // ISO: 6.7.8 (4)
            ADD_ERROR(init, "initializer for object with static storage duration must be constant");
            return;
        }
        symbol_init_initializer(sy);
        vector_add(sy->designations, NULL);
        vector_add(sy->initial_values, ce);
        return;
    }
    analyze_static_initializer_list_after(trav, init, sy, NULL);
}

void analyze_complete_struct_union_specifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    unsigned count = 0;
    VECTOR_FOR(syntax_component_t*, sdecl, syn->sus_declarations)
    {
        int j = i;
        count += sdecl->sdecl_declarators->size;
        VECTOR_FOR(syntax_component_t*, sdeclr, sdecl->sdecl_declarators)
        {
            if (!sdeclr) continue;
            syntax_component_t* id = syntax_get_declarator_identifier(sdeclr);
            if (!id) report_return;
            symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
            if (!sy) report_return;
            bool complete = type_is_complete(sy->type);
            bool flexible = !complete && sy->type->class == CTC_ARRAY && j == syn->sus_declarations->size - 1 &&
                i == sdecl->sdecl_declarators->size - 1;
            if (!complete && !flexible)
            {
                // ISO: 6.7.2.1 (2)
                if (sy->type->class == CTC_ARRAY)
                    ADD_ERROR(sdeclr, "flexible array members are only allowed at the end of a struct or union");
                else
                    ADD_ERROR(sdeclr, "incomplete types are not allowed within structs and unions");
            }
            if (flexible && syntax_get_enclosing(syn->parent, SC_STRUCT_UNION_SPECIFIER))
                // ISO: 6.7.2.1 (2)
                ADD_ERROR(sdeclr, "flexible array members are not permitted at the end of nested structs and unions");
            if (flexible && count == 1)
                // ISO: 6.7.2.1 (2)
                ADD_ERROR(sdeclr, "flexible array members cannot be a part of an otherwise empty struct or union");
        }
    }
}

void analyze_struct_union_specifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (syn->sus_declarations)
        analyze_complete_struct_union_specifier_after(trav, syn);
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
    trav->before[SC_COMPOUND_LITERAL] = analyze_compound_literal_expression_before;
    trav->after[SC_COMPOUND_LITERAL] = analyze_compound_literal_expression_after;
    trav->after[SC_MEMBER_EXPRESSION] = analyze_member_expression_after;
    trav->after[SC_DEREFERENCE_MEMBER_EXPRESSION] = analyze_dereference_member_expression_after;
    trav->after[SC_FUNCTION_CALL_EXPRESSION] = analyze_function_call_expression_after;
    trav->after[SC_SUBSCRIPT_EXPRESSION] = analyze_subscript_expression_after;
    trav->after[SC_IDENTIFIER] = analyze_identifier_after;

    // statements
    trav->after[SC_LABELED_STATEMENT] = analyze_labeled_statement_after;
    trav->after[SC_IF_STATEMENT] = analyze_if_statement_after;
    trav->after[SC_FOR_STATEMENT] = analyze_iteration_statement_after;
    trav->after[SC_DO_STATEMENT] = analyze_iteration_statement_after;
    trav->after[SC_WHILE_STATEMENT] = analyze_iteration_statement_after;
    trav->after[SC_CONTINUE_STATEMENT] = analyze_continue_statement_after;
    trav->after[SC_RETURN_STATEMENT] = analyze_return_statement_after;

    // declarations
    trav->after[SC_INIT_DECLARATOR] = analyze_init_declarator_after;
    trav->after[SC_STRUCT_UNION_SPECIFIER] = analyze_struct_union_specifier_after;

    traverse(trav);
    analysis_error_t* errors = ANALYSIS_TRAVERSER->errors;
    traverse_delete(trav);
    return errors;
}