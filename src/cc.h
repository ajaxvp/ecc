#ifndef CC_H
#define CC_H

#include <stdbool.h>
#include <stdio.h>

#define debug in_debug()

#define terminate(fmt, ...) { errorf(fmt, ## __VA_ARGS__); exit(1); }
#define numargs(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

// these macros are used in circumstances where an error was produced by something that wasn't expected to error.
// for instance, if there was a mistake with an earlier stage like parsing, this might get thrown.
#define report_return { printf("bad (%d)\n", __LINE__); return; }
#define report_continue { printf("bad (%d)\n", __LINE__); continue; }
#define report_return_value(x) { printf("bad (%d)\n", __LINE__); return (x); }

#define VECTOR_FOR(type, var, vec) type var = vector_get((vec), 0); for (unsigned i = 0; i < vec->size; ++i, var = vector_get((vec), i))
#define deep_free_syntax_vector(vec, var) if (vec) { VECTOR_FOR(syntax_component_t*, var, (vec)) free_syntax(var, tlu); vector_delete((vec)); }
#define SYMBOL_TABLE_FOR_ENTRIES_START(KEY_VAR, VALUE_VAR, CONTAINER) \
    for (unsigned i = 0; i < (CONTAINER)->capacity; ++i) \
    { \
        if (!(CONTAINER)->key[i]) continue; \
        char* KEY_VAR = (CONTAINER)->key[i]; \
        symbol_t* VALUE_VAR = (CONTAINER)->value[i]; \

#define SYMBOL_TABLE_FOR_ENTRIES_END }

#define LEXER_TOKEN_KEYWORD 0
#define LEXER_TOKEN_IDENTIFIER 1
#define LEXER_TOKEN_OPERATOR 2
#define LEXER_TOKEN_SEPARATOR 3
#define LEXER_TOKEN_INTEGER_CONSTANT 4
#define LEXER_TOKEN_FLOATING_CONSTANT 5
#define LEXER_TOKEN_CHARACTER_CONSTANT 6
#define LEXER_TOKEN_STRING_CONSTANT 7

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

typedef enum c_keyword
{
    KW_AUTO,
    KW_BREAK,
    KW_CASE,
    KW_CHAR,
    KW_CONST,
    KW_CONTINUE,
    KW_DEFAULT,
    KW_DO,
    KW_DOUBLE,
    KW_ELSE,
    KW_ENUM,
    KW_EXTERN,
    KW_FLOAT,
    KW_FOR,
    KW_GOTO,
    KW_IF,
    KW_INLINE,
    KW_INT,
    KW_LONG,
    KW_REGISTER,
    KW_RESTRICT,
    KW_RETURN,
    KW_SHORT,
    KW_SIGNED,
    KW_SIZEOF,
    KW_STATIC,
    KW_STRUCT,
    KW_SWITCH,
    KW_TYPEDEF,
    KW_UNION,
    KW_UNSIGNED,
    KW_VOID,
    KW_VOLATILE,
    KW_WHILE,
    KW_BOOL,
    KW_COMPLEX,
    KW_IMAGINARY,
    KW_ELEMENTS,
} c_keyword_t;

typedef enum preprocessor_token_type
{
    PPT_HEADER_NAME,
    PPT_IDENTIFIER,
    PPT_PP_NUMBER,
    PPT_CHARACTER_CONSTANT,
    PPT_STRING_LITERAL,
    PPT_PUNCTUATOR,
    PPT_OTHER,
    PPT_COMMENT,
    PPT_WHITESPACE,
    PPT_NO_ELEMENTS
} preprocessor_token_type_t;

typedef enum punctuator_type
{
    P_LEFT_BRACKET,
    P_RIGHT_BRACKET,
    P_LEFT_PARENTHESIS,
    P_RIGHT_PARENTHESIS,
    P_LEFT_BRACE,
    P_RIGHT_BRACE,
    P_PERIOD,
    P_DEREFERENCE_MEMBER,
    P_INCREMENT,
    P_DECREMENT,
    P_AND,
    P_ASTERISK,
    P_PLUS,
    P_MINUS,
    P_TILDE,
    P_EXCLAMATION_POINT,
    P_SLASH,
    P_PERCENT,
    P_LEFT_SHIFT,
    P_RIGHT_SHIFT,
    P_LESS,
    P_GREATER,
    P_LESS_EQUAL,
    P_GREATER_EQUAL,
    P_EQUAL,
    P_INEQUAL,
    P_CARET,
    P_PIPE,
    P_LOGICAL_AND,
    P_LOGICAL_OR,
    P_QUESTION_MARK,
    P_COLON,
    P_SEMICOLON,
    P_ELLIPSIS,
    P_ASSIGNMENT,
    P_MULTIPLY_ASSIGNMENT,
    P_DIVIDE_ASSIGNMENT,
    P_MODULO_ASSIGNMENT,
    P_ADD_ASSIGNMENT,
    P_SUB_ASSIGNMENT,
    P_SHIFT_LEFT_ASSIGNMENT,
    P_SHIFT_RIGHT_ASSIGNMENT,
    P_BITWISE_AND_ASSIGNMENT,
    P_BITWISE_XOR_ASSIGNMENT,
    P_BITWISE_OR_ASSIGNMENT,
    P_COMMA,
    P_HASH,
    P_DOUBLE_HASH,
    P_DIGRAPH_LEFT_BRACKET,
    P_DIGRAPH_RIGHT_BRACKET,
    P_DIGRAPH_LEFT_BRACE,
    P_DIGRAPH_RIGHT_BRACE,
    P_DIGRAPH_HASH,
    P_DIGRAPH_DOUBLE_HASH,
    P_NO_ELEMENTS
} punctuator_type_t;

typedef enum token_type
{
    T_KEYWORD,
    T_IDENTIFIER,
    T_INTEGER_CONSTANT,
    T_FLOATING_CONSTANT,
    T_CHARACTER_CONSTANT,
    T_STRING_LITERAL,
    T_PUNCTUATOR
} token_type_t;

typedef struct symbol_table_t symbol_table_t;
typedef struct syntax_traverser syntax_traverser_t;
typedef struct analysis_error analysis_error_t;
typedef struct syntax_component_t syntax_component_t;
typedef struct preprocessor_token preprocessor_token_t;

typedef struct preprocessor_token
{
    preprocessor_token_type_t type;
    unsigned row, col;
    preprocessor_token_t* next;
    union
    {
        // PPT_HEADER_NAME
        struct
        {
            bool quote_delimited;
            char* name;
        } header_name;

        // PPT_IDENTIFIER
        char* identifier;

        // PPT_PP_NUMBER
        char* pp_number;

        // PPT_CHARACTER_CONSTANT
        struct
        {
            int value;
            bool wide;
        } character_constant;

        // PPT_STRING_LITERAL
        struct
        {
            char* value;
            bool wide;
        } string_literal;

        // PPT_PUNCTUATOR
        punctuator_type_t punctuator;

        // PPT_WHITESPACE
        char* whitespace;

        // PPT_COMMENT
        char* comment;

        // PPT_OTHER
        unsigned char other;
    };
} preprocessor_token_t;

typedef struct token_t
{
    token_type_t type;
    unsigned row, col;
    token_t* next;
    union
    {
        // T_KEYWORD
        c_keyword_t keyword;

        // T_IDENTIFIER
        char* identifier;

        // T_INTEGER_CONSTANT
        struct
        {
            unsigned long long value;
            c_type_class_t class;
        } integer_constant;

        // T_FLOATING_CONSTANT
        struct
        {
            long double value;
            c_type_class_t class;
        } floating_constant;

        // T_CHARACTER_CONSTANT
        struct
        {
            int value;
            bool wide;
        } character_constant;

        // T_STRING_LITERAL
        struct
        {
            char* value;
            bool wide;
        } string_literal;

        // T_PUNCTUATOR
        punctuator_type_t punctuator;
    };
} token_t;

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

typedef enum constant_expression_type
{
    CE_ANY,
    CE_INTEGER,
    CE_ARITHMETIC,
    CE_ADDRESS
} constant_expression_type_t;

typedef enum c_type_class
{
    CTC_BOOL,
    CTC_CHAR,
    CTC_SIGNED_CHAR,
    CTC_SHORT_INT,
    CTC_INT,
    CTC_LONG_INT,
    CTC_LONG_LONG_INT,
    CTC_UNSIGNED_CHAR,
    CTC_UNSIGNED_SHORT_INT,
    CTC_UNSIGNED_INT,
    CTC_UNSIGNED_LONG_INT,
    CTC_UNSIGNED_LONG_LONG_INT,
    CTC_FLOAT,
    CTC_DOUBLE,
    CTC_LONG_DOUBLE,
    CTC_FLOAT_COMPLEX,
    CTC_DOUBLE_COMPLEX,
    CTC_LONG_DOUBLE_COMPLEX,
    CTC_FLOAT_IMAGINARY,
    CTC_DOUBLE_IMAGINARY,
    CTC_LONG_DOUBLE_IMAGINARY,
    CTC_ENUMERATED,
    CTC_VOID,
    CTC_ARRAY,
    CTC_STRUCTURE,
    CTC_UNION,
    CTC_FUNCTION,
    CTC_POINTER,
    CTC_LABEL,
    CTC_ERROR
} c_type_class_t;

#define BOOL_MAX (unsigned char) 0xff
#define CHAR_MAX (signed char) 0x7f
#define SIGNED_CHAR_MAX (signed char) 0x7f
#define UNSIGNED_CHAR_MAX (unsigned char) 0xff
#define SHORT_INT_MAX (short) 0x7fff
#define UNSIGNED_SHORT_INT_MAX (unsigned short) 0xffff
#define INT_MAX 0x7fffffff
#define UNSIGNED_INT_MAX 0xffffffffU
#define LONG_INT_MAX 0x7fffffffffffffffL
#define UNSIGNED_LONG_INT_MAX 0xffffffffffffffffUL
#define LONG_LONG_INT_MAX 0x7fffffffffffffffLL
#define UNSIGNED_LONG_LONG_INT_MAX 0xffffffffffffffffULL

#define C_TYPE_SIZE_T CTC_UNSIGNED_LONG_INT
#define C_TYPE_PTRSIZE_T CTC_LONG_INT
#define C_TYPE_WCHAR_T CTC_INT

typedef struct c_type c_type_t;

typedef struct c_type
{
    c_type_class_t class;
    unsigned char qualifiers;
    c_type_t* derived_from;
    union
    {
        struct
        {
            syntax_component_t* length_expression;
            bool unspecified_size;
        } array;
        struct
        {
            char* name;
            vector_t* member_types; // <c_type_t*>
            vector_t* member_names; // <char*>
            vector_t* member_bitfields; // <syntax_component_t*>
        } struct_union;
        struct
        {
            vector_t* param_types; // <c_type_t*>
            bool variadic;
        } function;
        struct
        {
            char* name;
            vector_t* constant_names; // <char*>
            vector_t* constant_expressions; // <syntax_component_t*>
        } enumerated;
    };
} c_type_t;

typedef enum storage_duration
{
    AUTOMATIC,
    STATIC,
    ALLOCATED
} storage_duration_t;

typedef enum c_namespace_class
{
    NSC_LABEL,
    NSC_STRUCT,
    NSC_UNION,
    NSC_ENUM,
    NSC_STRUCT_MEMBER,
    NSC_UNION_MEMBER,
    NSC_ORDINARY
} c_namespace_class_t;

typedef struct c_namespace
{
    c_namespace_class_t class;
    c_type_t* struct_union_type;
} c_namespace_t;

typedef enum storage_class_specifier
{
    SCS_TYPEDEF = 0,
    SCS_AUTO,
    SCS_REGISTER,
    SCS_STATIC,
    SCS_EXTERN
} storage_class_specifier_t;

typedef enum basic_type_specifier
{
    BTS_VOID = 0,
    BTS_CHAR,
    BTS_SHORT,
    BTS_INT,
    BTS_LONG,
    BTS_FLOAT,
    BTS_DOUBLE,
    BTS_SIGNED,
    BTS_UNSIGNED,
    BTS_BOOL,
    BTS_COMPLEX,
    BTS_IMAGINARY,

    BTS_NO_ELEMENTS
} basic_type_specifier_t;

typedef enum type_qualifier
{
    TQ_CONST = 0,
    TQ_RESTRICT,
    TQ_VOLATILE
} type_qualifier_t;

#define TQ_B_CONST (1 << TQ_CONST)
#define TQ_B_RESTRICT (1 << TQ_RESTRICT)
#define TQ_B_VOLATILE (1 << TQ_VOLATILE)

typedef enum function_specifier
{
    FS_INLINE = 0
} function_specifier_t;

typedef enum struct_or_union
{
    SOU_STRUCT = 0,
    SOU_UNION
} struct_or_union_t;

/* !IMPORTANT! DON'T MAKE CHANGES TO THIS ORDER W/O CHANGING THE CONST.C NAME ORDER */
typedef enum syntax_component_type_t
{
    SC_UNKNOWN = 0, // unk
    SC_ERROR, // err
    SC_TRANSLATION_UNIT, // tlu
    SC_FUNCTION_DEFINITION, // fdef
    SC_DECLARATION, // decl
    SC_INIT_DECLARATOR, // ideclr
    SC_STORAGE_CLASS_SPECIFIER, // scs
    SC_BASIC_TYPE_SPECIFIER, // bts
    SC_STRUCT_UNION_SPECIFIER, // sus
    SC_ENUM_SPECIFIER,
    SC_TYPE_QUALIFIER,
    SC_FUNCTION_SPECIFIER,
    SC_ENUMERATOR,
    SC_IDENTIFIER,
    SC_DECLARATOR,
    SC_POINTER,
    SC_ARRAY_DECLARATOR,
    SC_FUNCTION_DECLARATOR,
    SC_PARAMETER_DECLARATION,
    SC_ABSTRACT_DECLARATOR,
    SC_ABSTRACT_ARRAY_DECLARATOR,
    SC_ABSTRACT_FUNCTION_DECLARATOR,
    SC_LABELED_STATEMENT,
    SC_COMPOUND_STATEMENT,
    SC_EXPRESSION_STATEMENT,
    SC_IF_STATEMENT,
    SC_SWITCH_STATEMENT,
    SC_DO_STATEMENT,
    SC_WHILE_STATEMENT,
    SC_FOR_STATEMENT,
    SC_GOTO_STATEMENT,
    SC_CONTINUE_STATEMENT,
    SC_BREAK_STATEMENT,
    SC_RETURN_STATEMENT,
    SC_INITIALIZER_LIST,
    SC_DESIGNATION,
    SC_EXPRESSION,
    SC_ASSIGNMENT_EXPRESSION,
    SC_LOGICAL_OR_EXPRESSION,
    SC_LOGICAL_AND_EXPRESSION,
    SC_BITWISE_OR_EXPRESSION,
    SC_BITWISE_XOR_EXPRESSION,
    SC_BITWISE_AND_EXPRESSION,
    SC_EQUALITY_EXPRESSION,
    SC_INEQUALITY_EXPRESSION,
    SC_GREATER_EQUAL_EXPRESSION,
    SC_LESS_EQUAL_EXPRESSION,
    SC_GREATER_EXPRESSION,
    SC_LESS_EXPRESSION,
    SC_BITWISE_RIGHT_EXPRESSION,
    SC_BITWISE_LEFT_EXPRESSION,
    SC_SUBTRACTION_EXPRESSION,
    SC_ADDITION_EXPRESSION,
    SC_MODULAR_EXPRESSION,
    SC_DIVISION_EXPRESSION,
    SC_MULTIPLICATION_EXPRESSION,
    SC_CONDITIONAL_EXPRESSION,
    SC_CAST_EXPRESSION,
    SC_PREFIX_INCREMENT_EXPRESSION,
    SC_PREFIX_DECREMENT_EXPRESSION,
    SC_REFERENCE_EXPRESSION,
    SC_DEREFERENCE_EXPRESSION,
    SC_PLUS_EXPRESSION,
    SC_MINUS_EXPRESSION,
    SC_COMPLEMENT_EXPRESSION,
    SC_NOT_EXPRESSION,
    SC_SIZEOF_EXPRESSION,
    SC_SIZEOF_TYPE_EXPRESSION,
    SC_INITIALIZER_LIST_EXPRESSION,
    SC_POSTFIX_INCREMENT_EXPRESSION,
    SC_POSTFIX_DECREMENT_EXPRESSION,
    SC_DEREFERENCE_MEMBER_EXPRESSION,
    SC_MEMBER_EXPRESSION,
    SC_FUNCTION_CALL_EXPRESSION,
    SC_SUBSCRIPT_EXPRESSION,
    SC_TYPE_NAME,
    SC_TYPEDEF_NAME,
    SC_ENUMERATION_CONSTANT,
    SC_FLOATING_CONSTANT,
    SC_INTEGER_CONSTANT,
    SC_CHARACTER_CONSTANT,
    SC_STRING_LITERAL,
    SC_STRUCT_DECLARATION,
    SC_STRUCT_DECLARATOR,
    SC_CONSTANT_EXPRESSION,
    SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION,
    SC_DIVISION_ASSIGNMENT_EXPRESSION,
    SC_MODULAR_ASSIGNMENT_EXPRESSION,
    SC_ADDITION_ASSIGNMENT_EXPRESSION,
    SC_SUBTRACTION_ASSIGNMENT_EXPRESSION,
    SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION,
    SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION,
    SC_BITWISE_AND_ASSIGNMENT_EXPRESSION,
    SC_BITWISE_OR_ASSIGNMENT_EXPRESSION,
    SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION,

    SC_NO_ELEMENTS
} syntax_component_type_t;

// SC_TYPE_SPECIFIER = SC_BASIC_TYPE_SPECIFIER | SC_STRUCT_UNION_SPECIFIER | SC_ENUM_SPECIFIER | SC_TYPEDEF_NAME
// SC_DIRECT_DECLARATOR = SC_IDENTIFIER | SC_DECLARATOR | SC_ARRAY_DECLARATOR | SC_FUNCTION_DECLARATOR
// SC_DIRECT_ABSTRACT_DECLARATOR = SC_ABSTRACT_DECLARATOR | SC_ABSTRACT_ARRAY_DECLARATOR | SC_ABSTRACT_FUNCTION_DECLARATOR
// SC_STATEMENT = SC_LABELED_STATEMENT | SC_COMPOUND_STATEMENT | SC_EXPRESSION_STATEMENT | SC_SELECTION_STATEMENT | SC_ITERATION_STATEMENT | SC_JUMP_STATEMENT
// SC_BLOCK_ITEM = SC_DECLARATION | SC_STATEMENT
// SC_SELECTION_STATEMENT = SC_IF_STATEMENT | SC_SWITCH_STATEMENT
// SC_ITERATION_STATEMENT = SC_DO_STATEMENT | SC_WHILE_STATEMENT | SC_FOR_STATEMENT
// SC_JUMP_STATEMENT = SC_GOTO_STATEMENT | SC_CONTINUE_STATEMENT | SC_BREAK_STATEMENT | SC_RETURN_STATEMENT
// SC_INITIALIZER = SC_ASSIGNMENT_EXPRESSION | SC_INITIALIZER_LIST
// SC_DESIGNATOR = SC_IDENTIFIER | SC_CONSTANT_EXPRESSION
// SC_PRIMARY_EXPRESSION = SC_IDENTIFIER | SC_CONSTANT | SC_STRING_LITERAL | SC_EXPRESSION
// SC_CONSTANT_EXPRESSION = SC_CONDITIONAL_EXPRESSION
// SC_CONSTANT = SC_INTEGER_CONSTANT | SC_FLOATING_CONSTANT | SC_ENUMERATION_CONSTANT | SC_CHARACTER_CONSTANT

// THE GREATEST STRUCT OF ALL TIME
typedef struct syntax_component_t
{
    syntax_component_type_t type;
    c_type_t* ctype;
    unsigned row, col;
    struct syntax_component_t* parent;
    union
    {
        // SC_TRANSLATION_UNIT - tlu
        struct
        {
            vector_t* tlu_external_declarations;
            vector_t* tlu_errors; // <syntax_component_t> (SC_ERROR)
            symbol_table_t* tlu_st;
        };

        // SC_FUNCTION_DEFINITION - fdef
        struct
        {
            vector_t* fdef_declaration_specifiers; // <syntax_component_t> (SC_STORAGE_CLASS_SPECIFIER | SC_TYPE_SPECIFIER | SC_TYPE_QUALIFIER | SC_FUNCTION_SPECIFIER)
            struct syntax_component_t* fdef_declarator; // SC_DECLARATOR
            vector_t* fdef_knr_declarations; // <syntax_component_t> (SC_DECLARATION)
            struct syntax_component_t* fdef_body; // SC_COMPOUND_STATEMENT
        };

        // SC_DECLARATION - decl
        struct
        {
            vector_t* decl_declaration_specifiers; // <syntax_component_t> (SC_STORAGE_CLASS_SPECIFIER | SC_TYPE_SPECIFIER | SC_TYPE_QUALIFIER | SC_FUNCTION_SPECIFIER)
            vector_t* decl_init_declarators; // <syntax_component_t> (SC_INIT_DECLARATOR)
        };

        // SC_INIT_DECLARATOR - ideclr
        struct
        {
            struct syntax_component_t* ideclr_declarator; // SC_DECLARATOR
            struct syntax_component_t* ideclr_initializer; // SC_INITIALIZER
        };

        // SC_STORAGE_CLASS_SPECIFIER - scs
        storage_class_specifier_t scs;

        // SC_BASIC_TYPE_SPECIFIER - bts
        basic_type_specifier_t bts;

        // SC_TYPE_QUALIFIER - tq
        type_qualifier_t tq;

        // SC_FUNCTION_SPECIFIER - fs
        function_specifier_t fs;

        // SC_STRUCT_UNION_SPECIFIER - sus
        struct
        {
            struct_or_union_t sus_sou;
            struct syntax_component_t* sus_id; // SC_IDENTIFIER
            vector_t* sus_declarations; // <syntax_component_t> (SC_STRUCT_DECLARATION)
        };

        // SC_STRUCT_DECLARATION - sdecl
        struct
        {
            vector_t* sdecl_specifier_qualifier_list; // <syntax_component_t> (SC_TYPE_SPECIFIER | SC_TYPE_QUALIFIER)
            vector_t* sdecl_declarators; // <syntax_component_t> (SC_STRUCT_DECLARATOR)
        };

        // SC_STRUCT_DECLARATOR - sdeclr
        struct
        {
            struct syntax_component_t* sdeclr_declarator; // SC_DECLARATOR
            struct syntax_component_t* sdeclr_bits_expression; // SC_CONSTANT_EXPRESSION
        };

        // SC_ENUM_SPECIFIER - enums
        struct
        {
            struct syntax_component_t* enums_id; // SC_IDENTIFIER
            vector_t* enums_enumerators; // <syntax_component_t> (SC_ENUMERATOR)
        };

        // SC_ENUMERATOR - enumr
        struct
        {
            struct syntax_component_t* enumr_constant; // SC_ENUMERATION_CONSTANT
            struct syntax_component_t* enumr_expression; // SC_CONSTANT_EXPRESSION
        };

        // id (general identifier)
        // SC_IDENTIFIER
        // SC_ENUMERATION_CONSTANT
        // SC_TYPEDEF_NAME
        char* id;

        // con (general constant)
        // SC_STRING_LITERAL
        // SC_FLOATING_CONSTANT
        // SC_INTEGER_CONSTANT
        // SC_CHARACTER_CONSTANT
        char* con;

        // SC_DECLARATOR - declr
        struct
        {
            vector_t* declr_pointers; // <syntax_component_t> (SC_POINTER)
            struct syntax_component_t* declr_direct; // SC_DIRECT_DECLARATOR
        };

        // SC_POINTER - ptr
        vector_t* ptr_type_qualifiers; // <syntax_component_t> (SC_TYPE_QUALIFIER)

        // SC_ARRAY_DECLARATOR - adeclr
        struct
        {
            struct syntax_component_t* adeclr_direct; // SC_DIRECT_DECLARATOR
            bool adeclr_is_static;
            bool adeclr_unspecified_size;
            vector_t* adeclr_type_qualifiers; // <syntax_component_t> (SC_TYPE_QUALIFIER)
            struct syntax_component_t* adeclr_length_expression; // SC_ASSIGNMENT_EXPRESSION
        };

        // SC_FUNCTION_DECLARATOR - fdeclr
        struct
        {
            struct syntax_component_t* fdeclr_direct; // SC_DIRECT_DECLARATOR
            bool fdeclr_ellipsis;
            vector_t* fdeclr_parameter_declarations; // <syntax_component_t> (SC_PARAMETER_DECLARATION)
            vector_t* fdeclr_knr_identifiers; // <syntax_component_t> (SC_IDENTIFIER)
        };

        // SC_PARAMETER_DECLARATION - pdecl
        struct
        {
            vector_t* pdecl_declaration_specifiers; // <syntax_component_t> (SC_STORAGE_CLASS_SPECIFIER | SC_TYPE_SPECIFIER | SC_TYPE_QUALIFIER | SC_FUNCTION_SPECIFIER)
            struct syntax_component_t* pdecl_declr; // (SC_DECLARATOR | SC_ABSTRACT_DECLARATOR)
        };

        // SC_ABSTRACT_DECLARATOR - abdeclr
        struct
        {
            vector_t* abdeclr_pointers; // <syntax_component_t> (SC_POINTER)
            struct syntax_component_t* abdeclr_direct; // SC_DIRECT_ABSTRACT_DECLARATOR
        };

        // SC_ABSTRACT_ARRAY_DECLARATOR - abadeclr
        struct
        {
            struct syntax_component_t* abadeclr_direct; // SC_DIRECT_ABSTRACT_DECLARATOR
            bool abadeclr_unspecified_size;
            struct syntax_component_t* abadeclr_length_expression; // SC_ASSIGNMENT_EXPRESSION
        };

        // SC_ABSTRACT_FUNCTION_DECLARATOR - abfdeclr
        struct
        {
            struct syntax_component_t* abfdeclr_direct; // SC_DIRECT_ABSTRACT_DECLARATOR
            bool abfdeclr_ellipsis;
            vector_t* abfdeclr_parameter_declarations; // <syntax_component_t> (SC_PARAMETER_DECLARATION)
        };

        // SC_LABELED_STATEMENT - lstmt
        struct
        {
            struct syntax_component_t* lstmt_id; // SC_IDENTIFIER
            struct syntax_component_t* lstmt_case_expression; // SC_CONSTANT_EXPRESSION
            struct syntax_component_t* lstmt_stmt; // SC_STATEMENT
        };

        // SC_COMPOUND_STATEMENT - cstmt
        vector_t* cstmt_block_items; // <syntax_component_t> (SC_BLOCK_ITEM)

        // SC_EXPRESSION_STATEMENT - estmt
        struct syntax_component_t* estmt_expression; // SC_EXPRESSION

        // SC_IF_STATEMENT - ifstmt
        struct
        {
            struct syntax_component_t* ifstmt_condition; // SC_EXPRESSION
            struct syntax_component_t* ifstmt_body; // SC_STATEMENT
            struct syntax_component_t* ifstmt_else; // SC_STATEMENT
        };

        // SC_SWITCH_STATEMENT - swstmt
        struct
        {
            struct syntax_component_t* swstmt_condition; // SC_EXPRESSION
            struct syntax_component_t* swstmt_body; // SC_STATEMENT
        };

        // SC_DO_STATEMENT - dostmt
        struct
        {
            struct syntax_component_t* dostmt_condition; // SC_EXPRESSION
            struct syntax_component_t* dostmt_body; // SC_STATEMENT
        };

        // SC_WHILE_STATEMENT - whstmt
        struct
        {
            struct syntax_component_t* whstmt_condition; // SC_EXPRESSION
            struct syntax_component_t* whstmt_body; // SC_STATEMENT
        };

        // SC_FOR_STATEMENT - forstmt
        struct
        {
            struct syntax_component_t* forstmt_init; // SC_DECLARATION | SC_EXPRESSION
            struct syntax_component_t* forstmt_condition; // SC_EXPRESSION
            struct syntax_component_t* forstmt_post; // SC_EXPRESSION
            struct syntax_component_t* forstmt_body; // SC_STATEMENT
        };

        // SC_GOTO_STATEMENT - gtstmt
        struct syntax_component_t* gtstmt_label_id; // SC_IDENTIFIER

        // SC_RETURN_STATEMENT - retstmt
        struct syntax_component_t* retstmt_expression; // SC_EXPRESSION

        // SC_INITIALIZER_LIST - inlist
        struct
        {
            vector_t* inlist_designations; // <syntax_component_t> (SC_DESIGNATION)
            vector_t* inlist_initializers; // <syntax_component_t> (SC_INITIALIZER)
        };

        // SC_DESIGNATION - desig
        vector_t* desig_designators; // <syntax_component_t> (SC_DESIGNATOR)

        // SC_EXPRESSION - expr
        vector_t* expr_expressions; // <syntax_component_t> (SC_ASSIGNMENT_EXPRESSION)

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
        struct
        {
            struct syntax_component_t* bexpr_lhs;
            struct syntax_component_t* bexpr_rhs;
        };

        // SC_CONDITIONAL_EXPRESSION - cexpr
        struct
        {
            struct syntax_component_t* cexpr_condition; // SC_LOGICAL_OR_EXPRESSION
            struct syntax_component_t* cexpr_if; // SC_EXPRESSION
            struct syntax_component_t* cexpr_else; // SC_CONDITIONAL_EXPRESSION
        };

        // SC_CAST_EXPRESSION - caexpr
        struct
        {
            struct syntax_component_t* caexpr_type_name; // SC_TYPE_NAME
            struct syntax_component_t* caexpr_operand;
        };

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
        struct syntax_component_t* uexpr_operand;

        // SC_INITIALIZER_LIST_EXPRESSION - inlexpr
        struct
        {
            struct syntax_component_t* inlexpr_type_name; // SC_TYPE_NAME
            struct syntax_component_t* inlexpr_inlist; // SC_INITIALIZER_LIST
        };

        // SC_FUNCTION_CALL_EXPRESSION - fcallexpr
        struct
        {
            struct syntax_component_t* fcallexpr_expression; // SC_POSTFIX_EXPRESSION
            vector_t* fcallexpr_args; // <syntax_component_t> (SC_ASSIGNMENT_EXPRESSION)
        };

        // SC_SUBSCRIPT_EXPRESSION - subsexpr
        struct
        {
            struct syntax_component_t* subsexpr_expression; // SC_POSTFIX_EXPRESSION
            struct syntax_component_t* subsexpr_index_expression; // SC_EXPRESSION
        };

        // SC_TYPE_NAME - tn
        struct
        {
            vector_t* tn_specifier_qualifier_list; // <syntax_component_t> (SC_TYPE_SPECIFIER | SC_TYPE_QUALIFIER)
            struct syntax_component_t* tn_declarator; // SC_ABSTRACT_DECLARATOR
        };

        // memexpr (general member access expression)
        // SC_DEREFERENCE_MEMBER_EXPRESSION
        // SC_MEMBER_EXPRESSION
        struct
        {
            struct syntax_component_t* memexpr_expression; // SC_POSTFIX_EXPRESSION
            struct syntax_component_t* memexpr_id; // SC_IDENTIFIER
        };

        // SC_ERROR - err
        struct
        {
            char* err_message;
            int err_depth;
        };
    };
} syntax_component_t;

/*

IMPORTANT NOTES:

An identifier can denote an object; a function; a tag or a member of a structure, union, or
enumeration; a typedef name; a label name; a macro name; or a macro parameter. The
same identifier can denote different entities at different points in the program.

There are four kinds of scopes: function, file, block, and function prototype.

A label name is the only kind of identifier that has function scope.

If the declarator or type specifier that declares the identifier appears outside of any block
or list of parameters, the identifier has file scope.

If the declarator or type specifier that declares the identifier appears inside a block or
within the list of parameter declarations in a function definition, the identifier has block scope.

If the declarator or type specifier that declares the identifier appears within the list of parameter declarations
in a function prototype (not part of a function definition), the identifier has function prototype scope

Structure, union, and enumeration tags have scope that begins just after the appearance of
the tag in a type specifier that declares the tag. Each enumeration constant has scope that
begins just after the appearance of its defining enumerator in an enumerator list. Any
other identifier has scope that begins just after the completion of its declarator.

*/

/*

function scope: defined by a SC_FUNCTION_DEFINITION syntax element
file scope: defined by a SC_TRANSLATION_UNIT syntax element
block scope: defined by a SC_STATEMENT syntax element (depends on the circumstance but yeh)
function prototype scope: defined by a SC_FUNCTION_DECLARATOR syntax element (only if not part of a function definition)

object: declared by a SC_DECLARATOR
function: declared by a SC_DECLARATOR
tag of struct, union, or enum: declared by a SC_TYPE_SPECIFIER

*/

typedef struct symbol_t
{
    syntax_component_t* declarer; // the declaring identifier for this symbol in the syntax tree
    c_type_t* type;
    struct symbol_t* next; // next symbol in list (if in a list, otherwise NULL)
} symbol_t;

typedef struct symbol_table_t
{
    char** key;
    symbol_t** value;
    unsigned size;
    unsigned capacity;
} symbol_table_t;

typedef struct analysis_error
{
    unsigned row, col;
    char* message;
    bool warning;
    analysis_error_t* next;
} analysis_error_t;

typedef void (*traversal_function)(syntax_traverser_t* trav, syntax_component_t* syn);

typedef struct syntax_traverser
{
    syntax_component_t* tlu;

    traversal_function default_before;
    traversal_function default_after;

    traversal_function before[SC_NO_ELEMENTS];
    traversal_function after[SC_NO_ELEMENTS];
} syntax_traverser_t;

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
int sninfof(char* buffer, size_t maxlen, char* fmt, ...);
int snwarnf(char* buffer, size_t maxlen, char* fmt, ...);
int snerrorf(char* buffer, size_t maxlen, char* fmt, ...);

/* buffer.c */
buffer_t* buffer_init(void);
buffer_t* buffer_append(buffer_t* b, char c);
buffer_t* buffer_append_str(buffer_t* b, char* str);
buffer_t* buffer_pop(buffer_t* b);
void buffer_delete(buffer_t* b);
char* buffer_export(buffer_t* b);

/* vector.c */
vector_t* vector_init(void);
vector_t* vector_add(vector_t* v, void* el);
void* vector_get(vector_t* v, unsigned index);
int vector_contains(vector_t* v, void* el, int (*c)(void*, void*));
vector_t* vector_copy(vector_t* v);
vector_t* vector_deep_copy(vector_t* v, void* (*copy_member)(void*));
bool vector_equals(vector_t* v1, vector_t* v2, bool (*equals)(void*, void*));
void vector_delete(vector_t* v);
void vector_deep_delete(vector_t* v, void (*deleter)(void*));

/* set.c */
set_t* set_init(int (*comparator)(void*, void*));
set_t* set_add(set_t* s, void* value);
bool set_contains(set_t* s, void* value);
void set_delete(set_t* s);
void set_print(set_t* s, void (*printer)(void*));

/* const.c */
extern const char* KEYWORDS[37];
extern const char* SYNTAX_COMPONENT_NAMES[95];
extern const char* SPECIFIER_QUALIFIER_NAMES[9];
extern const char* ARITHMETIC_TYPE_NAMES[21];
extern unsigned ARITHMETIC_TYPE_SIZES[21];
extern const char* BASIC_TYPE_SPECIFIER_NAMES[12];
extern const char* STORAGE_CLASS_NAMES[5];
extern const char* TYPE_QUALIFIER_NAMES[3];
extern const char* FUNCTION_SPECIFIER_NAMES[1];
extern const char* DECLARATOR_NAMES[5];
extern const char* INITIALIZER_NAMES[3];
extern const char* STATEMENT_NAMES[6];
extern const char* STATEMENT_LABELED_NAMES[3];
extern const char* STATEMENT_SELECTION_NAMES[3];
extern const char* STATEMENT_ITERATION_NAMES[4];
extern const char* STATEMENT_JUMP_NAMES[4];
extern const char* EXPRESSION_NAMES[17];
extern const char* EXPRESSION_POSTFIX_NAMES[7];
extern const char* EXPRESSION_PRIMARY_NAMES[5];
extern const char* ABSTRACT_DECLARATOR_NAMES[4];
extern const char* ABSTRACT_DECLARATOR_NAMES[4];
extern const char* BOOL_NAMES[2];
extern const char* LEXER_TOKEN_NAMES[8];
extern const char* C_TYPE_CLASS_NAMES[30];
extern const char* PP_TOKEN_NAMES[PPT_NO_ELEMENTS];
extern const char* PUNCTUATOR_STRING_REPRS[P_NO_ELEMENTS];

/* lex.c */
preprocessor_token_t* lex_new(FILE* file, bool dump_error);
void pp_token_delete_all(preprocessor_token_t* tokens);
void pp_token_print(preprocessor_token_t* token, int (*printer)(const char* fmt, ...));

// old
lexer_token_t* lex(FILE* file);
void lex_delete(lexer_token_t* start);
void print_token(lexer_token_t* tok, int (*printer)(const char* fmt, ...));
bool is_assignment_operator_token(lexer_token_t* tok);
bool is_unary_operator_token(lexer_token_t* tok);

/* parse.c */
syntax_component_t* parse(lexer_token_t* toks);

/* traverse.c */
syntax_traverser_t* traverse_init(syntax_component_t* tlu, size_t size);
void traverse_delete(syntax_traverser_t* trav);
void traverse(syntax_traverser_t* trav);

/* analyze.c */
analysis_error_t* analyze(syntax_component_t* tlu);
analysis_error_t* error_init(syntax_component_t* syn, bool warning, char* fmt, ...);
analysis_error_t* error_list_add(analysis_error_t* errors, analysis_error_t* err);
void error_delete(analysis_error_t* err);
void error_delete_all(analysis_error_t* errors);
size_t error_list_size(analysis_error_t* errors, bool include_warnings);
void dump_errors(analysis_error_t* errors);

/* emit.c */
bool emit(syntax_component_t* unit, FILE* out);

/* symbol.c */
symbol_t* symbol_init(syntax_component_t* declarer);
syntax_component_t* symbol_get_scope(symbol_t* sy);
void symbol_print(symbol_t* sy, int (*printer)(const char*, ...));
void symbol_delete(symbol_t* sy);
symbol_table_t* symbol_table_init(void);
symbol_t* symbol_table_add(symbol_table_t* t, char* k, symbol_t* sy);
symbol_t* symbol_table_get_all(symbol_table_t* t, char* k);
symbol_t* symbol_table_get_syn_id(symbol_table_t* t, syntax_component_t* id);
symbol_t* symbol_table_lookup(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns);
symbol_t* symbol_table_count(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns, size_t* count, bool* first);
symbol_t* symbol_table_remove(symbol_table_t* t, syntax_component_t* id);
void symbol_table_print(symbol_table_t* t, int (*printer)(const char*, ...));
void symbol_table_delete(symbol_table_t* t, bool free_contents);

/* syntax.c */
syntax_component_t* find_declarator_identifier(syntax_component_t* declarator);
unsigned count_specifiers(syntax_component_t* declaration, unsigned type);
unsigned get_declaration_size(syntax_component_t* s);
bool syntax_is_declarator_type(syntax_component_type_t type);
bool syntax_is_expression_type(syntax_component_type_t type);
bool syntax_is_abstract_declarator_type(syntax_component_type_t type);
bool syntax_is_typedef_name(syntax_component_t* id);
bool syntax_is_lvalue(syntax_component_t* syn);
bool can_evaluate(syntax_component_t* expr, constant_expression_type_t ce_type);
bool syntax_is_vla(syntax_component_t* declr);
bool syntax_is_known_size_array(syntax_component_t* declr);
bool syntax_is_modifiable_lvalue(syntax_component_t* syn);
bool syntax_is_tentative_definition(syntax_component_t* id);
size_t syntax_no_specifiers(vector_t* declspecs, syntax_component_type_t type);
void namespace_delete(c_namespace_t* ns);
vector_t* syntax_get_declspecs(syntax_component_t* id);
syntax_component_t* syntax_get_declarator_identifier(syntax_component_t* declr);
syntax_component_t* syntax_get_declarator_function_definition(syntax_component_t* declr);
syntax_component_t* syntax_get_declarator_declaration(syntax_component_t* declr);
syntax_component_t* syntax_get_full_declarator(syntax_component_t* declr);
syntax_component_t* syntax_get_translation_unit(syntax_component_t* syn);
c_namespace_t* syntax_get_namespace(syntax_component_t* id);
c_namespace_t get_basic_namespace(c_namespace_class_t nsc);
void find_typedef(syntax_component_t** declaration_ref, syntax_component_t** declarator_ref, syntax_component_t* unit, char* identifier);
void print_syntax(syntax_component_t* s, int (*printer)(const char* fmt, ...));
bool syntax_has_specifier(vector_t* specifiers, syntax_component_type_t sc_type, int type);
void free_syntax(syntax_component_t* syn, syntax_component_t* tlu);
unsigned long long process_integer_constant(syntax_component_t* syn, c_type_class_t* class);
unsigned long long evaluate_constant_expression(syntax_component_t* expr, c_type_class_t* class, constant_expression_type_t cexpr_type);
unsigned long long evaluate_enumeration_constant(syntax_component_t* enumr);

/* type.c */
c_type_t* make_basic_type(c_type_class_t class);
c_type_t* integer_promotions(c_type_t* ct);
void usual_arithmetic_conversions(c_type_t* t1, c_type_t* t2, c_type_t** conv_t1, c_type_t** conv_t2);
c_type_t* usual_arithmetic_conversions_result_type(c_type_t* t1, c_type_t* t2);
void type_delete(c_type_t* ct);
c_type_t* type_compose(c_type_t* t1, c_type_t* t2);
bool type_is_compatible(c_type_t* t1, c_type_t* t2);
bool type_is_compatible_ignore_qualifiers(c_type_t* t1, c_type_t* t2);
bool type_is_complete(c_type_t* ct);
bool type_is_object_type(c_type_t* ct);
unsigned char qualifiers_to_bitfield(vector_t* quals);
bool type_is_signed_integer_type(c_type_class_t class);
bool type_is_signed_integer(c_type_t* ct);
bool type_is_unsigned_integer_type(c_type_class_t class);
bool type_is_unsigned_integer(c_type_t* ct);
bool type_is_integer_type(c_type_class_t class);
bool type_is_integer(c_type_t* ct);
int get_integer_type_conversion_rank(c_type_class_t class);
int get_integer_conversion_rank(c_type_t* ct);
bool type_is_object_type(c_type_t* ct);
bool type_is_real_floating_type(c_type_class_t class);
bool type_is_real_floating(c_type_t* ct);
bool type_is_real_type(c_type_class_t class);
bool type_is_real(c_type_t* ct);
bool type_is_complex_type(c_type_class_t class);
bool type_is_complex(c_type_t* ct);
bool type_is_floating_type(c_type_class_t class);
bool type_is_floating(c_type_t* ct);
bool type_is_arithmetic_type(c_type_class_t class);
bool type_is_arithmetic(c_type_t* ct);
bool type_is_scalar_type(c_type_class_t class);
bool type_is_scalar(c_type_t* ct);
analysis_error_t* type(syntax_component_t* tlu);
void type_humanized_print(c_type_t* ct, int (*printer)(const char*, ...));
c_type_t* type_copy(c_type_t* ct);
#define type_is_qualified(ct) (ct ? ((ct)->qualifiers != 0) : false)

/* util.c */
char* strdup(char* str);
bool contains_substr(char* str, char* substr);
bool streq(char* s1, char* s2);
int int_array_index_max(int* array, size_t length);
void print_int_array(int* array, size_t length);

/* from somewhere */
bool in_debug(void);

#endif