// this file contains some utility functions for working with the behemoth that is syntax_component_t.

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "ecc.h"

bool syntax_has_specifier(vector_t* specifiers, syntax_component_type_t sc_type, int type)
{
    if (!specifiers) return false;
    for (unsigned i = 0; i < specifiers->size; ++i)
    {
        syntax_component_t* specifier = vector_get(specifiers, i);
        if (specifier->type != sc_type) continue;
        switch (sc_type)
        {
            case SC_BASIC_TYPE_SPECIFIER:
                if (type == specifier->bts)
                    return true;
                break;
            case SC_FUNCTION_SPECIFIER:
                if (type == specifier->fs)
                    return true;
                break;
            case SC_TYPE_QUALIFIER:
                if (type == specifier->tq)
                    return true;
                break;
            case SC_STORAGE_CLASS_SPECIFIER:
                if (type == specifier->scs)
                    return true;
                break;
            default:
                return false;
        }
    }
    return false;
}

size_t syntax_no_specifiers(vector_t* declspecs, syntax_component_type_t type)
{
    size_t j = 0;
    VECTOR_FOR(syntax_component_t*, declspec, declspecs)
    {
        if (declspec->type != type) continue;
        ++j;
    }
    return j;
}

// SC_STATEMENT = SC_LABELED_STATEMENT | SC_COMPOUND_STATEMENT | SC_EXPRESSION_STATEMENT | SC_SELECTION_STATEMENT | SC_ITERATION_STATEMENT | SC_JUMP_STATEMENT
// SC_SELECTION_STATEMENT = SC_IF_STATEMENT | SC_SWITCH_STATEMENT
// SC_ITERATION_STATEMENT = SC_DO_STATEMENT | SC_WHILE_STATEMENT | SC_FOR_STATEMENT
// SC_JUMP_STATEMENT = SC_GOTO_STATEMENT | SC_CONTINUE_STATEMENT | SC_BREAK_STATEMENT | SC_RETURN_STATEMENT
bool syntax_is_statement_type(syntax_component_type_t type)
{
    return type == SC_LABELED_STATEMENT ||
        type == SC_COMPOUND_STATEMENT ||
        type == SC_EXPRESSION_STATEMENT  ||
        type == SC_IF_STATEMENT ||
        type == SC_SWITCH_STATEMENT ||
        type == SC_DO_STATEMENT ||
        type == SC_WHILE_STATEMENT ||
        type == SC_FOR_STATEMENT ||
        type == SC_GOTO_STATEMENT ||
        type == SC_CONTINUE_STATEMENT ||
        type == SC_BREAK_STATEMENT ||
        type == SC_RETURN_STATEMENT;
}

bool syntax_is_expression_type(syntax_component_type_t type)
{
    return type == SC_EXPRESSION ||
        type == SC_ASSIGNMENT_EXPRESSION ||
        type == SC_LOGICAL_OR_EXPRESSION ||
        type == SC_LOGICAL_AND_EXPRESSION ||
        type == SC_BITWISE_OR_EXPRESSION ||
        type == SC_BITWISE_XOR_EXPRESSION ||
        type == SC_BITWISE_AND_EXPRESSION ||
        type == SC_EQUALITY_EXPRESSION ||
        type == SC_INEQUALITY_EXPRESSION ||
        type == SC_GREATER_EQUAL_EXPRESSION ||
        type == SC_LESS_EQUAL_EXPRESSION ||
        type == SC_GREATER_EXPRESSION ||
        type == SC_LESS_EXPRESSION ||
        type == SC_BITWISE_RIGHT_EXPRESSION ||
        type == SC_BITWISE_LEFT_EXPRESSION ||
        type == SC_SUBTRACTION_EXPRESSION ||
        type == SC_ADDITION_EXPRESSION ||
        type == SC_MODULAR_EXPRESSION ||
        type == SC_DIVISION_EXPRESSION ||
        type == SC_MULTIPLICATION_EXPRESSION ||
        type == SC_CONDITIONAL_EXPRESSION ||
        type == SC_CAST_EXPRESSION ||
        type == SC_PREFIX_INCREMENT_EXPRESSION ||
        type == SC_PREFIX_DECREMENT_EXPRESSION ||
        type == SC_REFERENCE_EXPRESSION ||
        type == SC_DEREFERENCE_EXPRESSION ||
        type == SC_PLUS_EXPRESSION ||
        type == SC_MINUS_EXPRESSION ||
        type == SC_COMPLEMENT_EXPRESSION ||
        type == SC_NOT_EXPRESSION ||
        type == SC_SIZEOF_EXPRESSION ||
        type == SC_SIZEOF_TYPE_EXPRESSION ||
        type == SC_COMPOUND_LITERAL ||
        type == SC_POSTFIX_INCREMENT_EXPRESSION ||
        type == SC_POSTFIX_DECREMENT_EXPRESSION ||
        type == SC_DEREFERENCE_MEMBER_EXPRESSION ||
        type == SC_MEMBER_EXPRESSION ||
        type == SC_FUNCTION_CALL_EXPRESSION ||
        type == SC_SUBSCRIPT_EXPRESSION ||
        type == SC_CONSTANT_EXPRESSION ||
        type == SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION ||
        type == SC_DIVISION_ASSIGNMENT_EXPRESSION ||
        type == SC_MODULAR_ASSIGNMENT_EXPRESSION ||
        type == SC_ADDITION_ASSIGNMENT_EXPRESSION ||
        type == SC_SUBTRACTION_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_AND_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_OR_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION ||
        type == SC_PRIMARY_EXPRESSION_IDENTIFIER ||
        type == SC_INTEGER_CONSTANT ||
        type == SC_FLOATING_CONSTANT ||
        type == SC_CHARACTER_CONSTANT ||
        type == SC_ENUMERATION_CONSTANT ||
        type == SC_STRING_LITERAL;
}

bool syntax_is_concrete_declarator_type(syntax_component_type_t type)
{
    return type == SC_DECLARATOR_IDENTIFIER ||
        type == SC_DECLARATOR ||
        type == SC_ARRAY_DECLARATOR ||
        type == SC_FUNCTION_DECLARATOR ||
        type == SC_INIT_DECLARATOR ||
        type == SC_STRUCT_DECLARATOR;
}

bool syntax_is_abstract_declarator_type(syntax_component_type_t type)
{
    return type == SC_ABSTRACT_ARRAY_DECLARATOR ||
        type == SC_ABSTRACT_DECLARATOR ||
        type == SC_ABSTRACT_FUNCTION_DECLARATOR;
}

bool syntax_is_declarator_type(syntax_component_type_t type)
{
    return syntax_is_abstract_declarator_type(type) ||
        syntax_is_concrete_declarator_type(type);
}

bool syntax_is_declaration_type(syntax_component_type_t type)
{
    return type == SC_DECLARATION ||
        type == SC_STRUCT_DECLARATION ||
        type == SC_PARAMETER_DECLARATION;
}

// SC_DIRECT_DECLARATOR = SC_IDENTIFIER | SC_DECLARATOR | SC_ARRAY_DECLARATOR | SC_FUNCTION_DECLARATOR

// supports the following declarator types: SC_INIT_DECLARATOR, SC_STRUCT_DECLARATOR, SC_DECLARATOR, SC_DIRECT_DECLARATOR
// no abstract declarators b/c those don't have identifiers silly!!!
syntax_component_t* syntax_get_declarator_identifier(syntax_component_t* declr)
{
    if (!declr)
        return NULL;
    if (declr->type == SC_INIT_DECLARATOR)
        return syntax_get_declarator_identifier(declr->ideclr_declarator);
    if (declr->type == SC_STRUCT_DECLARATOR)
        return syntax_get_declarator_identifier(declr->sdeclr_declarator);
    if (declr->type == SC_DECLARATOR)
        return syntax_get_declarator_identifier(declr->declr_direct);
    if (declr->type == SC_ARRAY_DECLARATOR)
        return syntax_get_declarator_identifier(declr->adeclr_direct);
    if (declr->type == SC_FUNCTION_DECLARATOR)
        return syntax_get_declarator_identifier(declr->fdeclr_direct);
    if (declr->type == SC_DECLARATOR_IDENTIFIER)
        return declr;
    return NULL;
}

syntax_component_t* syntax_get_declarator_function_definition(syntax_component_t* declr)
{
    for (; declr && syntax_is_concrete_declarator_type(declr->type); declr = declr->parent);
    if (!declr)
        return NULL;
    return declr->type == SC_FUNCTION_DEFINITION ? declr : NULL;
}

syntax_component_t* syntax_get_declarator_declaration(syntax_component_t* declr)
{
    for (; declr && syntax_is_concrete_declarator_type(declr->type); declr = declr->parent);
    if (!declr)
        return NULL;
    return syntax_is_declaration_type(declr->type) ? declr : NULL;
}

// a full declarator is a declarator that is not a part of another declarator
// so for example, '*a' has two declarators: one that is an identifier and one that has the ptr
// the one with the ptr is the full declarator
// as another, '(*)(void)' has two declarators: one that is an abstract declarator with a ptr and the abs. function declarator
// the abs. function declarator is the full declarator
// will return itself if it is the full declarator
syntax_component_t* syntax_get_full_declarator(syntax_component_t* declr)
{
    if (!declr)
        return NULL;
    for (; declr->parent && syntax_is_declarator_type(declr->parent->type); declr = declr->parent);
    return declr;
}

syntax_component_t* syntax_get_translation_unit(syntax_component_t* syn)
{
    for (; syn && syn->type != SC_TRANSLATION_UNIT; syn = syn->parent);
    return syn;
}

symbol_table_t* syntax_get_symbol_table(syntax_component_t* syn)
{
    syntax_component_t* tlu = syntax_get_translation_unit(syn);
    if (!tlu) return NULL;
    return tlu->tlu_st;
}

syntax_component_t* syntax_get_enclosing(syntax_component_t* syn, syntax_component_type_t enclosing)
{
    for (; syn && syn->type != enclosing; syn = syn->parent);
    return syn;
}

