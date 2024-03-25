// this file contains some utility functions for working with the behemoth that is syntax_component_t.

#include <stdbool.h>
#include <string.h>

#include "cc.h"

// get through nested declarator to get to the first non-nested one
syntax_component_t* dig_declarator(syntax_component_t* declarator)
{
    if (!declarator) return NULL;
    if (declarator->sc3_type != DECLARATOR_NEST) return declarator;
    return dig_declarator(declarator->sc3_nested_declarator);
}

// gets through all nested declarators to get to the identifier declarator
syntax_component_t* dig_declarator_identifier(syntax_component_t* declarator)
{
    if (!declarator) return NULL;
    if (declarator->sc3_type == DECLARATOR_IDENTIFIER) return declarator;
    switch (declarator->sc3_type)
    {
        case DECLARATOR_NEST:
            return dig_declarator_identifier(declarator->sc3_nested_declarator);
        case DECLARATOR_POINTER:
            return dig_declarator_identifier(declarator->sc3_pointer_subdeclarator);
        case DECLARATOR_ARRAY:
            return dig_declarator_identifier(declarator->sc3_array_subdeclarator);
        case DECLARATOR_FUNCTION:
            return dig_declarator_identifier(declarator->sc3_function_subdeclarator);
        default:
            return NULL;
    }
}

void find_typedef(syntax_component_t** declaration_ref, syntax_component_t** declarator_ref, syntax_component_t* unit, char* identifier)
{
    *declaration_ref = NULL;
    *declarator_ref = NULL;
    for (unsigned i = 0; i < unit->sc0_external_declarations->size; ++i)
    {
        syntax_component_t* decl = vector_get(unit->sc0_external_declarations, i);
        if (decl->type != SYNTAX_COMPONENT_DECLARATION)
            continue;
        bool found_typedef_specifier = false;
        for (unsigned j = 0; j < decl->sc1_specifiers_qualifiers->size; ++j)
        {
            syntax_component_t* specifier = vector_get(decl->sc1_specifiers_qualifiers, j);
            if (specifier->sc2_type != SPECIFIER_QUALIFIER_STORAGE_CLASS)
                continue;
            if (specifier->sc2_storage_class_id == STORAGE_CLASS_TYPEDEF)
            {
                found_typedef_specifier = true;
                break;
            }
        }
        if (!found_typedef_specifier)
            continue;
        *declaration_ref = decl;
        for (unsigned j = 0; j < decl->sc1_declarators->size; ++j)
        {
            syntax_component_t* declarator = dig_declarator_identifier(vector_get(decl->sc1_declarators, j));
            if (declarator->sc3_type == DECLARATOR_IDENTIFIER &&
                !strcmp(declarator->sc3_identifier, identifier))
            {
                *declarator_ref = declarator;
                return;
            }
        }
    }
}

// ps - print structure
// pf - print field
#define ps(fmt, ...) { for (unsigned i = 0; i < indent; ++i) printer("    "); printer(fmt, ##__VA_ARGS__); }
#define pf(fmt, ...) { for (unsigned i = 0; i < indent + 1; ++i) printer("    "); printer(fmt, ##__VA_ARGS__); }

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
        ps("vector {}\n");
        return;
    }
    ps("vector {\n");
    for (unsigned i = 0; i < v->size; ++i)
    {
        syntax_component_t* s = vector_get(v, i);
        print_syntax_indented(s, indent + 1, printer);
    }
    ps("}\n");
}

static void print_vector_str_indented(vector_t* v, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!v)
    {
        ps("null\n");
        return;
    }
    if (!v->size)
    {
        ps("vector {}\n");
        return;
    }
    ps("vector {\n");
    for (unsigned i = 0; i < v->size; ++i)
        pf("\"%s\"\n", vector_get(v, i));
    ps("}\n");
}

