#ifndef CC_H
#define CC_H

#include <stdbool.h>
#include <stdio.h>

#define terminate(fmt, ...) { errorf(fmt, ## __VA_ARGS__); exit(1); }

#define LEXER_TOKEN_KEYWORD 0
#define LEXER_TOKEN_IDENTIFIER 1
#define LEXER_TOKEN_OPERATOR 2
#define LEXER_TOKEN_SEPARATOR 3
#define LEXER_TOKEN_INTEGER_CONSTANT 4
#define LEXER_TOKEN_FLOATING_CONSTANT 5
#define LEXER_TOKEN_CHARACTER_CONSTANT 6
#define LEXER_TOKEN_STRING_CONSTANT 7

#define SYNTAX_COMPONENT_TRANSLATION_UNIT 0
#define SYNTAX_COMPONENT_DECLARATION 1
#define SYNTAX_COMPONENT_SPECIFIER_QUALIFIER 2
#define SYNTAX_COMPONENT_DECLARATOR 3
#define SYNTAX_COMPONENT_INITIALIZER 4
#define SYNTAX_COMPONENT_STRUCT_UNION 5
#define SYNTAX_COMPONENT_ENUM 6
#define SYNTAX_COMPONENT_ENUMERATOR 7
#define SYNTAX_COMPONENT_DESIGNATOR 8
#define SYNTAX_COMPONENT_FUNCTION_DEFINITION 9
#define SYNTAX_COMPONENT_STATEMENT 10
#define SYNTAX_COMPONENT_EXPRESSION 11
#define SYNTAX_COMPONENT_TYPE_NAME 12
#define SYNTAX_COMPONENT_ABSTRACT_DECLARATOR 13
#define SYNTAX_COMPONENT_DIRECT_ABSTRACT_DECLARATOR 14

#define SPECIFIER_QUALIFIER_VOID 0
#define SPECIFIER_QUALIFIER_ARITHMETIC_TYPE 1
#define SPECIFIER_QUALIFIER_TYPEDEF 2
#define SPECIFIER_QUALIFIER_STRUCT 3
#define SPECIFIER_QUALIFIER_UNION 4
#define SPECIFIER_QUALIFIER_ENUM 5
#define SPECIFIER_QUALIFIER_STORAGE_CLASS 6
#define SPECIFIER_QUALIFIER_FUNCTION 7
#define SPECIFIER_QUALIFIER_QUALIFIER 8

#define DECLARATOR_IDENTIFIER 0
#define DECLARATOR_NEST 1
#define DECLARATOR_POINTER 2
#define DECLARATOR_ARRAY 3
#define DECLARATOR_FUNCTION 4

#define INITIALIZER_EXPRESSION 0
#define INITIALIZER_LIST 1

#define STATEMENT_LABELED 0
#define STATEMENT_COMPOUND 1
#define STATEMENT_EXPRESSION 2
#define STATEMENT_SELECTION 3
#define STATEMENT_ITERATION 4
#define STATEMENT_JUMP 5

#define STATEMENT_LABELED_IDENTIFIER 0
#define STATEMENT_LABELED_CASE 1
#define STATEMENT_LABELED_DEFAULT 2

#define STATEMENT_SELECTION_IF 0
#define STATEMENT_SELECTION_IF_ELSE 1
#define STATEMENT_SELECTION_SWITCH 2

#define STATEMENT_ITERATION_WHILE 0
#define STATEMENT_ITERATION_DO_WHILE 1
#define STATEMENT_ITERATION_FOR_EXPRESSION 2
#define STATEMENT_ITERATION_FOR_DECLARATION 3

#define STATEMENT_JUMP_GOTO 0
#define STATEMENT_JUMP_CONTINUE 1
#define STATEMENT_JUMP_BREAK 2
#define STATEMENT_JUMP_RETURN 3