syntax_component_t* syntax_get_function_definition(syntax_component_t* syn)
{
    return syntax_get_enclosing(syn, SC_FUNCTION_DEFINITION);
}

bool syntax_is_assignment_expression(syntax_component_type_t type)
{
    return type == SC_ASSIGNMENT_EXPRESSION ||
        type == SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION ||
        type == SC_DIVISION_ASSIGNMENT_EXPRESSION ||
        type == SC_MODULAR_ASSIGNMENT_EXPRESSION ||
        type == SC_ADDITION_ASSIGNMENT_EXPRESSION ||
        type == SC_SUBTRACTION_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_AND_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_OR_ASSIGNMENT_EXPRESSION ||
        type == SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION;
}

bool syntax_is_identifier(syntax_component_type_t type)
{
    return type == SC_IDENTIFIER ||
        type == SC_PRIMARY_EXPRESSION_IDENTIFIER ||
        type == SC_ENUMERATION_CONSTANT ||
        type == SC_DECLARATOR_IDENTIFIER ||
        type == SC_TYPEDEF_NAME;
}

void namespace_delete(c_namespace_t* ns)
{
    if (!ns) return;
    if (ns->struct_union_type) type_delete(ns->struct_union_type);
    free(ns);
}

bool namespace_equals(c_namespace_t* ns1, c_namespace_t* ns2)
{
    if (!ns1 && !ns2) return true;
    if (!ns1 || !ns2) return false;
    if (ns1->class != ns2->class) return false;
    if (!type_is_compatible(ns1->struct_union_type, ns2->struct_union_type)) return false;
    return true;
}

void namespace_print(c_namespace_t* ns, int (*printer)(const char* fmt, ...))
{
    if (!ns)
    {
        printer("(null)");
        return;
    }
    printer("namespace { class: %s", C_NAMESPACE_CLASS_NAMES[ns->class]);
    if (ns->struct_union_type)
    {
        printer(", struct/union type: ");
        type_humanized_print(ns->struct_union_type, printer);
    }
    printer(" }");
}

// gets the type that contains the object being designated using a base, tracing type
// e.g., designation .x[0] will get x's type (an array type)
// e.g., designation .y.a will get y's type (a struct/union type)
// { .x = { .a = 3 } } find .x.a aggregate type
static c_type_t* get_type_by_designation(syntax_component_t* desig, syntax_component_t* term, bool parent)
{
    // get parent components (initializer list and whatever is containing that initializer list)
    syntax_component_t* inlist = desig->parent;
    if (!inlist || inlist->type != SC_INITIALIZER_LIST) report_return_value(NULL);
    syntax_component_t* enclosing = inlist->parent;
    if (!enclosing) report_return_value(NULL);

    // pass thru additional initializer list levels

    // find the starting type dependent on the enclosing type
    c_type_t* trace = NULL;
    // if the enclosing syntax element is a compound literal (like: (struct point) { .x = 3 }), get the parenthesized type
    if (enclosing->type == SC_COMPOUND_LITERAL)
        trace = create_type(enclosing->cl_type_name, enclosing->cl_type_name->tn_declarator);
    // if the enclosing syntax element is an init declarator (like: p = { .x = 3 }), get the type by its symbol
    else if (enclosing->type == SC_INIT_DECLARATOR)
    {
        syntax_component_t* ideclr_id = syntax_get_declarator_identifier(enclosing->ideclr_declarator);
        if (!ideclr_id) report_return_value(NULL);
        symbol_t* ideclr_sy = symbol_table_get_syn_id(syntax_get_symbol_table(ideclr_id), ideclr_id);
        if (!ideclr_sy) return NULL;
        trace = type_copy(ideclr_sy->type);
    }
    // if the enclosing syntax element is an initializer list, find the aggregate type of its designation
    else if (enclosing->type == SC_INITIALIZER_LIST)
    {
        VECTOR_FOR(syntax_component_t*, d, enclosing->inlist_designations)
        {
            syntax_component_t* hinlist = vector_get(enclosing->inlist_initializers, i);
            if (inlist != hinlist)
                continue;
            trace = get_type_by_designation(d, NULL, false);
            break;
        }
    }
    else
        report_return_value(NULL);
    if (!trace) return NULL;

    c_type_t* top = trace;
    VECTOR_FOR(syntax_component_t*, designator, desig->desig_designators)
    {
        if (parent && term && designator == term)
            break;
        if (syntax_is_identifier(designator->type))
        {
            if (trace->class != CTC_STRUCTURE && trace->class != CTC_UNION)
                return NULL;
            c_type_t* old = trace;
            VECTOR_FOR(char*, member_name, trace->struct_union.member_names)
            {
                if (!streq(designator->id, member_name))
                    continue;
                trace = vector_get(trace->struct_union.member_types, i);
                break;
            }
            if (old == trace)
                return NULL;
        }
        else
        {
            if (trace->class != CTC_ARRAY)
                return NULL;
            trace = trace->derived_from;
        }
        if (!parent && term && designator == term)
            break;
    }
    c_type_t* ct = type_copy(trace);
    type_delete(top);
    return ct;
}

static c_type_t* get_aggregate_type_by_designation(syntax_component_t* desig, syntax_component_t* term)
{
    return get_type_by_designation(desig, term, true);
}

// finds the namespace to look in based on the contextualized id
// allocates space for the c_namespace_t object
c_namespace_t* syntax_get_namespace(syntax_component_t* id)
{
    if (!id) return NULL;
    syntax_component_t* syn = id->parent;
    if (!syn) return NULL;
    if (syn->type == SC_GOTO_STATEMENT)
        return make_basic_namespace(NSC_LABEL);
    if (syn->type == SC_STRUCT_UNION_SPECIFIER)
    {
        switch (syn->sus_sou)
        {
            case SOU_STRUCT: return make_basic_namespace(NSC_STRUCT);
            case SOU_UNION: return make_basic_namespace(NSC_UNION);
        }
        return NULL;
    }
    if (syn->type == SC_ENUM_SPECIFIER)
        return make_basic_namespace(NSC_ENUM);
    if (syn->memexpr_id == id && (syn->type == SC_MEMBER_EXPRESSION ||
        syn->type == SC_DEREFERENCE_MEMBER_EXPRESSION))
    {
        c_type_t* ty = syn->memexpr_expression->ctype;
        if (syn->type == SC_DEREFERENCE_MEMBER_EXPRESSION)
            ty = ty ? ty->derived_from : NULL;
        if (!ty || (ty->class != CTC_STRUCTURE && ty->class != CTC_UNION))
            return NULL;
        c_namespace_t* ns = calloc(1, sizeof *ns);
        ns->struct_union_type = type_copy(ty);
        switch (ty->class)
        {
            case CTC_STRUCTURE: ns->class = NSC_STRUCT_MEMBER; break;
            case CTC_UNION: ns->class = NSC_UNION_MEMBER; break;
            default:
            {
                namespace_delete(ns);
                return NULL;
            }
        }
        return ns;
    }
    if (syn->type == SC_DESIGNATION)
    {
        // get the type of the identifier by stepping through its designation from the root type
        c_type_t* ct = get_aggregate_type_by_designation(syn, id);
        if (!ct) return NULL;

        // return a namespace with that struct type
        c_namespace_t* ns = calloc(1, sizeof *ns);
        ns->class = ct->class == CTC_STRUCTURE ? NSC_STRUCT_MEMBER : NSC_UNION_MEMBER;
        ns->struct_union_type = ct;
        return ns;
    }
    symbol_t* sy = symbol_table_get_syn_id(syntax_get_symbol_table(id), id);
    if (!sy)
        return make_basic_namespace(NSC_ORDINARY);
    c_namespace_t* ns = calloc(1, sizeof *ns);
    ns->class = sy->ns->class;
    if (sy->ns->struct_union_type)
        ns->struct_union_type = type_copy(sy->ns->struct_union_type);
    return ns;
}

c_namespace_t get_basic_namespace(c_namespace_class_t nsc)
{
    return (c_namespace_t) { .class = nsc, .struct_union_type = NULL };
}

bool syntax_is_lvalue(syntax_component_t* syn)
{
    if (!syn) return false;
    if (!syn->ctype) return false;
    // ISO: 6.3.2.1 (1)
    return syntax_is_expression_type(syn->type) &&
        (type_is_object_type(syn->ctype) ||
        (!type_is_complete(syn->ctype) && syn->ctype->class != CTC_VOID));
}

bool can_evaluate(syntax_component_t* expr, constexpr_type_t ce_type)
{
    c_type_class_t c = CTC_ERROR;
    evaluate_constant_expression(expr, &c, ce_type);
    return c != CTC_ERROR;
}

bool syntax_is_known_size_array(syntax_component_t* declr)
{
    if (!declr) return false;
    syntax_component_t* length_expr = NULL;
    switch (declr->type)
    {
        case SC_ARRAY_DECLARATOR: length_expr = declr->adeclr_length_expression; break;
        case SC_ABSTRACT_ARRAY_DECLARATOR: length_expr = declr->abadeclr_length_expression; break;
        default: return false;
    }
    if (!length_expr) 
        return false;
    return can_evaluate(length_expr, CE_INTEGER);
}

// if this function returns false it DOES NOT necessarily mean that
// the array is of known size! use the other function fool
bool syntax_is_vla(syntax_component_t* declr)
{
    if (!declr) return false;
    syntax_component_t* length_expr = NULL;
    if (declr->type == SC_ARRAY_DECLARATOR)
    {
        if (declr->adeclr_unspecified_size)
            return true;
        length_expr = declr->adeclr_length_expression;
    }
    if (declr->type == SC_ABSTRACT_ARRAY_DECLARATOR)
    {
        if (declr->abadeclr_unspecified_size)
            return true;
        length_expr = declr->abadeclr_length_expression;
    }
    if (!length_expr)
        return false;
    return !can_evaluate(length_expr, CE_INTEGER);
}