static void print_syntax_indented(syntax_component_t* s, unsigned indent, int (*printer)(const char* fmt, ...))
{
    if (!s)
    {
        ps("null\n");
        return;
    }
    switch (s->type)
    {
        case SYNTAX_COMPONENT_TRANSLATION_UNIT:
        {
            ps("TRANSLATION_UNIT {\n");
            pf("external_declarations:\n");
            print_vector_indented(s->sc0_external_declarations, indent + 2, printer);
            break;
        }
        case SYNTAX_COMPONENT_DECLARATION:
        {
            ps("DECLARATION {\n");
            pf("specifiers_qualifiers:\n");
            print_vector_indented(s->sc1_specifiers_qualifiers, indent + 2, printer);
            pf("declarators:\n");
            print_vector_indented(s->sc1_declarators, indent + 2, printer);
            pf("initializers:\n");
            print_vector_indented(s->sc1_initializers, indent + 2, printer);
            break;
        }
        case SYNTAX_COMPONENT_SPECIFIER_QUALIFIER:
        {
            ps("SPECIFIER_QUALIFIER {\n");
            pf("type: %s\n", SPECIFIER_QUALIFIER_NAMES[s->sc2_type]);
            switch (s->sc2_type)
            {
                case SPECIFIER_QUALIFIER_ARITHMETIC_TYPE:
                    pf("arithmetic_type: %s\n", ARITHMETIC_TYPE_NAMES[s->sc2_arithmetic_type_id]);
                    break;
                case SPECIFIER_QUALIFIER_TYPEDEF:
                    pf("typedef: %s\n", dig_declarator_identifier(s->sc2_typedef_declarator)->sc3_identifier);
                    break;
                case SPECIFIER_QUALIFIER_STRUCT:
                    pf("struct:\n");
                    print_syntax_indented(s->sc2_struct, indent + 2, printer);
                    break;
                case SPECIFIER_QUALIFIER_UNION:
                    pf("union:\n");
                    print_syntax_indented(s->sc2_union, indent + 2, printer);
                    break;
                case SPECIFIER_QUALIFIER_ENUM:
                    pf("enum:\n");
                    print_syntax_indented(s->sc2_enum, indent + 2, printer);
                    break;
                case SPECIFIER_QUALIFIER_STORAGE_CLASS:
                    pf("storage_class: %s\n", STORAGE_CLASS_NAMES[s->sc2_storage_class_id]);
                    break;
                case SPECIFIER_QUALIFIER_FUNCTION:
                    pf("function: %s\n", FUNCTION_SPECIFIER_NAMES[s->sc2_function_id]);
                    break;
                case SPECIFIER_QUALIFIER_QUALIFIER:
                    pf("qualifier: %s\n", QUALIFIER_NAMES[s->sc2_qualifier_id]);
                    break;
            }
            break; 
        }
        case SYNTAX_COMPONENT_DECLARATOR:
        {
            ps("DECLARATOR {\n");
            pf("type: %s\n", DECLARATOR_NAMES[s->sc3_type]);
            switch (s->sc3_type)
            {
                case DECLARATOR_IDENTIFIER:
                    pf("identifier: \"%s\"\n", s->sc3_identifier);
                    break;
                case DECLARATOR_NEST:
                    pf("nested_declarator:\n");
                    print_syntax_indented(s->sc3_nested_declarator, indent + 2, printer);
                    break;
                case DECLARATOR_POINTER:
                    pf("pointer_qualifiers:\n");
                    print_vector_indented(s->sc3_pointer_qualifiers, indent + 2, printer);
                    pf("pointer_subdeclarator:\n");
                    print_syntax_indented(s->sc3_pointer_subdeclarator, indent + 2, printer);
                    break;
                case DECLARATOR_ARRAY:
                    pf("array_subdeclarator:\n");
                    print_syntax_indented(s->sc3_array_subdeclarator, indent + 2, printer);
                    pf("static: %s\n", BOOL_NAMES[s->sc3_array_static]);
                    pf("array_qualifiers:\n");
                    print_vector_indented(s->sc3_array_qualifiers, indent + 2, printer);
                    pf("array_expression:\n");
                    print_syntax_indented(s->sc3_array_expression, indent + 2, printer);
                    break;
                case DECLARATOR_FUNCTION:
                    pf("function_subdeclarator:\n");
                    print_syntax_indented(s->sc3_function_subdeclarator, indent + 2, printer);
                    if (s->sc3_function_parameters)
                    {
                        pf("function_paramaters:\n");
                        print_vector_indented(s->sc3_function_parameters, indent + 2, printer);
                    }
                    else
                    {
                        pf("function_identifiers:\n");
                        print_vector_str_indented(s->sc3_function_identifiers, indent + 2, printer);
                    }
                    break;
            }
            break;
        }
        case SYNTAX_COMPONENT_INITIALIZER:
        {
            ps("INITIALIZER {\n");
            pf("type: %s\n", INITIALIZER_NAMES[s->sc4_type]);
            switch (s->sc4_type)
            {
                case INITIALIZER_EXPRESSION:
                    pf("expression:\n");
                    print_syntax_indented(s->sc4_expression, indent + 2, printer);
                    break;
                case INITIALIZER_LIST:
                    pf("initializer_list:\n");
                    print_vector_indented(s->sc4_initializer_list, indent + 2, printer);
                    break;
                case INITIALIZER_DESIGNATION:
                    pf("designator_list:\n");
                    print_vector_indented(s->sc4_designator_list, indent + 2, printer);
                    pf("subinitializer:\n");
                    print_syntax_indented(s->sc4_subinitializer, indent + 2, printer);
                    break;
            }
            break;
        }
        case SYNTAX_COMPONENT_STRUCT_UNION:
        {
            ps("STRUCT_UNION {\n");
            pf("is_union: %s\n", BOOL_NAMES[s->sc5_is_union]);
            pf("identifier: \"%s\"\n", s->sc5_identifier);
            pf("declarations:\n");
            print_vector_indented(s->sc5_declarations, indent + 2, printer);
            break;
        }
        case SYNTAX_COMPONENT_ENUM:
        {
            ps("ENUM {\n");
            pf("identifier: \"%s\"\n", s->sc6_identifier);
            pf("enumerators:\n");
            print_vector_indented(s->sc6_enumerators, indent + 2, printer);
            break;
        }
        case SYNTAX_COMPONENT_ENUMERATOR:
        {
            ps("ENUMERATOR {\n");
            pf("identifier: \"%s\"\n", s->sc7_identifier);
            pf("const_expression:\n");
            print_syntax_indented(s->sc7_const_expression, indent + 2, printer);
            break;
        }
        case SYNTAX_COMPONENT_DESIGNATOR:
        {
            ps("DESIGNATOR: {\n");
            pf("array_designator_expression:\n");
            print_syntax_indented(s->sc8_array_designator_expression, indent + 2, printer);
            pf("struct_designator_identifier: \"%s\"\n", s->sc8_struct_designator_identifier);
            break;
        }
        case SYNTAX_COMPONENT_FUNCTION_DEFINITION:
        {
            ps("FUNCTION_DEFINITION {\n");
            pf("function_specifiers_qualifiers:\n");
            print_vector_indented(s->sc9_function_specifiers_qualifiers, indent + 2, printer);
            pf("function_declarator:\n");
            print_syntax_indented(s->sc9_function_declarator, indent + 2, printer);
            pf("function_body:\n");
            print_syntax_indented(s->sc9_function_body, indent + 2, printer);
            pf("function_declaration_list:\n");
            print_vector_indented(s->sc9_function_declaration_list, indent + 2, printer);
            break;
        }
        case SYNTAX_COMPONENT_STATEMENT:
        {
            ps("STATEMENT {\n");
            pf("type: %s\n", STATEMENT_NAMES[s->sc10_type]);
            switch (s->sc10_type)
            {
                case STATEMENT_LABELED:
                {
                    pf("labeled_type: %s\n", STATEMENT_LABELED_NAMES[s->sc10_labeled_type]);
                    switch (s->sc10_labeled_type)
                    {
                        case STATEMENT_LABELED_IDENTIFIER:
                            pf("identifier: \"%s\"\n", s->sc10_labeled_identifier);
                            break;
                        case STATEMENT_LABELED_CASE:
                            pf("const_expression:\n");
                            print_syntax_indented(s->sc10_labeled_case_const_expression, indent + 2, printer);
                            break;
                        case STATEMENT_LABELED_DEFAULT:
                            break;
                    }
                    break;
                }
                case STATEMENT_COMPOUND:
                    pf("compound_block_items:\n");
                    print_vector_indented(s->sc10_compound_block_items, indent + 2, printer);
                    break;
                case STATEMENT_EXPRESSION:
                    pf("expression:\n");
                    print_syntax_indented(s->sc10_expression, indent + 2, printer);
                    break;
                case STATEMENT_SELECTION:
                {
                    pf("selection_type: %s\n", STATEMENT_SELECTION_NAMES[s->sc10_selection_type]);
                    pf("condition:\n");
                    print_syntax_indented(s->sc10_selection_condition, indent + 2, printer);
                    switch (s->sc10_selection_type)
                    {
                        case STATEMENT_SELECTION_IF:
                            pf("then:\n");
                            print_syntax_indented(s->sc10_selection_then, indent + 2, printer);
                            break;
                        case STATEMENT_SELECTION_IF_ELSE:
                            pf("then:\n");
                            print_syntax_indented(s->sc10_selection_if_else_then, indent + 2, printer);
                            pf("else:\n");
                            print_syntax_indented(s->sc10_selection_if_else_else, indent + 2, printer);
                            break;
                        case STATEMENT_SELECTION_SWITCH:
                            pf("switch:\n");
                            print_syntax_indented(s->sc10_selection_switch, indent + 2, printer);
                            break;
                    }
                    break;
                }
                case STATEMENT_ITERATION:
                {
                    pf("iteration_type: %s\n", STATEMENT_ITERATION_NAMES[s->sc10_iteration_type]);
                    switch (s->sc10_iteration_type)
                    {
                        case STATEMENT_ITERATION_WHILE:
                            pf("condition:\n");
                            print_syntax_indented(s->sc10_iteration_while_condition, indent + 2, printer);
                            pf("action:\n");
                            print_syntax_indented(s->sc10_iteration_while_action, indent + 2, printer);
                            break;
                        case STATEMENT_ITERATION_DO_WHILE:
                            pf("action:\n");
                            print_syntax_indented(s->sc10_iteration_do_while_action, indent + 2, printer);
                            pf("condition:\n");
                            print_syntax_indented(s->sc10_iteration_do_while_condition, indent + 2, printer);
                            break;
                        case STATEMENT_ITERATION_FOR_DECLARATION:
                        case STATEMENT_ITERATION_FOR_EXPRESSION:
                        {
                            pf("init:\n");
                            print_syntax_indented(s->sc10_iteration_for_init, indent + 2, printer);
                            pf("condition:\n");
                            print_syntax_indented(s->sc10_iteration_for_condition, indent + 2, printer);
                            pf("update:\n");
                            print_syntax_indented(s->sc10_iteration_for_update, indent + 2, printer);
                            pf("action:\n");
                            print_syntax_indented(s->sc10_iteration_for_action, indent + 2, printer);
                            break;
                        }
                    }
                    break;
                }
                case STATEMENT_JUMP:
                {
                    pf("jump_type: %s\n", STATEMENT_JUMP_NAMES[s->sc10_jump_type]);
                    switch (s->sc10_jump_type)
                    {
                        // cont, break, return
                        case STATEMENT_JUMP_GOTO:
                        {
                            pf("identifier: \"%s\"\n", s->sc10_jump_goto_identifier);
                            break;
                        }
                        case STATEMENT_JUMP_CONTINUE:
                        case STATEMENT_JUMP_BREAK:
                            break; // noop
                        case STATEMENT_JUMP_RETURN:
                        {
                            pf("return_expression:\n");
                            print_syntax_indented(s->sc10_jump_return_expression, indent + 2, printer);
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }

        case SYNTAX_COMPONENT_EXPRESSION:
        {
            ps("EXPRESSION {\n");
            pf("type: %s\n", EXPRESSION_NAMES[s->sc11_type]);
            pf("operator_id: %d\n", s->sc11_operator_id);
            switch (s->sc11_type)
            {
                case EXPRESSION_PRIMARY:
                {
                    pf("primary_type: %s\n", EXPRESSION_PRIMARY_NAMES[s->sc11_primary_type]);
                    switch (s->sc11_primary_type)
                    {
                        case EXPRESSION_PRIMARY_IDENTIFIER:
                            pf("identifier: %s\n", s->sc11_primary_identifier);
                            break;
                        case EXPRESSION_PRIMARY_INTEGER_CONSTANT:
                            pf("integer_constant: %lld\n", s->sc11_primary_integer_constant);
                            break;
                        case EXPRESSION_PRIMARY_FLOATING_CONSTANT:
                            pf("floating_constant: %Lf\n", s->sc11_primary_floating_constant);
                            break;
                        case EXPRESSION_PRIMARY_STRING_LITERAL:
                            pf("string_literal: %s\n", s->sc11_primary_string_literal);
                            break;
                        case EXPRESSION_PRIMARY_NEST:
                            pf("nested_expression:\n");
                            print_syntax_indented(s->sc11_primary_nested_expression, indent + 2, printer);
                            break;
                    }
                    break;
                }
                case EXPRESSION_POSTFIX:
                {
                    pf("postfix_type: %s\n", EXPRESSION_POSTFIX_NAMES[s->sc11_postfix_type]);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_postfix_nested_expression, indent + 2, printer);
                    switch (s->sc11_postfix_type)
                    {
                        case EXPRESSION_POSTFIX_COMPOUND_LITERAL:
                        {
                            pf("type_name:\n");
                            print_syntax_indented(s->sc11_postfix_type_name, indent + 2, printer);
                            pf("initializer_list:\n");
                            print_syntax_indented(s->sc11_postfix_initializer_list, indent + 2, printer);
                            break;
                        }
                        case EXPRESSION_POSTFIX_SUBSCRIPT:
                        {
                            pf("expression:\n");
                            print_syntax_indented(s->sc11_postfix_subscript_expression, indent + 2, printer);
                            break;
                        }
                        case EXPRESSION_POSTFIX_FUNCTION_CALL:
                        {
                            pf("argument_list:\n");
                            print_vector_indented(s->sc11_postfix_argument_list, indent + 2, printer);
                            break;
                        }
                        case EXPRESSION_POSTFIX_MEMBER:
                        case EXPRESSION_POSTFIX_PTR_MEMBER:
                        {
                            pf("identifier: %s\n", s->sc11_postfix_member_identifier);
                            break;
                        }
                    }
                    break;
                }
                case EXPRESSION_UNARY:
                {
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_unary_nested_expression, indent + 2, printer);
                    pf("cast_expression:\n");
                    print_syntax_indented(s->sc11_unary_cast_expression, indent + 2, printer);
                    pf("type_name:\n");
                    print_syntax_indented(s->sc11_unary_type, indent + 2, printer);
                    break;
                }
                case EXPRESSION_CAST:
                {
                    pf("type_name:\n");
                    print_syntax_indented(s->sc11_cast_type, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_cast_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_MULTIPLICATIVE:
                {
                    pf("cast_expression:\n");
                    print_syntax_indented(s->sc11_multiplicative_cast_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_multiplicative_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_ADDITIVE:
                {
                    pf("multiplicative_expression:\n");
                    print_syntax_indented(s->sc11_additive_multiplicative_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_additive_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_SHIFT:
                {
                    pf("additive_expression:\n");
                    print_syntax_indented(s->sc11_shift_additive_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_shift_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_RELATIONAL:
                {
                    pf("shift_expression:\n");
                    print_syntax_indented(s->sc11_relational_shift_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_relational_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_EQUALITY:
                {
                    pf("relational_expression:\n");
                    print_syntax_indented(s->sc11_equality_relational_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_equality_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_AND:
                {
                    pf("equality_expression:\n");
                    print_syntax_indented(s->sc11_and_equality_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_and_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_XOR:
                {
                    pf("and_expression:\n");
                    print_syntax_indented(s->sc11_xor_and_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_xor_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_OR:
                {
                    pf("xor_expression:\n");
                    print_syntax_indented(s->sc11_or_xor_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_or_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_LOGICAL_AND:
                {
                    pf("or_expression:\n");
                    print_syntax_indented(s->sc11_land_or_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_land_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_LOGICAL_OR:
                {
                    pf("land_expression:\n");
                    print_syntax_indented(s->sc11_lor_land_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_lor_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_CONDITIONAL:
                {
                    pf("lor_expression:\n");
                    print_syntax_indented(s->sc11_conditional_lor_expression, indent + 2, printer);
                    pf("then_expression:\n");
                    print_syntax_indented(s->sc11_conditional_then_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_conditional_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_ASSIGNMENT:
                {
                    pf("unary_expression:\n");
                    print_syntax_indented(s->sc11_assignment_unary_expression, indent + 2, printer);
                    pf("nested_expression:\n");
                    print_syntax_indented(s->sc11_assignment_nested_expression, indent + 2, printer);
                    break;
                }
                case EXPRESSION_ASSIGNMENT_LIST:
                {
                    pf("assignment_list:\n");
                    print_vector_indented(s->sc11_assignment_list, indent + 2, printer);
                    break;
                }
            }
            break;
        }

        case SYNTAX_COMPONENT_TYPE_NAME:
        {
            ps("TYPE_NAME {\n");
            pf("specifiers_qualifiers:\n");
            print_vector_indented(s->sc12_specifiers_qualifiers, indent + 2, printer);
            pf("declarator:\n");
            print_syntax_indented(s->sc12_declarator, indent + 2, printer);
            break;
        }

        case SYNTAX_COMPONENT_PARAMETER_DECLARATION:
        {
            ps("PARAMETER_DECLARATION {\n");
            pf("specifiers_qualifiers:\n");
            print_vector_indented(s->sc15_specifiers_qualifiers, indent + 2, printer);
            pf("declarator:\n");
            print_syntax_indented(s->sc15_declarator, indent + 2, printer);
            pf("ellipsis: %s\n", BOOL_NAMES[s->sc15_ellipsis]);
            break;
        }

        case SYNTAX_COMPONENT_STRUCT_DECLARATION:
        {
            ps("STRUCT_DECLARATION {\n");
            pf("specifiers_qualifiers:\n");
            print_vector_indented(s->sc16_specifiers_qualifiers, indent + 2, printer);
            pf("declarators:\n");
            print_vector_indented(s->sc16_declarators, indent + 2, printer);
            break;
        }

        case SYNTAX_COMPONENT_STRUCT_DECLARATOR:
        {
            ps("STRUCT_DECLARATOR {\n");
            pf("declarator:\n");
            print_syntax_indented(s->sc17_declarator, indent + 2, printer);
            pf("const_expression:\n");
            print_syntax_indented(s->sc17_const_expression, indent + 2, printer);
            break;
        }

        default:
            ps("unknown syntax component\n");
            return;
    }
    ps("}\n");
}

#undef ps
#undef pf

void print_syntax(syntax_component_t* s, int (*printer)(const char* fmt, ...))
{
    print_syntax_indented(s, 0, printer);
}