#define EXPRESSION_IDENTIFIER 0
#define EXPRESSION_INTEGER_CONSTANT 1
#define EXPRESSION_FLOATING_CONSTANT 2
#define EXPRESSION_STRING_CONSTANT 3
#define EXPRESSION_NEST 4
#define EXPRESSION_SUBSCRIPT 5
#define EXPRESSION_FUNCTION_CALL 6
#define EXPRESSION_MEMBER_ACCESS 7
#define EXPRESSION_PTR_MEMBER_ACCESS 8
#define EXPRESSION_POSTFIX_INCREMENT 9
#define EXPRESSION_POSTFIX_DECREMENT 10
#define EXPRESSION_COMPOUND_LITERAL 11
#define EXPRESSION_PREFIX_INCREMENT 12
#define EXPRESSION_PREFIX_DECREMENT 13
#define EXPRESSION_UNARY_CAST 14
#define EXPRESSION_SIZEOF_EXPRESSION 15
#define EXPRESSION_SIZEOF_TYPE 16
#define EXPRESSION_CAST 17
#define EXPRESSION_MULTIPLICATION 18
#define EXPRESSION_DIVISION 19
#define EXPRESSION_MODULO 20
#define EXPRESSION_ADDITION 21
#define EXPRESSION_SUBTRACTION 22
#define EXPRESSION_SHIFT_LEFT 23
#define EXPRESSION_SHIFT_RIGHT 24
#define EXPRESSION_LESS 25
#define EXPRESSION_GREATER 26
#define EXPRESSION_LESS_EQUAL 27
#define EXPRESSION_GREATER_EQUAL 28
#define EXPRESSION_EQUAL 29
#define EXPRESSION_NOT_EQUAL 30
#define EXPRESSION_AND 31
#define EXPRESSION_XOR 32
#define EXPRESSION_OR 33
#define EXPRESSION_LOGICAL_AND 34
#define EXPRESSION_LOGICAL_OR 35
#define EXPRESSION_TERNARY 36
#define EXPRESSION_ASSIGNMENT 37

#define DIRECT_ABSTRACT_DECLARATOR_SUBSCRIPT 0
#define DIRECT_ABSTRACT_DECLARATOR_ASTERISK 1
#define DIRECT_ABSTRACT_DECLARATOR_PARAMETER_LIST 2

#define KEYWORDS_LEN 37
#define KEYWORD_AUTO 0
#define KEYWORD_BREAK 1
#define KEYWORD_CASE 2
#define KEYWORD_CHAR 3
#define KEYWORD_CONST 4
#define KEYWORD_CONTINUE 5
#define KEYWORD_DEFAULT 6
#define KEYWORD_DO 7
#define KEYWORD_DOUBLE 8
#define KEYWORD_ELSE 9
#define KEYWORD_ENUM 10
#define KEYWORD_EXTERN 11
#define KEYWORD_FLOAT 12
#define KEYWORD_FOR 13
#define KEYWORD_GOTO 14
#define KEYWORD_IF 15
#define KEYWORD_INLINE 16
#define KEYWORD_INT 17
#define KEYWORD_LONG 18
#define KEYWORD_REGISTER 19
#define KEYWORD_RESTRICT 20
#define KEYWORD_RETURN 21
#define KEYWORD_SHORT 22
#define KEYWORD_SIGNED 23
#define KEYWORD_SIZEOF 24
#define KEYWORD_STATIC 25
#define KEYWORD_STRUCT 26
#define KEYWORD_SWITCH 27
#define KEYWORD_TYPEDEF 28
#define KEYWORD_UNION 29
#define KEYWORD_UNSIGNED 30
#define KEYWORD_VOID 31
#define KEYWORD_VOLATILE 32
#define KEYWORD_WHILE 33
#define KEYWORD_BOOL 34
#define KEYWORD_COMPLEX 35
#define KEYWORD_IMAGINARY 36