// "if it is a structure or union, does not have any member (including,
// recursively, any member or element of all contained aggregates or unions)
// with a const-qualified type."
static bool compliance_6_3_2_1_para1_struct_clause(c_type_t* ct)
{
    if (!ct)
        return false;
    
    if (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION)
        return true;

    // shouldn't happen but safety check nonetheless against incomplete struct/union types
    if (!ct->struct_union.member_names || !ct->struct_union.member_types)
        return false;
    
    for (unsigned int i = 0; i < ct->struct_union.member_names->size; ++i)
    {
        c_type_t* mct = vector_get(ct->struct_union.member_types, i);
        if (mct->qualifiers & TQ_B_CONST)
            return false;
        if (mct->class == CTC_STRUCTURE || mct->class == CTC_UNION)
            if (!compliance_6_3_2_1_para1_struct_clause(mct))
                return false;
    }

    return true;
}

bool syntax_is_modifiable_lvalue(syntax_component_t* syn)
{
    if (!syn) return false;
    if (!syn->ctype) return false;
    if (!syntax_is_lvalue(syn))
        return false;
    if (syn->ctype->class == CTC_ARRAY)
        return false;
    if (!type_is_complete(syn->ctype))
        return false;
    if (syn->ctype->qualifiers & TQ_B_CONST)
        return false;
    if (!compliance_6_3_2_1_para1_struct_clause(syn->ctype))
        return false;

    // ISO: 6.3.2.1 (1)
    return true;
}

/*

in the following conditions, the lvalue expression x must acquire an address:
  x in &x
  x in x++
  x in x--
  x in x.m
  x in x = y
  x in x += y (et. al. compound assignment operators)

*/
bool syntax_is_in_lvalue_context(syntax_component_t* syn)
{
    if (!syn) return false;
    if (!syn->parent) return false;
    syntax_component_type_t type = syn->parent->type;
    return type == SC_REFERENCE_EXPRESSION ||
        type == SC_PREFIX_INCREMENT_EXPRESSION ||
        type == SC_PREFIX_DECREMENT_EXPRESSION ||
        type == SC_POSTFIX_INCREMENT_EXPRESSION ||
        type == SC_POSTFIX_DECREMENT_EXPRESSION ||
        (type == SC_MEMBER_EXPRESSION && syn->parent->memexpr_expression == syn) ||
        (syntax_is_assignment_expression(type) && syn->parent->bexpr_lhs == syn) ||
        (type == SC_INIT_DECLARATOR && syn->parent->ideclr_declarator == syn);
}

// using a declaring identifier, get its declaration specifiers, if any
vector_t* syntax_get_declspecs(syntax_component_t* id)
{
    if (!id) return NULL;
    syntax_component_t* full_declr = syntax_get_full_declarator(id);
    if (!full_declr) return NULL;
    syntax_component_t* decl = full_declr->parent;
    if (decl && decl->type == SC_FUNCTION_DEFINITION)
        return decl->fdef_declaration_specifiers;
    if (!syntax_is_declaration_type(decl->type))
        return NULL;
    vector_t* declspecs;
    switch (decl->type)
    {
        case SC_DECLARATION: declspecs = decl->decl_declaration_specifiers; break;
        case SC_STRUCT_DECLARATION: declspecs = decl->sdecl_specifier_qualifier_list; break;
        case SC_PARAMETER_DECLARATION: declspecs = decl->pdecl_declaration_specifiers; break;
        default: return NULL;
    }
    return declspecs;
}

// identifier must be a declaring identifier
bool syntax_is_typedef_name(syntax_component_t* id)
{
    if (!id) return false;
    vector_t* declspecs = syntax_get_declspecs(id);
    if (!declspecs) return false;
    return syntax_has_specifier(declspecs, SC_STORAGE_CLASS_SPECIFIER, SCS_TYPEDEF);
}

bool syntax_is_tentative_definition(syntax_component_t* id)
{
    syntax_component_t* tlu = syntax_get_translation_unit(id);
    if (!tlu)
        return false;
    symbol_t* sy = symbol_table_get_syn_id(tlu->tlu_st, id);
    if (!sy)
        return false;
    syntax_component_t* ideclr = syntax_get_full_declarator(id);
    if (!ideclr)
        return false;
    if (ideclr->type != SC_INIT_DECLARATOR)
        return false;
    if (ideclr->ideclr_initializer)
        return false;
    syntax_component_t* scope = symbol_get_scope(sy);
    if (!scope || scope->type != SC_TRANSLATION_UNIT)
        return false;
    syntax_component_t* decl = ideclr->parent;
    if (!decl || decl->type != SC_DECLARATION)
        return false;
    VECTOR_FOR(syntax_component_t*, declspec, decl->decl_declaration_specifiers)
    {
        if (declspec->type == SC_STORAGE_CLASS_SPECIFIER && declspec->scs != SCS_STATIC)
            return false;
    }
    return true;
}

bool syntax_has_initializer(syntax_component_t* id)
{
    syntax_component_t* tlu = syntax_get_translation_unit(id);
    if (!tlu)
        return false;
    symbol_t* sy = symbol_table_get_syn_id(tlu->tlu_st, id);
    if (!sy)
        return false;
    syntax_component_t* ideclr = syntax_get_full_declarator(id);
    if (!ideclr)
        return false;
    if (ideclr->type != SC_INIT_DECLARATOR)
        return false;
    if (!ideclr->ideclr_initializer)
        return false;
    return true;
}

typedef struct contains_traverser
{
    syntax_traverser_t base;
    syntax_component_type_t type;
    bool found;
} contains_traverser_t;

void contains_default_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    contains_traverser_t* ctrav = (contains_traverser_t*) trav;
    if (syn->type == ctrav->type)
        ctrav->found = true;
}

bool syntax_contains_subelement(syntax_component_t* syn, syntax_component_type_t type)
{
    contains_traverser_t* trav = (contains_traverser_t*) traverse_init(syn, sizeof(contains_traverser_t));
    trav->type = type;
    trav->found = false;

    traverse((syntax_traverser_t*) trav);

    bool found = trav->found;
    traverse_delete((syntax_traverser_t*) trav);
    return found;
}

// ps - print structure
// pf - print field
#define ps(fmt, ...) { for (unsigned i = 0; i < indent; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define pf(fmt, ...) { for (unsigned i = 0; i < indent + 1; ++i) printer("  "); printer(fmt, ##__VA_ARGS__); }
#define next_indent (indent + 2)
#define sty(x) "[" x "]"

static void print_syntax_indented(syntax_component_t* s, unsigned indent, int (*printer)(const char* fmt, ...));

static void print_vector_indented(vector_t* v, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!v)
    {
        ps("null\n");
        return;
    }
    if (!v->size)
    {
        ps("[\n\n");
        ps("]\n");
        return;
    }
    ps("[\n");
    for (unsigned i = 0; i < v->size; ++i)
    {
        syntax_component_t* s = vector_get(v, i);
        print_syntax_indented(s, indent + 1, printer);
    }
    ps("]\n");
}

    // SC_ENUM_SPECIFIER,
    // SC_ENUMERATOR,
    // SC_ARRAY_DECLARATOR,
    // SC_FUNCTION_DECLARATOR,
    // SC_PARAMETER_DECLARATION,
    // SC_ABSTRACT_DECLARATOR,
    // SC_ABSTRACT_ARRAY_DECLARATOR,
    // SC_ABSTRACT_FUNCTION_DECLARATOR,
    // SC_LABELED_STATEMENT,
    // SC_COMPOUND_STATEMENT,
    // SC_EXPRESSION_STATEMENT,
    // SC_IF_STATEMENT,
    // SC_SWITCH_STATEMENT,
    // SC_DO_STATEMENT,
    // SC_WHILE_STATEMENT,
    // SC_FOR_STATEMENT,
    // SC_GOTO_STATEMENT,
    // SC_CONTINUE_STATEMENT,
    // SC_BREAK_STATEMENT,
    // SC_RETURN_STATEMENT,
    // SC_INITIALIZER_LIST,
    // SC_DESIGNATION,
    // SC_EXPRESSION,
    // SC_ASSIGNMENT_EXPRESSION,
    // SC_LOGICAL_OR_EXPRESSION,
    // SC_LOGICAL_AND_EXPRESSION,
    // SC_BITWISE_OR_EXPRESSION,
    // SC_BITWISE_XOR_EXPRESSION,
    // SC_BITWISE_AND_EXPRESSION,
    // SC_EQUALITY_EXPRESSION,
    // SC_INEQUALITY_EXPRESSION,
    // SC_GREATER_EQUAL_EXPRESSION,
    // SC_LESS_EQUAL_EXPRESSION,
    // SC_GREATER_EXPRESSION,
    // SC_LESS_EXPRESSION,
    // SC_BITWISE_RIGHT_EXPRESSION,
    // SC_BITWISE_LEFT_EXPRESSION,
    // SC_SUBTRACTION_EXPRESSION,
    // SC_ADDITION_EXPRESSION,
    // SC_MODULAR_EXPRESSION,
    // SC_DIVISION_EXPRESSION,
    // SC_MULTIPLICATION_EXPRESSION,
    // SC_CONDITIONAL_EXPRESSION,
    // SC_CAST_EXPRESSION,
    // SC_PREFIX_INCREMENT_EXPRESSION,
    // SC_PREFIX_DECREMENT_EXPRESSION,
    // SC_REFERENCE_EXPRESSION,
    // SC_DEREFERENCE_EXPRESSION,
    // SC_PLUS_EXPRESSION,
    // SC_MINUS_EXPRESSION,
    // SC_COMPLEMENT_EXPRESSION,
    // SC_NOT_EXPRESSION,
    // SC_SIZEOF_EXPRESSION,
    // SC_SIZEOF_TYPE_EXPRESSION,
    // SC_COMPOUND_LITERAL,
    // SC_POSTFIX_INCREMENT_EXPRESSION,
    // SC_POSTFIX_DECREMENT_EXPRESSION,
    // SC_DEREFERENCE_MEMBER_EXPRESSION,
    // SC_MEMBER_EXPRESSION,
    // SC_FUNCTION_CALL_EXPRESSION,
    // SC_SUBSCRIPT_EXPRESSION,
    // SC_TYPE_NAME,
    // SC_TYPEDEF_NAME,
    // SC_ENUMERATION_CONSTANT,
    // SC_FLOATING_CONSTANT,
    // SC_INTEGER_CONSTANT,
    // SC_CHARACTER_CONSTANT,
    // SC_STRING_LITERAL

