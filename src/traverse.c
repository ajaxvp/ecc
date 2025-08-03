#include <stdlib.h>

#include "ecc.h"

#define max(x, y) ((x) > (y) ? (x) : (y))

#define BEFORE trav->before[syn->type] ? trav->before[syn->type](trav, syn) : trav->default_before(trav, syn)
#define AFTER trav->after[syn->type] ? trav->after[syn->type](trav, syn) : trav->default_after(trav, syn)

static void no_action(syntax_traverser_t* trav, syntax_component_t* syn) {}

syntax_traverser_t* traverse_init(syntax_component_t* tlu, size_t size)
{
    syntax_traverser_t* trav = calloc(1, max(size, sizeof *trav));
    trav->tlu = tlu;
    trav->default_before = no_action;
    trav->default_after = no_action;
    return trav;
}

// does NOT free the translation unit
void traverse_delete(syntax_traverser_t* trav)
{
    free(trav);
}

#define traverse_vector(trav, v) if (v) { VECTOR_FOR(syntax_component_t*, s, (v)) traverse_syntax(trav, s); }

static void traverse_syntax(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (!syn) return;
    BEFORE;
    switch (syn->type)
    {
        case SC_TRANSLATION_UNIT:
        {
            traverse_vector(trav, syn->tlu_external_declarations);
            break;
        }
        // SC_FUNCTION_DEFINITION - fdef
        case SC_FUNCTION_DEFINITION:
        {
            traverse_syntax(trav, syn->fdef_body);
            traverse_vector(trav, syn->fdef_declaration_specifiers);
            traverse_syntax(trav, syn->fdef_declarator);
            traverse_vector(trav, syn->fdef_knr_declarations);
            break;
        }
        // SC_DECLARATION - decl
        case SC_DECLARATION:
        {
            traverse_vector(trav, syn->decl_declaration_specifiers);
            traverse_vector(trav, syn->decl_init_declarators);
            break;
        }
        // SC_INIT_DECLARATOR - ideclr
        case SC_INIT_DECLARATOR:
        {
            traverse_syntax(trav, syn->ideclr_declarator);
            traverse_syntax(trav, syn->ideclr_initializer);
            break;
        }
        // SC_STORAGE_CLASS_SPECIFIER - scs
        // SC_BASIC_TYPE_SPECIFIER - bts
        // SC_TYPE_QUALIFIER - tq
        // SC_FUNCTION_SPECIFIER - fs
        // SC_IDENTIFIER
        // SC_ENUMERATION_CONSTANT
        // SC_TYPEDEF_NAME
        // SC_FLOATING_CONSTANT - floc
        // SC_INTEGER_CONSTANT - intc
        // SC_CHARACTER_CONSTANT - chrc
        // SC_STRING_LITERAL - strl
        case SC_STORAGE_CLASS_SPECIFIER:
        case SC_BASIC_TYPE_SPECIFIER:
        case SC_TYPE_QUALIFIER:
        case SC_FUNCTION_SPECIFIER:
        case SC_IDENTIFIER:
        case SC_ENUMERATION_CONSTANT:
        case SC_TYPEDEF_NAME:
        case SC_DECLARATOR_IDENTIFIER:
        case SC_PRIMARY_EXPRESSION_IDENTIFIER:
        case SC_FLOATING_CONSTANT:
        case SC_INTEGER_CONSTANT:
        case SC_CHARACTER_CONSTANT:
        case SC_STRING_LITERAL:
            break;

        // SC_STRUCT_UNION_SPECIFIER - sus
        case SC_STRUCT_UNION_SPECIFIER:
        {
            traverse_syntax(trav, syn->sus_id);
            traverse_vector(trav, syn->sus_declarations);
            break;
        }

        // SC_STRUCT_DECLARATION - sdecl
        case SC_STRUCT_DECLARATION:
        {
            traverse_vector(trav, syn->sdecl_specifier_qualifier_list);
            traverse_vector(trav, syn->sdecl_declarators);
            break;
        }

        // SC_STRUCT_DECLARATOR - sdeclr
        case SC_STRUCT_DECLARATOR:
        {
            traverse_syntax(trav, syn->sdeclr_declarator);
            traverse_syntax(trav, syn->sdeclr_bits_expression);
            break;
        }

        // SC_ENUM_SPECIFIER - enums
        case SC_ENUM_SPECIFIER:
        {
            traverse_syntax(trav, syn->enums_id);
            traverse_vector(trav, syn->enums_enumerators);
            break;
        }

        // SC_ENUMERATOR - enumr
        case SC_ENUMERATOR:
        {
            traverse_syntax(trav, syn->enumr_expression);
            traverse_syntax(trav, syn->enumr_constant);
            break;
        }

        // SC_DECLARATOR - declr
        case SC_DECLARATOR:
        {
            traverse_vector(trav, syn->declr_pointers);
            traverse_syntax(trav, syn->declr_direct);
            break;
        }

        // SC_POINTER - ptr
        case SC_POINTER:
        {
            traverse_vector(trav, syn->ptr_type_qualifiers);
            break;
        }

        // SC_ARRAY_DECLARATOR - adeclr
        case SC_ARRAY_DECLARATOR:
        {
            traverse_syntax(trav, syn->adeclr_direct);
            traverse_syntax(trav, syn->adeclr_length_expression);
            traverse_vector(trav, syn->adeclr_type_qualifiers);
            break;
        }

        // SC_FUNCTION_DECLARATOR - fdeclr
        case SC_FUNCTION_DECLARATOR:
        {
            traverse_syntax(trav, syn->fdeclr_direct);
            traverse_vector(trav, syn->fdeclr_knr_identifiers);
            traverse_vector(trav, syn->fdeclr_parameter_declarations);
            break;
        }

        // SC_PARAMETER_DECLARATION - pdecl
        case SC_PARAMETER_DECLARATION:
        {
            traverse_syntax(trav, syn->pdecl_declr);
            traverse_vector(trav, syn->pdecl_declaration_specifiers);
            break;
        }

        // SC_ABSTRACT_DECLARATOR - abdeclr
        case SC_ABSTRACT_DECLARATOR:
        {
            traverse_syntax(trav, syn->abdeclr_direct);
            traverse_vector(trav, syn->abdeclr_pointers);
            break;
        }

        // SC_ABSTRACT_ARRAY_DECLARATOR - abadeclr
        case SC_ABSTRACT_ARRAY_DECLARATOR:
        {
            traverse_syntax(trav, syn->abadeclr_direct);
            traverse_syntax(trav, syn->abadeclr_length_expression);
            break;
        }

        // SC_ABSTRACT_FUNCTION_DECLARATOR - abfdeclr
        case SC_ABSTRACT_FUNCTION_DECLARATOR:
        {
            traverse_syntax(trav, syn->abfdeclr_direct);
            traverse_vector(trav, syn->abfdeclr_parameter_declarations);
            break;
        }

        // SC_LABELED_STATEMENT - lstmt
        case SC_LABELED_STATEMENT:
        {
            traverse_syntax(trav, syn->lstmt_case_expression);
            traverse_syntax(trav, syn->lstmt_id);
            traverse_syntax(trav, syn->lstmt_stmt);
            break;
        }

        // SC_COMPOUND_STATEMENT - cstmt
        case SC_COMPOUND_STATEMENT:
        {
            traverse_vector(trav, syn->cstmt_block_items);
            break;
        }

        // SC_EXPRESSION_STATEMENT - estmt
        case SC_EXPRESSION_STATEMENT:
        {
            traverse_syntax(trav, syn->estmt_expression);
            break;
        }

        // SC_IF_STATEMENT - ifstmt
        case SC_IF_STATEMENT:
        {
            traverse_syntax(trav, syn->ifstmt_condition);
            traverse_syntax(trav, syn->ifstmt_body);
            traverse_syntax(trav, syn->ifstmt_else);
            break;
        }

        // SC_SWITCH_STATEMENT - swstmt
        case SC_SWITCH_STATEMENT:
        {
            traverse_syntax(trav, syn->swstmt_condition);
            traverse_syntax(trav, syn->swstmt_body);
            break;
        }

        // SC_DO_STATEMENT - dostmt
        case SC_DO_STATEMENT:
        {
            traverse_syntax(trav, syn->dostmt_condition);
            traverse_syntax(trav, syn->dostmt_body);
            break;
        }

        // SC_WHILE_STATEMENT - whstmt
        case SC_WHILE_STATEMENT:
        {
            traverse_syntax(trav, syn->whstmt_condition);
            traverse_syntax(trav, syn->whstmt_body);
            break;
        }

        // SC_FOR_STATEMENT - forstmt
        case SC_FOR_STATEMENT:
        {
            traverse_syntax(trav, syn->forstmt_init);
            traverse_syntax(trav, syn->forstmt_condition);
            traverse_syntax(trav, syn->forstmt_post);
            traverse_syntax(trav, syn->forstmt_body);
            break;
        }

        // SC_GOTO_STATEMENT - gtstmt
        case SC_GOTO_STATEMENT:
        {
            traverse_syntax(trav, syn->gtstmt_label_id);
            break;
        }

        // SC_RETURN_STATEMENT - retstmt
        case SC_RETURN_STATEMENT:
        {
            traverse_syntax(trav, syn->retstmt_expression);
            break;
        }

        // SC_INITIALIZER_LIST - inlist
        case SC_INITIALIZER_LIST:
        {
            traverse_vector(trav, syn->inlist_designations);
            traverse_vector(trav, syn->inlist_initializers);
            break;
        }

        // SC_DESIGNATION - desig
        case SC_DESIGNATION:
        {
            traverse_vector(trav, syn->desig_designators);
            break;
        }

        // SC_EXPRESSION - expr
        case SC_EXPRESSION:
        {
            traverse_vector(trav, syn->expr_expressions);
            break;
        }

        // bexpr (general binary expression)
        // SC_ASSIGNMENT_EXPRESSION
        // SC_LOGICAL_OR_EXPRESSION
        // SC_LOGICAL_AND_EXPRESSION
        // SC_BITWISE_OR_EXPRESSION
        // SC_BITWISE_XOR_EXPRESSION
        // SC_BITWISE_AND_EXPRESSION
        // SC_EQUALITY_EXPRESSION
        // SC_INEQUALITY_EXPRESSION
        // SC_GREATER_EQUAL_EXPRESSION
        // SC_LESS_EQUAL_EXPRESSION
        // SC_GREATER_EXPRESSION
        // SC_LESS_EXPRESSION
        // SC_BITWISE_RIGHT_EXPRESSION
        // SC_BITWISE_LEFT_EXPRESSION
        // SC_SUBTRACTION_EXPRESSION
        // SC_ADDITION_EXPRESSION
        // SC_MODULAR_EXPRESSION
        // SC_DIVISION_EXPRESSION
        // SC_MULTIPLICATION_EXPRESSION
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
        {
            traverse_syntax(trav, syn->bexpr_lhs);
            traverse_syntax(trav, syn->bexpr_rhs);
            break;
        }

        // SC_CONDITIONAL_EXPRESSION - cexpr
        case SC_CONDITIONAL_EXPRESSION:
        {
            traverse_syntax(trav, syn->cexpr_condition);
            traverse_syntax(trav, syn->cexpr_if);
            traverse_syntax(trav, syn->cexpr_else);
            break;
        }

        // SC_CAST_EXPRESSION - caexpr
        case SC_CAST_EXPRESSION:
        {
            traverse_syntax(trav, syn->caexpr_type_name);
            traverse_syntax(trav, syn->caexpr_operand);
            break;
        }

        // uexpr (general unary expression)
        // SC_PREFIX_INCREMENT_EXPRESSION
        // SC_PREFIX_DECREMENT_EXPRESSION
        // SC_REFERENCE_EXPRESSION
        // SC_DEREFERENCE_EXPRESSION
        // SC_PLUS_EXPRESSION
        // SC_MINUS_EXPRESSION
        // SC_COMPLEMENT_EXPRESSION
        // SC_NOT_EXPRESSION
        // SC_SIZEOF_EXPRESSION
        // SC_SIZEOF_TYPE_EXPRESSION
        // SC_POSTFIX_INCREMENT_EXPRESSION
        // SC_POSTFIX_DECREMENT_EXPRESSION

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
            traverse_syntax(trav, syn->uexpr_operand);
            break;
        }

        // SC_COMPOUND_LITERAL - inlexpr
        case SC_COMPOUND_LITERAL:
        {
            traverse_syntax(trav, syn->cl_type_name);
            traverse_syntax(trav, syn->cl_inlist);
            break;
        }

        // SC_FUNCTION_CALL_EXPRESSION - fcallexpr
        case SC_FUNCTION_CALL_EXPRESSION:
        {
            traverse_syntax(trav, syn->fcallexpr_expression);
            traverse_vector(trav, syn->fcallexpr_args);
            break;
        }

        // SC_INTRINSIC_CALL_EXPRESSION - fcallexpr
        case SC_INTRINSIC_CALL_EXPRESSION:
        {
            traverse_vector(trav, syn->icallexpr_args);
            break;
        }

        // SC_SUBSCRIPT_EXPRESSION - subsexpr
        case SC_SUBSCRIPT_EXPRESSION:
        {
            traverse_syntax(trav, syn->subsexpr_expression);
            traverse_syntax(trav, syn->subsexpr_index_expression);
            break;
        }

        // SC_TYPE_NAME - tn
        case SC_TYPE_NAME:
        {
            traverse_vector(trav, syn->tn_specifier_qualifier_list);
            traverse_syntax(trav, syn->tn_declarator);
            break;
        }

        // memexpr (general member access expression)
        // SC_DEREFERENCE_MEMBER_EXPRESSION
        // SC_MEMBER_EXPRESSION
        case SC_DEREFERENCE_MEMBER_EXPRESSION:
        case SC_MEMBER_EXPRESSION:
        {
            traverse_syntax(trav, syn->memexpr_expression);
            traverse_syntax(trav, syn->memexpr_id);
            break;
        }

        default:
            break;
    }
    AFTER;
}

void traverse(syntax_traverser_t* trav)
{
    traverse_syntax(trav, trav->tlu);
}