#define ARITHMETIC_TYPE_CHAR 0
#define ARITHMETIC_TYPE_SIGNED_CHAR 1
#define ARITHMETIC_TYPE_UNSIGNED_CHAR 2
#define ARITHMETIC_TYPE_SHORT_INT 3
#define ARITHMETIC_TYPE_UNSIGNED_SHORT_INT 4
#define ARITHMETIC_TYPE_INT 5
#define ARITHMETIC_TYPE_UNSIGNED_INT 6
#define ARITHMETIC_TYPE_LONG_INT 7
#define ARITHMETIC_TYPE_UNSIGNED_LONG_INT 8
#define ARITHMETIC_TYPE_LONG_LONG_INT 9
#define ARITHMETIC_TYPE_UNSIGNED_LONG_LONG_INT 10
#define ARITHMETIC_TYPE_FLOAT 11
#define ARITHMETIC_TYPE_DOUBLE 12
#define ARITHMETIC_TYPE_LONG_DOUBLE 13
#define ARITHMETIC_TYPE_BOOL 14
#define ARITHMETIC_TYPE_FLOAT_COMPLEX 15
#define ARITHMETIC_TYPE_DOUBLE_COMPLEX 16
#define ARITHMETIC_TYPE_LONG_DOUBLE_COMPLEX 17
#define ARITHMETIC_TYPE_FLOAT_IMAGINARY 18
#define ARITHMETIC_TYPE_DOUBLE_IMAGINARY 19
#define ARITHMETIC_TYPE_LONG_DOUBLE_IMAGINARY 20

#define STORAGE_CLASS_TYPEDEF 0
#define STORAGE_CLASS_AUTO 1
#define STORAGE_CLASS_REGISTER 2
#define STORAGE_CLASS_STATIC 3
#define STORAGE_CLASS_EXTERN 4

#define QUALIFIER_CONST 0
#define QUALIFIER_VOLATILE 1
#define QUALIFIER_RESTRICT 2

#define FUNCTION_SPECIFIER_INLINE 0

typedef struct lexer_token_t
{
    unsigned type;
    unsigned row, col;
    union
    {
        unsigned keyword_id;
        char* string_value;
        unsigned operator_id;
        unsigned separator_id;
    };
    struct lexer_token_t* next;
} lexer_token_t;

typedef struct buffer_t
{
    char* data;
    unsigned capacity;
    unsigned size;
} buffer_t;

typedef struct vector_t
{
    void** data;
    unsigned capacity;
    unsigned size;
} vector_t;