static void print_syntax_indented(syntax_component_t* s, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!s)
    {
        ps("null\n");
        return;
    }
    switch (s->type)
    {
        case SC_IDENTIFIER:
        {
            ps("identifier: %s\n", s->id);
            break;
        }

        case SC_TYPEDEF_NAME:
        {
            ps("typedef name: %s\n", s->id);
            break;
        }

        case SC_ENUMERATION_CONSTANT:
        {
            ps("enumeration constant: %s\n", s->id);
            break;
        }
        
        case SC_DECLARATOR_IDENTIFIER:
        {
            ps("identifier (declarator): %s\n", s->id);
            break;
        }
        
        case SC_PRIMARY_EXPRESSION_IDENTIFIER:
        {
            ps("identifier (primary expression): %s\n", s->id);
            break;
        }

        case SC_STORAGE_CLASS_SPECIFIER:
        {
            ps("storage class specifier: %s\n", STORAGE_CLASS_NAMES[s->scs]);
            break;
        }

        case SC_BASIC_TYPE_SPECIFIER:
        {
            ps("type specifier (basic): %s\n", BASIC_TYPE_SPECIFIER_NAMES[s->bts]);
            break;
        }

        case SC_TYPE_QUALIFIER:
        {
            ps("type qualifier: %s\n", TYPE_QUALIFIER_NAMES[s->tq]);
            break;
        }

        case SC_FUNCTION_SPECIFIER:
        {
            ps("function specifier: %s\n", FUNCTION_SPECIFIER_NAMES[s->fs]);
            break;
        }

        case SC_POINTER:
        {
            ps(sty("pointer") "\n");
            pf("type qualifiers:\n");
            print_vector_indented(s->ptr_type_qualifiers, next_indent, printer);
            break;
        }

        case SC_DECLARATOR:
        {
            ps(sty("declarator") "\n");
            pf("pointers:\n");
            print_vector_indented(s->declr_pointers, next_indent, printer);
            pf("direct declarator:\n");
            print_syntax_indented(s->declr_direct, next_indent, printer);
            break;
        }

        case SC_INIT_DECLARATOR:
        {
            ps(sty("init declarator") "\n");
            pf("declarator:\n");
            print_syntax_indented(s->ideclr_declarator, next_indent, printer);
            pf("initializer:\n");
            print_syntax_indented(s->ideclr_initializer, next_indent, printer);
            break;
        }

        case SC_DECLARATION:
        {
            ps(sty("declaration") "\n");
            pf("declaration specifiers:\n");
            print_vector_indented(s->decl_declaration_specifiers, next_indent, printer);
            pf("init declarators:\n");
            print_vector_indented(s->decl_init_declarators, next_indent, printer);
            break;
        }

        case SC_ERROR:
        {
            ps(sty("error") "\n");
            pf("message: %s\n", s->err_message);
            pf("depth: %d\n", s->err_depth);
            break;
        }

        case SC_TRANSLATION_UNIT:
        {
            ps(sty("translation unit") "\n");
            pf("external declarations:\n");
            print_vector_indented(s->tlu_external_declarations, next_indent, printer);
            break;
        }

        case SC_FUNCTION_DEFINITION:
        {
            ps(sty("function definition") "\n");
            pf("declaration specifiers:\n");
            print_vector_indented(s->fdef_declaration_specifiers, next_indent, printer);
            pf("declarator:\n");
            print_syntax_indented(s->fdef_declarator, next_indent, printer);
            pf("K&R declarations:\n");
            print_vector_indented(s->fdef_knr_declarations, next_indent, printer);
            pf("body:\n");
            print_syntax_indented(s->fdef_body, next_indent, printer);
            break;
        }

        case SC_STRUCT_UNION_SPECIFIER:
        {
            if (s->sus_sou == SOU_STRUCT)
            {
                ps(sty("struct specifier") "\n");
            }
            else if (s->sus_sou == SOU_UNION)
            {
                ps(sty("union specifier") "\n");
            }
            pf("identifier:\n");
            print_syntax_indented(s->sus_id, next_indent, printer);
            pf("declarations:\n");
            print_vector_indented(s->sus_declarations, next_indent, printer);
            break;
        }

        case SC_ARRAY_DECLARATOR:
        {
            ps(sty("array declarator") "\n");
            pf("direct declarator:\n");
            print_syntax_indented(s->adeclr_direct, next_indent, printer);
            pf("type qualifiers:\n");
            print_vector_indented(s->adeclr_type_qualifiers, next_indent, printer);
            pf("static: %s\n", BOOL_NAMES[s->adeclr_is_static]);
            pf("length expression:\n");
            print_syntax_indented(s->adeclr_length_expression, next_indent, printer);
            pf("unspecified size: %s\n", BOOL_NAMES[s->adeclr_unspecified_size]);
            break;
        }

        case SC_FUNCTION_DECLARATOR:
        {
            ps(sty("function declarator") "\n");
            pf("direct declarator:\n");
            print_syntax_indented(s->fdeclr_direct, next_indent, printer);
            pf("parameter declarations:\n");
            print_vector_indented(s->fdeclr_parameter_declarations, next_indent, printer);
            pf("K&R identifiers:\n");
            print_vector_indented(s->fdeclr_knr_identifiers, next_indent, printer);
            pf("ellipsis: %s\n", BOOL_NAMES[s->fdeclr_ellipsis]);
            break;
        }

        case SC_PARAMETER_DECLARATION:
        {
            ps(sty("parameter declaration") "\n");
            pf("declaration specifiers:\n");
            print_vector_indented(s->pdecl_declaration_specifiers, next_indent, printer);
            pf("declarator:\n");
            print_syntax_indented(s->pdecl_declr, next_indent, printer);
            break;
        }

        case SC_COMPOUND_STATEMENT:
        {
            ps(sty("compound statement") "\n");
            pf("block items:\n");
            print_vector_indented(s->cstmt_block_items, next_indent, printer);
            break;
        }

        case SC_RETURN_STATEMENT:
        {
            ps(sty("return statement") "\n");
            pf("expression:\n");
            print_syntax_indented(s->retstmt_expression, next_indent, printer);
            break;
        }

        case SC_EXPRESSION:
        {
            ps(sty("expression") "\n");
            pf("subexpressions:\n")
            print_vector_indented(s->expr_expressions, next_indent, printer);
            break;
        }

        case SC_INTEGER_CONSTANT:
        {
            ps("integer constant (ULL): %llu\n", s->intc);
            break;
        }

        case SC_FLOATING_CONSTANT:
        {
            ps("floating constant (LD): %Lf\n", s->floc);
            break;
        }

        case SC_IF_STATEMENT:
        {
            ps(sty("if statement") "\n");
            pf("condition:\n");
            print_syntax_indented(s->ifstmt_condition, next_indent, printer);
            pf("body:\n");
            print_syntax_indented(s->ifstmt_body, next_indent, printer);
            pf("else:\n");
            print_syntax_indented(s->ifstmt_else, next_indent, printer);
            break;
        }

        case SC_SWITCH_STATEMENT:
        {
            ps(sty("switch statement") "\n");
            pf("condition:\n");
            print_syntax_indented(s->swstmt_condition, next_indent, printer);
            pf("body:\n");
            print_syntax_indented(s->swstmt_body, next_indent, printer);
            break;
        }

        case SC_LABELED_STATEMENT:
        {
            ps(sty("labeled statement") "\n");
            pf("case expression:\n");
            print_syntax_indented(s->lstmt_case_expression, next_indent, printer);
            pf("identifier:\n");
            print_syntax_indented(s->lstmt_id, next_indent, printer);
            pf("statement:\n");
            print_syntax_indented(s->lstmt_stmt, next_indent, printer);
            break;
        }

        case SC_BREAK_STATEMENT:
            ps(sty("break statement") "\n");
            break;

        case SC_CONTINUE_STATEMENT:
            ps(sty("continue statement") "\n");
            break;
        
        case SC_EXPRESSION_STATEMENT:
        {
            ps(sty("expression statement") "\n");
            pf("expression:\n");
            print_syntax_indented(s->estmt_expression, next_indent, printer);
            break;
        }

        case SC_DESIGNATION:
        {
            ps(sty("designation") "\n");
            pf("designators:\n");
            print_vector_indented(s->desig_designators, next_indent, printer);
            break;
        }

        case SC_INITIALIZER_LIST:
        {
            ps(sty("initializer list") "\n");
            pf("designations:\n");
            print_vector_indented(s->inlist_designations, next_indent, printer);
            pf("initializers:\n");
            print_vector_indented(s->inlist_initializers, next_indent, printer);
            break;
        }

        case SC_STRUCT_DECLARATION:
        {
            ps(sty("struct declaration") "\n");
            pf("specifiers & qualifiers:\n");
            print_vector_indented(s->sdecl_specifier_qualifier_list, next_indent, printer);
            pf("declarators:\n");
            print_vector_indented(s->sdecl_declarators, next_indent, printer);
            break;
        }

        case SC_STRUCT_DECLARATOR:
        {
            ps(sty("struct declarator") "\n");
            pf("declarator:\n");
            print_syntax_indented(s->sdeclr_declarator, next_indent, printer);
            pf("bits expression:\n");
            print_syntax_indented(s->sdeclr_bits_expression, next_indent, printer);
            break;
        }

        case SC_ASSIGNMENT_EXPRESSION:
        case SC_LOGICAL_OR_EXPRESSION:
        case SC_LOGICAL_AND_EXPRESSION:
        case SC_BITWISE_OR_EXPRESSION:
        case SC_BITWISE_XOR_EXPRESSION:
        case SC_BITWISE_AND_EXPRESSION:
        case SC_EQUALITY_EXPRESSION:
        case SC_INEQUALITY_EXPRESSION:
        case SC_GREATER_EQUAL_EXPRESSION:
        case SC_LESS_EQUAL_EXPRESSION:
        case SC_GREATER_EXPRESSION:
        case SC_LESS_EXPRESSION:
        case SC_BITWISE_RIGHT_EXPRESSION:
        case SC_BITWISE_LEFT_EXPRESSION:
        case SC_SUBTRACTION_EXPRESSION:
        case SC_ADDITION_EXPRESSION:
        case SC_MODULAR_EXPRESSION:
        case SC_DIVISION_EXPRESSION:
        case SC_MULTIPLICATION_EXPRESSION:
        {
            ps(sty("binary expression") "\n");
            pf("type name: %s\n", SYNTAX_COMPONENT_NAMES[s->type]);
            pf("lhs:\n");
            print_syntax_indented(s->bexpr_lhs, next_indent, printer);
            pf("rhs:\n");
            print_syntax_indented(s->bexpr_rhs, next_indent, printer);
            break;
        }

        case SC_PREFIX_INCREMENT_EXPRESSION:
        case SC_PREFIX_DECREMENT_EXPRESSION:
        case SC_REFERENCE_EXPRESSION:
        case SC_DEREFERENCE_EXPRESSION:
        case SC_PLUS_EXPRESSION:
        case SC_MINUS_EXPRESSION:
        case SC_COMPLEMENT_EXPRESSION:
        case SC_NOT_EXPRESSION:
        case SC_SIZEOF_EXPRESSION:
        case SC_SIZEOF_TYPE_EXPRESSION:
        case SC_POSTFIX_INCREMENT_EXPRESSION:
        case SC_POSTFIX_DECREMENT_EXPRESSION:
        {
            ps(sty("unary expression") "\n");
            pf("type name: %s\n", SYNTAX_COMPONENT_NAMES[s->type]);
            pf("operand:\n");
            print_syntax_indented(s->uexpr_operand, next_indent, printer);
            break;
        }

        case SC_CAST_EXPRESSION:
        {
            ps(sty("cast expression") "\n");
            pf("type name:\n");
            print_syntax_indented(s->caexpr_type_name, next_indent, printer);
            pf("operand:\n");
            print_syntax_indented(s->caexpr_operand, next_indent, printer);
            break;
        }

        case SC_TYPE_NAME:
        {
            ps(sty("type name") "\n");
            pf("specifiers & qualifiers:\n");
            print_vector_indented(s->tn_specifier_qualifier_list, next_indent, printer);
            pf("declarator:\n");
            print_syntax_indented(s->tn_declarator, next_indent, printer);
            break;
        }

        case SC_ENUM_SPECIFIER:
        {
            ps(sty("enum specifier") "\n");
            pf("identifier:\n");
            print_syntax_indented(s->enums_id, next_indent, printer);
            pf("enumerators:\n");
            print_vector_indented(s->enums_enumerators, next_indent, printer);
            break;
        }

        case SC_ENUMERATOR:
        {
            ps(sty("enumerator") "\n");
            pf("enumeration constant:\n");
            print_syntax_indented(s->enumr_constant, next_indent, printer);
            pf("expression:\n");
            print_syntax_indented(s->enumr_expression, next_indent, printer);
            break;
        }

        case SC_SUBSCRIPT_EXPRESSION:
        {
            ps(sty("subscript expression") "\n");
            pf("expression:\n");
            print_syntax_indented(s->subsexpr_expression, next_indent, printer);
            pf("index expression:\n");
            print_syntax_indented(s->subsexpr_index_expression, next_indent, printer);
            break;
        }

        case SC_ABSTRACT_DECLARATOR:
        {
            ps(sty("abstract declarator") "\n");
            pf("direct declarator:\n");
            print_syntax_indented(s->abdeclr_direct, next_indent, printer);
            pf("pointers:\n");
            print_vector_indented(s->abdeclr_pointers, next_indent, printer);
            break;
        }

        case SC_ABSTRACT_ARRAY_DECLARATOR:
        {
            ps(sty("abstract array declarator") "\n");
            pf("direct declarator:\n");
            print_syntax_indented(s->abadeclr_direct, next_indent, printer);
            pf("length expression:\n");
            print_syntax_indented(s->abadeclr_length_expression, next_indent, printer);
            pf("unspecified size: %s\n", BOOL_NAMES[s->abadeclr_unspecified_size]);
            break;
        }

        case SC_FUNCTION_CALL_EXPRESSION:
        {
            ps(sty("function call expression") "\n");
            pf("calling expression:\n");
            print_syntax_indented(s->fcallexpr_expression, next_indent, printer);
            pf("arguments:\n");
            print_vector_indented(s->fcallexpr_args, next_indent, printer);
            break;
        }

        case SC_STRING_LITERAL:
        {
            if (s->strl_reg)
                ps("string literal: \"%s\"\n", s->strl_reg)
            else
                ps("string literal: \"%ls\"\n", s->strl_wide)
            break;
        }

        case SC_COMPOUND_LITERAL:
        {
            ps(sty("compound literal") "\n");
            if (s->cl_id)
                pf("identifier: %s\n", s->cl_id);
            pf("type name:\n");
            print_syntax_indented(s->cl_type_name, next_indent, printer);
            pf("initializer list:\n");
            print_syntax_indented(s->cl_inlist, next_indent, printer);
            break;
        }

        case SC_MEMBER_EXPRESSION:
        case SC_DEREFERENCE_MEMBER_EXPRESSION:
        {
            if (s->type == SC_MEMBER_EXPRESSION)
            {
                ps(sty("member expression") "\n");
            }
            else
            {
                ps(sty("dereference member expression") "\n");
            }
            pf("expression:\n");
            print_syntax_indented(s->memexpr_expression, next_indent, printer);
            pf("identifier:\n");
            print_syntax_indented(s->memexpr_id, next_indent, printer);
            break;
        }

        case SC_FOR_STATEMENT:
        {
            ps(sty("for statement") "\n");
            pf("init:\n");
            print_syntax_indented(s->forstmt_init, next_indent, printer);
            pf("condition:\n");
            print_syntax_indented(s->forstmt_condition, next_indent, printer);
            pf("post:\n");
            print_syntax_indented(s->forstmt_post, next_indent, printer);
            pf("body:\n");
            print_syntax_indented(s->forstmt_body, next_indent, printer);
            break;
        }

        default:
            ps(sty("unknown/unprintable syntax") "\n");
            pf("type name: %s\n", SYNTAX_COMPONENT_NAMES[s->type]);
            return;
    }
    if (s->ctype)
    {
        pf("C type: ");
        type_humanized_print(s->ctype, printer);
        pf("\n");
    }
    if (debug)
    {
        if (s->parent)
        {
            pf("parent type (for confirmation): %s\n", SYNTAX_COMPONENT_NAMES[s->parent->type]);
        }
        else
        {
            pf("parent type (for confirmation): no parent\n");
        }
    }
}

#undef ps
#undef pf
#undef next_indent
#undef sty

void print_syntax(syntax_component_t* s, int (*printer)(const char* fmt, ...))
{
    print_syntax_indented(s, 0, printer);
}

// free a syntax component and EVERYTHING IT HAS UNDERNEATH IT.
void free_syntax(syntax_component_t* syn, syntax_component_t* tlu)
{
    if (!syn)
        return;
    switch (syn->type)
    {
        case SC_ERROR:
        {
            free(syn->err_message);
            break;
        }
        case SC_TRANSLATION_UNIT:
        {
            deep_free_syntax_vector(syn->tlu_external_declarations, s1);
            deep_free_syntax_vector(syn->tlu_errors, s2);
            symbol_table_delete(syn->tlu_st, true);
            break;
        }
        case SC_IDENTIFIER:
        case SC_TYPEDEF_NAME:
        case SC_ENUMERATION_CONSTANT:
        case SC_DECLARATOR_IDENTIFIER:
        case SC_PRIMARY_EXPRESSION_IDENTIFIER:
        {
            // remove the id from the symbol table if it defines a symbol
            if (tlu && syn->id)
                symbol_delete(symbol_table_remove(tlu->tlu_st, syn));
            free(syn->id);
            break;
        }
        case SC_POINTER:
        {
            deep_free_syntax_vector(syn->ptr_type_qualifiers, s);
            break;
        }
        case SC_DECLARATOR:
        {
            deep_free_syntax_vector(syn->declr_pointers, s);
            free_syntax(syn->declr_direct, tlu);
            break;
        }
        case SC_INIT_DECLARATOR:
        {
            free_syntax(syn->ideclr_declarator, tlu);
            free_syntax(syn->ideclr_initializer, tlu);
            break;
        }
        case SC_DECLARATION:
        {
            deep_free_syntax_vector(syn->decl_declaration_specifiers, s1);
            deep_free_syntax_vector(syn->decl_init_declarators, s2);
            break;
        }
        case SC_ARRAY_DECLARATOR:
        {
            free_syntax(syn->adeclr_direct, tlu);
            deep_free_syntax_vector(syn->adeclr_type_qualifiers, s);
            free_syntax(syn->adeclr_length_expression, tlu);
            break;
        }
        case SC_FUNCTION_DECLARATOR:
        {
            free_syntax(syn->fdeclr_direct, tlu);
            deep_free_syntax_vector(syn->fdeclr_knr_identifiers, s1);
            deep_free_syntax_vector(syn->fdeclr_parameter_declarations, s2);
            break;
        }
        case SC_PARAMETER_DECLARATION:
        {
            free_syntax(syn->pdecl_declr, tlu);
            deep_free_syntax_vector(syn->pdecl_declaration_specifiers, s);
            break;
        }
        case SC_STRUCT_UNION_SPECIFIER:
        {
            deep_free_syntax_vector(syn->sus_declarations, s);
            free_syntax(syn->sus_id, tlu);
            break;
        }
        case SC_STRUCT_DECLARATION:
        {
            deep_free_syntax_vector(syn->sdecl_declarators, s1);
            deep_free_syntax_vector(syn->sdecl_specifier_qualifier_list, s2);
            break;
        }
        case SC_STRUCT_DECLARATOR:
        {
            free_syntax(syn->sdeclr_declarator, tlu);
            free_syntax(syn->sdeclr_bits_expression, tlu);
            break;
        }
        case SC_ENUM_SPECIFIER:
        {
            deep_free_syntax_vector(syn->enums_enumerators, s);
            free_syntax(syn->enums_id, tlu);
            break;
        }
        case SC_ENUMERATOR:
        {
            free_syntax(syn->enumr_expression, tlu);
            free_syntax(syn->enumr_constant, tlu);
            break;
        }
        case SC_ABSTRACT_DECLARATOR:
        {
            free_syntax(syn->abdeclr_direct, tlu);
            deep_free_syntax_vector(syn->abdeclr_pointers, s);
            break;
        }
        case SC_ABSTRACT_ARRAY_DECLARATOR:
        {
            free_syntax(syn->abadeclr_direct, tlu);
            free_syntax(syn->abadeclr_length_expression, tlu);
            break;
        }
        case SC_ABSTRACT_FUNCTION_DECLARATOR:
        {
            free_syntax(syn->abfdeclr_direct, tlu);
            deep_free_syntax_vector(syn->abfdeclr_parameter_declarations, s);
            break;
        }
        case SC_FUNCTION_DEFINITION:
        {
            free_syntax(syn->fdef_body, tlu);
            deep_free_syntax_vector(syn->fdef_declaration_specifiers, s1);
            free_syntax(syn->fdef_declarator, tlu);
            deep_free_syntax_vector(syn->fdef_knr_declarations, s2);
            break;
        }
        case SC_COMPOUND_STATEMENT:
        {
            deep_free_syntax_vector(syn->cstmt_block_items, s);
            break;
        }
        case SC_RETURN_STATEMENT:
        {
            free_syntax(syn->retstmt_expression, tlu);
            break;
        }
        case SC_GOTO_STATEMENT:
        {
            free_syntax(syn->gtstmt_label_id, tlu);
            break;
        }
        case SC_EXPRESSION:
        {
            deep_free_syntax_vector(syn->expr_expressions, s);
            break;
        }
        case SC_WHILE_STATEMENT:
        {
            free_syntax(syn->whstmt_body, tlu);
            free_syntax(syn->whstmt_condition, tlu);
            break;
        }
        case SC_DO_STATEMENT:
        {
            free_syntax(syn->dostmt_body, tlu);
            free_syntax(syn->dostmt_condition, tlu);
            break;
        }
        case SC_CAST_EXPRESSION:
        {
            free_syntax(syn->caexpr_type_name, tlu);
            free_syntax(syn->caexpr_operand, tlu);
            break;
        }
        case SC_CONDITIONAL_EXPRESSION:
        case SC_CONSTANT_EXPRESSION:
        {
            free_syntax(syn->cexpr_condition, tlu);
            free_syntax(syn->cexpr_if, tlu);
            free_syntax(syn->cexpr_else, tlu);
            break;
        }
        case SC_PREFIX_INCREMENT_EXPRESSION:
        case SC_PREFIX_DECREMENT_EXPRESSION:
        case SC_REFERENCE_EXPRESSION:
        case SC_DEREFERENCE_EXPRESSION:
        case SC_PLUS_EXPRESSION:
        case SC_MINUS_EXPRESSION:
        case SC_COMPLEMENT_EXPRESSION:
        case SC_NOT_EXPRESSION:
        case SC_SIZEOF_EXPRESSION:
        case SC_SIZEOF_TYPE_EXPRESSION:
        case SC_POSTFIX_INCREMENT_EXPRESSION:
        case SC_POSTFIX_DECREMENT_EXPRESSION:
        {
            free_syntax(syn->uexpr_operand, tlu);
            break;
        }
        case SC_ASSIGNMENT_EXPRESSION:
        case SC_LOGICAL_OR_EXPRESSION:
        case SC_LOGICAL_AND_EXPRESSION:
        case SC_BITWISE_OR_EXPRESSION:
        case SC_BITWISE_XOR_EXPRESSION:
        case SC_BITWISE_AND_EXPRESSION:
        case SC_EQUALITY_EXPRESSION:
        case SC_INEQUALITY_EXPRESSION:
        case SC_GREATER_EQUAL_EXPRESSION:
        case SC_LESS_EQUAL_EXPRESSION:
        case SC_GREATER_EXPRESSION:
        case SC_LESS_EXPRESSION:
        case SC_BITWISE_RIGHT_EXPRESSION:
        case SC_BITWISE_LEFT_EXPRESSION:
        case SC_SUBTRACTION_EXPRESSION:
        case SC_ADDITION_EXPRESSION:
        case SC_MODULAR_EXPRESSION:
        case SC_DIVISION_EXPRESSION:
        case SC_MULTIPLICATION_EXPRESSION:
        {
            free_syntax(syn->bexpr_lhs, tlu);
            free_syntax(syn->bexpr_rhs, tlu);
            break;
        }
        case SC_STRING_LITERAL:
        {
            free(syn->strl_reg);
            free(syn->strl_wide);
            free(syn->strl_id);
            break;
        }
        case SC_IF_STATEMENT:
        {
            free_syntax(syn->ifstmt_body, tlu);
            free_syntax(syn->ifstmt_condition, tlu);
            free_syntax(syn->ifstmt_else, tlu);
            break;
        }
        case SC_LABELED_STATEMENT:
        {
            free_syntax(syn->lstmt_case_expression, tlu);
            free_syntax(syn->lstmt_id, tlu);
            free_syntax(syn->lstmt_stmt, tlu);
            break;
        }
        case SC_EXPRESSION_STATEMENT:
        {
            free_syntax(syn->estmt_expression, tlu);
            break;
        }
        case SC_SWITCH_STATEMENT:
        {
            free_syntax(syn->swstmt_body, tlu);
            free_syntax(syn->swstmt_condition, tlu);
            break;
        }
        case SC_FOR_STATEMENT:
        {
            free_syntax(syn->forstmt_body, tlu);
            free_syntax(syn->forstmt_condition, tlu);
            free_syntax(syn->forstmt_post, tlu);
            free_syntax(syn->forstmt_init, tlu);
            break;
        }
        case SC_DESIGNATION:
        {
            deep_free_syntax_vector(syn->desig_designators, s);
            break;
        }
        case SC_INITIALIZER_LIST:
        {
            deep_free_syntax_vector(syn->inlist_designations, s1);
            deep_free_syntax_vector(syn->inlist_initializers, s2);
            break;
        }
        case SC_TYPE_NAME:
        {
            free_syntax(syn->tn_declarator, tlu);
            deep_free_syntax_vector(syn->tn_specifier_qualifier_list, s);
            break;
        }
        case SC_COMPOUND_LITERAL:
        {
            free_syntax(syn->cl_inlist, tlu);
            free_syntax(syn->cl_type_name, tlu);
            free(syn->cl_id);
            break;
        }
        case SC_MEMBER_EXPRESSION:
        case SC_DEREFERENCE_MEMBER_EXPRESSION:
        {
            free_syntax(syn->memexpr_expression, tlu);
            free_syntax(syn->memexpr_id, tlu);
            break;
        }
        case SC_FUNCTION_CALL_EXPRESSION:
        {
            free_syntax(syn->fcallexpr_expression, tlu);
            deep_free_syntax_vector(syn->fcallexpr_args, s);
            break;
        }
        case SC_SUBSCRIPT_EXPRESSION:
        {
            free_syntax(syn->subsexpr_expression, tlu);
            free_syntax(syn->subsexpr_index_expression, tlu);
            break;
        }
        // nothing to free
        case SC_UNKNOWN:
        case SC_BASIC_TYPE_SPECIFIER:
        case SC_TYPE_QUALIFIER:
        case SC_FUNCTION_SPECIFIER:
        case SC_STORAGE_CLASS_SPECIFIER:
        case SC_BREAK_STATEMENT:
        case SC_CONTINUE_STATEMENT:
        case SC_INTEGER_CONSTANT:
        case SC_FLOATING_CONSTANT:
        case SC_CHARACTER_CONSTANT:
            break;
        default:
        {
            if (debug)
                warnf("no defined free'ing operation for syntax elements of type %s\n", SYNTAX_COMPONENT_NAMES[syn->type]);
            break;
        }
    }
    type_delete(syn->ctype);
    air_insn_delete_all(syn->code);
    free(syn);
}