// THE GREATEST STRUCT OF ALL TIME
typedef struct syntax_component_t
{
    unsigned type;
    union
    {
        // SYNTAX_COMPONENT_TRANSLATION_UNIT
        vector_t* sc0_external_declarations; // <syntax_component_t> (SYNTAX_COMPONENT_DECLARATION | SYNTAX_COMPONENT_FUNCTION_DEFINITION)

        // SYNTAX_COMPONENT_DECLARATION
        struct
        {
            vector_t* sc1_specifiers_qualifiers; // <syntax_component_t> (SYNTAX_COMPONENT_SPECIFIER_QUALIFIER)
            vector_t* sc1_declarators; // <syntax_component_t> (SYNTAX_COMPONENT_DECLARATOR)
            // the corresponding declarator for each initializer should be at the same index as the declarator
            vector_t* sc1_initializers; // <syntax_component_t> (SYNTAX_COMPONENT_INITIALIZER)
        };

        // SYNTAX_COMPONENT_SPECIFIER_QUALIFIER
        struct
        {
            unsigned sc2_type;
            union
            {
                unsigned sc2_arithmetic_type_id;
                struct syntax_component_t* sc2_typedef; // (SYNTAX_COMPONENT_DECLARATION)
                struct syntax_component_t* sc2_struct; // (SYNTAX_COMPONENT_STRUCT_UNION)
                struct syntax_component_t* sc2_union; // (SYNTAX_COMPONENT_STRUCT_UNION)
                struct syntax_component_t* sc2_enum; // (SYNTAX_COMPONENT_ENUM)
                unsigned sc2_storage_class_id;
                unsigned sc2_function_id;
                unsigned sc2_qualifier_id;
            };
        };

        // SYNTAX_COMPONENT_DECLARATOR
        struct
        {
            unsigned sc3_type;
            union
            {
                char* sc3_identifier;
                struct syntax_component_t* sc3_nested_declarator; // (SYNTAX_COMPONENT_DECLARATOR)
                struct
                {
                    vector_t* sc3_pointer_qualifiers; // <syntax_component_t> (SYNTAX_COMPONENT_SPECIFIER_QUALIFIER)
                    struct syntax_component_t* sc3_pointer_subdeclarator; // (SYNTAX_COMPONENT_DECLARATOR)
                };
                struct
                {
                    struct syntax_component_t* sc3_array_subdeclarator; // (SYNTAX_COMPONENT_DECLARATOR)
                    bool sc3_array_static;
                    vector_t* sc3_array_qualifiers; // <syntax_component_t> (SYNTAX_COMPONENT_SPECIFIER_QUALIFIER)
                    struct syntax_component_t* sc3_array_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                };
                struct
                {
                    struct syntax_component_t* sc3_function_subdeclarator; // (SYNTAX_COMPONENT_DECLARATOR)
                    union
                    {
                        vector_t* sc3_function_parameters; // <syntax_component_t> (SYNTAX_COMPONENT_DECLARATION)
                        vector_t* sc3_function_identifiers; // <char*>
                    };
                };
                
            };
        };

        // SYNTAX_COMPONENT_INITIALIZER
        struct
        {
            unsigned sc4_type;
            union
            {
                struct syntax_component_t* sc4_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                vector_t* sc4_initializer_list; // <syntax_component_t> (SYNTAX_COMPONENT_INITIALIZER)
                struct
                {
                    struct syntax_component_t* sc4_designator; // (SYNTAX_COMPONENT_EXPRESSION)
                    struct syntax_component_t* sc4_subinitializer; // (SYNTAX_COMPONENT_INITIALIZER)
                };
            };
        };

        // SYNTAX_COMPONENT_STRUCT_UNION
        struct
        {
            bool sc5_is_union;
            char* sc5_identifier;
            vector_t* sc5_declarations; // <syntax_component_t> (SYNTAX_COMPONENT_DECLARATION)
        };

        // SYNTAX_COMPONENT_ENUM
        struct
        {
            char* sc6_identifier;
            vector_t* sc6_enumerators; // <syntax_component_t> (SYNTAX_COMPONENT_ENUMERATOR)
        };

        // SYNTAX_COMPONENT_ENUMERATOR
        struct
        {
            char* sc7_identifier;
            struct syntax_component_t* sc7_const_expression; // (SYNTAX_COMPONENT_EXPRESSION)
        };

        // SYNTAX_COMPONENT_DESIGNATOR
        struct
        {
            struct syntax_component_t* sc8_array_designator_expression; // (SYNTAX_COMPONENT_EXPRESSION)
            char* sc8_struct_designator_identifier;
        };

        // SYNTAX_COMPONENT_FUNCTION_DEFINITION
        struct
        {
            struct syntax_component_t* sc9_function_declaration; // (SYNTAX_COMPONENT_DECLARATION)
            struct syntax_component_t* sc9_function_body; // (SYNTAX_COMPONENT_STATEMENT)
        };

        // SYNTAX_COMPONENT_STATEMENT
        struct
        {
            unsigned sc10_type;
            union
            {
                struct
                {
                    unsigned sc10_labeled_type;
                    union
                    {
                        char* sc10_labeled_identifier;
                        struct syntax_component_t* sc10_labeled_case_const_expression;
                    };
                    struct syntax_component_t* sc10_labeled_substatement; // (SYNTAX_COMPONENT_STATEMENT)
                };
                vector_t* sc10_compound_block_items; // <syntax_component_t> (SYNTAX_COMPONENT_DECLARATION | SYNTAX_COMPONENT_STATEMENT)
                struct syntax_component_t* sc10_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                struct
                {
                    unsigned sc10_selection_type;
                    struct syntax_component_t* sc10_selection_condition; // (SYNTAX_COMPONENT_EXPRESSION)
                    union
                    {
                        struct syntax_component_t* sc10_selection_then; // (SYNTAX_COMPONENT_STATEMENT)
                        struct
                        {
                            struct syntax_component_t* sc10_selection_if_else_then; // (SYNTAX_COMPONENT_STATEMENT)
                            struct syntax_component_t* sc10_selection_if_else_else; // (SYNTAX_COMPONENT_STATEMENT)
                        };
                        struct syntax_component_t* sc10_selection_switch; // (SYNTAX_COMPONENT_STATEMENT)
                    };
                };
                struct
                {
                    unsigned sc10_iteration_type;
                    union
                    {
                        struct
                        {
                            struct syntax_component_t* sc10_iteration_while_condition; // (SYNTAX_COMPONENT_EXPRESSION)
                            struct syntax_component_t* sc10_iteration_while_action; // (SYNTAX_COMPONENT_STATEMENT)
                        };
                        struct
                        {
                            struct syntax_component_t* sc10_iteration_do_while_action; // (SYNTAX_COMPONENT_STATEMENT)
                            struct syntax_component_t* sc10_iteration_do_while_condition; // (SYNTAX_COMPONENT_EXPRESSION)
                        };
                        struct
                        {
                            struct syntax_component_t* sc10_iteration_for_init; // (SYNTAX_COMPONENT_EXPRESSION | SYNTAX_COMPONENT_DECLARATION)
                            struct syntax_component_t* sc10_iteration_for_condition; // (SYNTAX_COMPONENT_EXPRESSION)
                            struct syntax_component_t* sc10_iteration_for_update; // (SYNTAX_COMPONENT_EXPRESSION)
                            struct syntax_component_t* sc10_iteration_for_action; // (SYNTAX_COMPONENT_STATEMENT)
                        };
                    };
                };
                struct
                {
                    unsigned sc10_jump_type;
                    union
                    {
                        char* sc10_jump_goto_identifier;
                        struct syntax_component_t* sc10_jump_return_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                    };
                };
            };
        };
        
        // SYNTAX_COMPONENT_EXPRESSION
        struct
        {
            unsigned sc11_type;
            union
            {
                char* sc11_identifier;
                unsigned long long sc11_integer_constant;
                long double sc11_floating_constant;
                long long* sc11_string_constant;
                struct syntax_component_t* sc11_nested_expression; // (SYNTAX_COMPONENT_EXPRESSION)

                // EXPRESSION_FUNCTION_CALL
                struct
                {
                    struct syntax_component_t* sc11_function;
                    vector_t* sc11_function_arguments; // <syntax_component_t> (SYNTAX_COMPONENT_EXPRESSION)
                };

                // EXPRESSION_COMPOUND_LITERAL
                struct
                {
                    struct syntax_component_t* sc11_compound_literal_type; // (SYNTAX_COMPONENT_TYPE_NAME)
                    vector_t* sc11_compound_literal_initializer_list; // <syntax_component_t> (SYNTAX_COMPONENT_INITIALIZER)
                };

                // EXPRESSION_UNARY_CAST
                struct
                {
                    unsigned sc11_unary_cast_operator_id;
                    struct syntax_component_t* sc11_unary_cast_expression; // (SYNTAX_COMPONENT_EXPRESSION) 
                };

                // EXPRESSION_SIZEOF_EXPRESSION
                // EXPRESSION_SIZEOF_TYPE
                struct syntax_component_t* sc11_sizeof_arg; // (SYNTAX_COMPONENT_TYPE_NAME | SYNTAX_COMPONENT_EXPRESSION)

                // EXPRESSION_CAST
                struct
                {
                    struct syntax_component_t* sc11_cast_type; // (SYNTAX_COMPONENT_TYPE_NAME)
                    struct syntax_component_t* sc11_cast_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                };

                // any unary operator
                struct
                {
                    unsigned sc11_unary_operator_id;
                    struct syntax_component_t* sc11_unary_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                };

                // any binary operator
                struct
                {
                    unsigned sc11_binary_operator_id;
                    struct syntax_component_t* sc11_binary_left_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                    struct syntax_component_t* sc11_binary_right_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                };

                // EXPRESSION_TERNARY
                struct
                {
                    struct syntax_component_t* sc11_ternary_condition; // (SYNTAX_COMPONENT_EXPRESSION)
                    struct syntax_component_t* sc11_ternary_then; // (SYNTAX_COMPONENT_EXPRESSION)
                    struct syntax_component_t* sc11_ternary_else; // (SYNTAX_COMPONENT_EXPRESSION)
                };
            };
        };

        // SYNTAX_COMPONENT_TYPE_NAME
        struct
        {
            vector_t* sc12_specifiers; // <syntax_component_t> (SYNTAX_COMPONENT_SPECIFIER)
            vector_t* sc12_qualifiers; // <unsigned>
            struct syntax_component_t* sc12_abstract_declarator; // (SYNTAX_COMPONENT_ABSTRACT_DECLARATOR)
        };

        // SYNTAX_COMPONENT_ABSTRACT_DECLARATOR
        struct
        {
            bool sc13_pointer;
            struct syntax_component_t* sc13_direct_abstract_declarator; // (SYNTAX_COMPONENT_DIRECT_ABSTRACT_DECLARATOR)
        };

        // SYNTAX_COMPONENT_DIRECT_ABSTRACT_DECLARATOR
        struct
        {
            unsigned sc14_type;
            struct syntax_component_t* sc14_nested_abstract_declarator; // (SYNTAX_COMPONENT_ABSTRACT_DECLARATOR)
            struct syntax_component_t* sc14_nested_direct_abstract_declarator; // (SYNTAX_COMPONENT_DIRECT_ABSTRACT_DECLARATOR)
            union
            {
                struct syntax_component_t* sc14_subscript_expression; // (SYNTAX_COMPONENT_EXPRESSION)
                vector_t* sc14_parameter_list; // <syntax_component_t> (SYNTAX_COMPONENT_DECLARATION)
            };
        };
    };
} syntax_component_t;