// only handles arithmetic types
unsigned long long constexpr_cast(unsigned long long value, c_type_t* old_type, c_type_t* new_type)
{
    unsigned long long nv = 0;
    #define subcase(ctc, t, old_t) \
        case ctc: \
        { \
            t v = (t) (*((old_t*) &value));\
            *((t*) &nv) = v; \
            break; \
        }
    #define subswitch(old_t) \
        switch (new_type->class) \
        { \
            subcase(CTC_BOOL, bool, old_t) \
            subcase(CTC_CHAR, char, old_t) \
            subcase(CTC_SIGNED_CHAR, signed char, old_t) \
            subcase(CTC_SHORT_INT, short int, old_t) \
            subcase(CTC_INT, int, old_t) \
            subcase(CTC_LONG_INT, long int, old_t) \
            subcase(CTC_LONG_LONG_INT, long long int, old_t) \
            subcase(CTC_UNSIGNED_CHAR, unsigned char, old_t) \
            subcase(CTC_UNSIGNED_SHORT_INT, unsigned short int, old_t) \
            subcase(CTC_UNSIGNED_INT, unsigned int, old_t) \
            subcase(CTC_UNSIGNED_LONG_INT, unsigned long int, old_t) \
            subcase(CTC_UNSIGNED_LONG_LONG_INT, unsigned long long int, old_t) \
            subcase(CTC_FLOAT, float, old_t) \
            subcase(CTC_DOUBLE, double, old_t) \
            subcase(CTC_LONG_DOUBLE, long double, old_t) \
            subcase(CTC_FLOAT_COMPLEX, float _Complex, old_t) \
            subcase(CTC_DOUBLE_COMPLEX, double _Complex, old_t) \
            subcase(CTC_LONG_DOUBLE_COMPLEX, long double _Complex, old_t) \
            default: return 0; \
        }
    switch (old_type->class)
    {
        case CTC_BOOL:
            subswitch(bool);
            break;
        case CTC_CHAR:
            subswitch(char)
            break;
        case CTC_SIGNED_CHAR:
            subswitch(signed char)
            break;
        case CTC_SHORT_INT:
            subswitch(short int)
            break;
        case CTC_INT:
            subswitch(int)
            break;
        case CTC_LONG_INT:
            subswitch(long int)
            break;
        case CTC_LONG_LONG_INT:
            subswitch(long long int)
            break;
        case CTC_UNSIGNED_CHAR:
            subswitch(unsigned char)
            break;
        case CTC_UNSIGNED_SHORT_INT:
            subswitch(unsigned short int)
            break;
        case CTC_UNSIGNED_INT:
            subswitch(unsigned int)
            break;
        case CTC_UNSIGNED_LONG_INT:
            subswitch(unsigned long int)
            break;
        case CTC_UNSIGNED_LONG_LONG_INT:
            subswitch(unsigned long long int)
            break;
        case CTC_FLOAT:
            subswitch(float)
            break;
        case CTC_DOUBLE:
            subswitch(double)
            break;
        case CTC_LONG_DOUBLE:
            subswitch(double)
            break;
        case CTC_FLOAT_COMPLEX:
            subswitch(float _Complex)
            break;
        case CTC_DOUBLE_COMPLEX:
            subswitch(double _Complex)
            break;
        case CTC_LONG_DOUBLE_COMPLEX:
            subswitch(long double _Complex)
            break;
        default: return 0;
    }
    #undef subswitch
    #undef subcase
    return nv;
}

unsigned long long constexpr_cast_type(unsigned long long value, c_type_class_t old_class, c_type_class_t new_class)
{
    c_type_t* old = make_basic_type(old_class);
    c_type_t* new = make_basic_type(new_class);
    unsigned long long r = constexpr_cast(value, old, new);
    type_delete(old);
    type_delete(new);
    return r;
}

unsigned long long process_integer_constant(char* con, c_type_class_t* class)
{
    int radix = 10;
    if (starts_with_ignore_case(con, "0x"))
    {
        radix = 16;
        con += 2;
    }
    else if (starts_with_ignore_case(con, "0") && strlen(con) > 1 && con[1] >= '0' && con[2] <= '9')
    {
        radix = 8;
        ++con;
    }
    char* end = NULL;
    unsigned long long full = strtoull(con, &end, radix);
    if (errno == ERANGE || !end || (*end &&
        !streq_ignore_case(end, "ull") &&
        !streq_ignore_case(end, "llu") &&
        !streq_ignore_case(end, "ll") &&
        !streq_ignore_case(end, "ul") &&
        !streq_ignore_case(end, "lu") &&
        !streq_ignore_case(end, "l") &&
        !streq_ignore_case(end, "u")))
    {
        errno = 0;
        *class = CTC_ERROR;
        return 0ULL;
    }
    if (ends_with_ignore_case(con, "ull") ||
        ends_with_ignore_case(con, "llu"))
    {
        *class = CTC_UNSIGNED_LONG_LONG_INT;
        return full;
    }
    if (ends_with_ignore_case(con, "ll"))
    {
        *class = CTC_LONG_LONG_INT;
        if (radix == 10)
            return (long long) full;
        if (full > LONG_LONG_INT_MAX)
        {
            *class = CTC_UNSIGNED_LONG_LONG_INT;
            return full;
        }
        return (long long) full;
    }
    if (ends_with_ignore_case(con, "ul") ||
        ends_with_ignore_case(con, "lu"))
    {
        *class = CTC_UNSIGNED_LONG_INT;
        if (full > UNSIGNED_LONG_INT_MAX)
        {
            *class = CTC_UNSIGNED_LONG_LONG_INT;
            return full;
        }
        return (long long) full;
    }
    if (ends_with_ignore_case(con, "l"))
    {
        *class = CTC_LONG_INT;

        if (radix == 10)
        {
            if (full > LONG_INT_MAX)
            {
                *class = CTC_LONG_LONG_INT;
                return (long long) full;
            }
            return (long) full;
        }

        if (full > LONG_INT_MAX)
            *class = CTC_UNSIGNED_LONG_INT;
        if (full > UNSIGNED_LONG_INT_MAX)
            *class = CTC_LONG_LONG_INT;
        if (full > UNSIGNED_LONG_LONG_INT_MAX)
            *class = CTC_UNSIGNED_LONG_LONG_INT;
        
        if (*class == CTC_LONG_INT)
            return (long) full;
        if (*class == CTC_UNSIGNED_LONG_INT)
            return (unsigned long) full;
        if (*class == CTC_LONG_LONG_INT)
            return (long long) full;
        return full;
    }
    if (ends_with_ignore_case(con, "u"))
    {
        *class = CTC_UNSIGNED_INT;

        if (full > UNSIGNED_INT_MAX)
            *class = CTC_UNSIGNED_LONG_INT;
        if (full > UNSIGNED_LONG_INT_MAX)
            *class = CTC_UNSIGNED_LONG_LONG_INT;
        
        if (*class == CTC_UNSIGNED_INT)
            return (unsigned) full;
        if (*class == CTC_UNSIGNED_LONG_INT)
            return (unsigned long) full;
        return full;
    }
    
    *class = CTC_INT;

    if (full > INT_MAX)
        *class = radix == 10 ? CTC_LONG_INT : CTC_UNSIGNED_INT;
    if (radix != 10 && full > UNSIGNED_INT_MAX)
        *class = CTC_LONG_INT;
    if (full > LONG_INT_MAX)
        *class = radix == 10 ? CTC_LONG_LONG_INT : CTC_UNSIGNED_LONG_INT;
    if (radix != 10 && full > UNSIGNED_LONG_INT_MAX)
        *class = CTC_LONG_LONG_INT;
    if (radix != 10 && full > LONG_LONG_INT_MAX)
        *class = CTC_UNSIGNED_LONG_LONG_INT;
    
    if (*class == CTC_INT)
        return (int) full;
    if (*class == CTC_UNSIGNED_INT)
        return (unsigned) full;
    if (*class == CTC_LONG_INT)
        return (long) full;
    if (*class == CTC_UNSIGNED_LONG_INT)
        return (unsigned long) full;
    if (*class == CTC_LONG_LONG_INT)
        return (long long) full;
    return full;
}

long double process_floating_constant(char* con, c_type_class_t* class)
{
    char* end = NULL;
    long double value = strtof(con, &end);

    if (end && (streq(end, "f") || streq(end, "F")))
    {
        *class = CTC_FLOAT;
        return value;
    }

    if (end && (streq(end, "l") || streq(end, "L")))
    {
        *class = CTC_LONG_DOUBLE;
        return value;
    }

    if (errno == ERANGE || !end || *end)
    {
        errno = 0;
        *class = CTC_ERROR;
        return 0.0L;
    }

    *class = CTC_DOUBLE;
    return value;
}

// temp expression til i figure out errors here
#define do_something(syn, error) (void) (error)