typedef struct binary_node_t
{
    void* value;
    struct binary_node_t* left;
    struct binary_node_t* right;
} binary_node_t;

typedef struct set_t
{
    binary_node_t* root;
    unsigned size;
    int (*comparator)(void*, void*);
} set_t;

/* log.c */
int infof(char* fmt, ...);
int warnf(char* fmt, ...);
int errorf(char* fmt, ...);

/* buffer.c */
buffer_t* buffer_init(void);
static buffer_t* buffer_resize(buffer_t* b, unsigned capacity);
buffer_t* buffer_append(buffer_t* b, char c);
buffer_t* buffer_append_str(buffer_t* b, char* str);
void buffer_delete(buffer_t* b);
char* buffer_export(buffer_t* b);

/* vector.c */
vector_t* vector_init(void);
vector_t* vector_add(vector_t* v, void* el);
void* vector_get(vector_t* v, unsigned index);
bool vector_contains(vector_t* v, void* el, int (*c)(void*, void*));
void vector_delete(vector_t* v);

/* set.c */
set_t* set_init(int (*comparator)(void*, void*));
set_t* set_add(set_t* s, void* value);
bool set_contains(set_t* s, void* value);
void set_delete(set_t* s);
void set_print(set_t* s, void (*printer)(void*));

/* const.c */
extern const char* KEYWORDS[37];

/* lex.c */
lexer_token_t* lex(FILE* file);
void lex_delete(lexer_token_t* start);

#endif