// ! UNFINISHED !
// the expression given to this function doesn't have to be checked for constant-ness
// if it finds something it can't evaluate, it will toss it out and return CTC_ERROR in the 'class' parameter
// otherwise, it will give back the type you should cast the return type to.
unsigned long long evaluate_constant_expression(syntax_component_t* expr, c_type_class_t* class, constexpr_type_t cexpr_type)
{
    //     type == SC_EQUALITY_EXPRESSION ||
    //     type == SC_INEQUALITY_EXPRESSION ||
    //     type == SC_GREATER_EQUAL_EXPRESSION ||
    //     type == SC_LESS_EQUAL_EXPRESSION ||
    //     type == SC_GREATER_EXPRESSION ||
    //     type == SC_LESS_EXPRESSION ||
    //     type == SC_BITWISE_RIGHT_EXPRESSION ||
    //     type == SC_BITWISE_LEFT_EXPRESSION ||
    //     type == SC_SUBTRACTION_EXPRESSION ||
    //     type == SC_ADDITION_EXPRESSION ||
    //     type == SC_MODULAR_EXPRESSION ||
    //     type == SC_DIVISION_EXPRESSION ||
    //     type == SC_MULTIPLICATION_EXPRESSION ||
    //     type == SC_CONDITIONAL_EXPRESSION ||
    //     type == SC_CAST_EXPRESSION ||
    //     type == SC_REFERENCE_EXPRESSION ||
    //     type == SC_DEREFERENCE_EXPRESSION ||
    //     type == SC_PLUS_EXPRESSION ||
    //     type == SC_MINUS_EXPRESSION ||
    //     type == SC_COMPLEMENT_EXPRESSION ||
    //     type == SC_NOT_EXPRESSION ||
    //     type == SC_SIZEOF_EXPRESSION ||
    //     type == SC_SIZEOF_TYPE_EXPRESSION ||
    //     type == SC_COMPOUND_LITERAL ||
    //     type == SC_DEREFERENCE_MEMBER_EXPRESSION ||
    //     type == SC_MEMBER_EXPRESSION ||
    //     type == SC_SUBSCRIPT_EXPRESSION ||
    //     type == SC_CONSTANT_EXPRESSION ||
    //     type == SC_INTEGER_CONSTANT ||
    //     type == SC_FLOATING_CONSTANT ||
    //     type == SC_CHARACTER_CONSTANT ||
    //     type == SC_STRING_LITERAL;
    *class = CTC_ERROR;
    unsigned long long r = 0;
    if (!expr)
    {
        do_something(expr, "couldn't find expression");
        return 0;
    }
    switch (expr->type)
    {
        case SC_LOGICAL_OR_EXPRESSION:
        {
            c_type_class_t c = CTC_ERROR;
            unsigned long long value = evaluate_constant_expression(expr->bexpr_lhs, &c, cexpr_type);
            if (!type_is_scalar_type(c))
            {
                do_something(expr->bexpr_lhs, "left hand side of logical or expression must be of scalar type");
                break;
            }
            if (value != 0)
            {
                *class = CTC_INT;
                r = 1ULL;
                break;
            }
            c = CTC_ERROR;
            value = evaluate_constant_expression(expr->bexpr_rhs, &c, cexpr_type);
            if (!type_is_scalar_type(c))
            {
                do_something(expr->bexpr_rhs, "right hand side of logical or expression must be of scalar type");
                break;
            }
            *class = CTC_INT;
            r = value != 0 ? 1ULL : 0ULL;
            break;
        }

        case SC_LOGICAL_AND_EXPRESSION:
        {
            c_type_class_t c = CTC_ERROR;
            unsigned long long value = evaluate_constant_expression(expr->bexpr_lhs, &c, cexpr_type);
            if (!type_is_scalar_type(c))
            {
                do_something(expr->bexpr_lhs, "left hand side of logical and expression must be of scalar type");
                break;
            }
            if (value == 0)
            {
                *class = CTC_INT;
                break;
            }
            c = CTC_ERROR;
            value = evaluate_constant_expression(expr->bexpr_rhs, &c, cexpr_type);
            if (!type_is_scalar_type(c))
            {
                do_something(expr->bexpr_rhs, "right hand side of logical and expression must be of scalar type");
                break;
            }
            *class = CTC_INT;
            r = value != 0 ? 1ULL : 0ULL;
            break;
        }

        #define bitwise_case(name, operator, sc_type) \
            case sc_type: \
            { \
                c_type_class_t lhc = CTC_ERROR; \
                c_type_class_t rhc = CTC_ERROR; \
                unsigned long long lhvalue = evaluate_constant_expression(expr->bexpr_lhs, &lhc, cexpr_type); \
                unsigned long long rhvalue = evaluate_constant_expression(expr->bexpr_rhs, &rhc, cexpr_type); \
                if (!type_is_integer_type(lhc)) \
                { \
                    do_something(expr->bexpr_lhs, "left hand side of " name " expression must of integer type"); \
                    break; \
                } \
                if (!type_is_integer_type(rhc)) \
                { \
                    do_something(expr->bexpr_rhs, "right hand side of " name " expression must of integer type"); \
                    break; \
                } \
                c_type_t* t1 = make_basic_type(lhc); \
                c_type_t* t2 = make_basic_type(rhc); \
                c_type_t* rt = usual_arithmetic_conversions_result_type(t1, t2); \
                *class = rt->class; \
                lhvalue = constexpr_cast(lhvalue, t1, rt); \
                rhvalue = constexpr_cast(rhvalue, t2, rt); \
                c_type_t* ull = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT); \
                r = constexpr_cast(lhvalue operator rhvalue, ull, rt); \
                type_delete(t1); \
                type_delete(t2); \
                type_delete(rt); \
                type_delete(ull); \
                break; \
            }

        bitwise_case("bitwise or", |, SC_BITWISE_OR_EXPRESSION)

        bitwise_case("bitwise xor", ^, SC_BITWISE_XOR_EXPRESSION)

        bitwise_case("bitwise and", &, SC_BITWISE_AND_EXPRESSION)
    
        // case SC_EQUALITY_EXPRESSION: 
        // { 
        //     c_type_class_t lhc = CTC_ERROR; 
        //     c_type_class_t rhc = CTC_ERROR;
        //     unsigned long long lhvalue = evaluate_constant_expression(expr->bexpr_lhs, &lhc, cexpr_type); 
        //     unsigned long long rhvalue = evaluate_constant_expression(expr->bexpr_rhs, &rhc, cexpr_type); 
        //     if (!type_is_integer_type(lhc)) 
        //     { 
        //         do_something(expr->bexpr_lhs, "left hand side of equality expression must of integer type"); 
        //         break;
        //     } 
        //     if (!type_is_integer_type(rhc)) 
        //     { 
        //         do_something(expr->bexpr_rhs, "right hand side of equality expression must of integer type"); 
        //         break;
        //     } 
        //     c_type_t* t1 = make_basic_type(lhc); 
        //     c_type_t* t2 = make_basic_type(rhc); 
        //     c_type_t* rt = usual_arithmetic_conversions_result_type(t1, t2); 
        //     *class = rt->class; 
        //     lhvalue = constexpr_cast(lhvalue, t1, rt); 
        //     rhvalue = constexpr_cast(rhvalue, t2, rt); 
        //     c_type_t* ull = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT); 
        //     r = constexpr_cast(lhvalue == rhvalue, ull, rt); 
        //     type_delete(t1);
        //     type_delete(t2);
        //     type_delete(rt);
        //     type_delete(ull);
        //     break;
        // }

        case SC_INTEGER_CONSTANT:
            r = expr->intc;
            *class = expr->ctype->class;
            break;

        // ISO: 6.6 (3)
        case SC_ASSIGNMENT_EXPRESSION:
        case SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION:
        case SC_DIVISION_ASSIGNMENT_EXPRESSION:
        case SC_MODULAR_ASSIGNMENT_EXPRESSION:
        case SC_ADDITION_ASSIGNMENT_EXPRESSION:
        case SC_SUBTRACTION_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_AND_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_OR_ASSIGNMENT_EXPRESSION:
        case SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION:
        case SC_PREFIX_INCREMENT_EXPRESSION:
        case SC_PREFIX_DECREMENT_EXPRESSION:
        case SC_POSTFIX_INCREMENT_EXPRESSION:
        case SC_POSTFIX_DECREMENT_EXPRESSION:
        case SC_FUNCTION_CALL_EXPRESSION:
            break;
        
        case SC_EXPRESSION:
        {
            // ISO: 6.6 (3)
            if (expr->expr_expressions->size > 1)
                break;
            r = evaluate_constant_expression(vector_get(expr->expr_expressions, 0), class, cexpr_type);
            break;
        }

        case SC_PRIMARY_EXPRESSION_IDENTIFIER:
        {
            c_namespace_t ns = get_basic_namespace(NSC_ORDINARY);
            symbol_t* sy = symbol_table_lookup(syntax_get_translation_unit(expr)->tlu_st, expr, &ns);
            // enumeration constant
            if (sy && sy->declarer->parent && sy->declarer->parent->type == SC_ENUMERATOR)
            {
                syntax_component_t* enumr = sy->declarer->parent;
                int offset = -1;
                // if the given enumerator doesn't have a constant assignment,
                // we have to find its value by looking at previous values
                if (!enumr->enumr_expression)
                {
                    syntax_component_t* orig_enumr = enumr;
                    syntax_component_t* enums = enumr->parent;
                    int orig_idx = 0, idx = 0;
                    // loop thru enumerators, finding the one closest to ours
                    // that has a constant assigned to it
                    VECTOR_FOR(syntax_component_t*, s, enums->enums_enumerators)
                    {
                        if (s == orig_enumr)
                        {
                            orig_idx = i;
                            break;
                        }
                        if (s->enumr_expression)
                        {
                            enumr = s;
                            idx = i;
                        }
                    }
                    // if we didn't find an enumerator with an expression,
                    // use its index in the vector
                    if (orig_enumr == enumr)
                    {
                        r = (int) orig_idx;
                        *class = CTC_INT;
                        break;
                    }
                    offset = orig_idx - idx;
                }
                c_type_class_t c = CTC_ERROR;
                unsigned long long result = evaluate_constant_expression(enumr->enumr_expression, &c, CE_INTEGER);
                if (c == CTC_ERROR)
                    break;
                if ((get_integer_type_conversion_rank(c) > get_integer_type_conversion_rank(CTC_INT)) || c == CTC_UNSIGNED_INT)
                {
                    do_something(expr, "enumerator must be able to fit in type 'int'");
                    break;
                }
                if (offset != -1)
                {
                    if (offset > 0 && result > INT_MAX - offset)
                    {
                        do_something(expr, "enumerator value overflows type 'int'");
                        break;
                    }
                    result += offset;
                }
                r = constexpr_cast_type(result, c, CTC_INT);
                *class = CTC_INT;
                break;
            }
            do_something(expr, "identifier that is not enumeration constant not allowed in constant expression");
            break;
        }

        default:
            do_something(expr, "undefined operation");
            break;
    }
    return r;
}

unsigned long long evaluate_enumeration_constant(syntax_component_t* enumr)
{
    int offset = -1;
    // if the given enumerator doesn't have a constant assignment,
    // we have to find its value by looking at previous values
    if (!enumr->enumr_expression)
    {
        syntax_component_t* orig_enumr = enumr;
        syntax_component_t* enums = enumr->parent;
        int orig_idx = 0, idx = 0;
        // loop thru enumerators, finding the one closest to ours
        // that has a constant assigned to it
        VECTOR_FOR(syntax_component_t*, s, enums->enums_enumerators)
        {
            if (s == orig_enumr)
            {
                orig_idx = i;
                break;
            }
            if (s->enumr_expression)
            {
                enumr = s;
                idx = i;
            }
        }
        // if we didn't find an enumerator with an expression,
        // use its index in the vector
        if (orig_enumr == enumr)
            return (int) orig_idx;
        offset = orig_idx - idx;
    }
    c_type_class_t c = CTC_ERROR;
    unsigned long long result = evaluate_constant_expression(enumr->enumr_expression, &c, CE_INTEGER);
    if (c == CTC_ERROR)
        report_return_value(0);
    if ((get_integer_type_conversion_rank(c) > get_integer_type_conversion_rank(CTC_INT)) || c == CTC_UNSIGNED_INT)
        report_return_value(0);
    if (offset != -1)
    {
        if (offset > 0 && result > INT_MAX - offset)
            report_return_value(0);
        result += offset;
    }
    return constexpr_cast_type(result, c, CTC_INT);
}