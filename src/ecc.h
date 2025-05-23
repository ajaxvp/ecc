#ifndef ECC_H
#define ECC_H

#include <stdbool.h>
#include <stdio.h>

#define debug in_debug()

// these macros are used in circumstances where an error was produced by something that wasn't expected to error.
// for instance, if there was a mistake with an earlier stage like parsing, this might get thrown.
#define report_return { printf("bad (%s:%d)\n", __FILE__, __LINE__); return; }
#define report_continue { printf("bad (%s:%d)\n", __FILE__, __LINE__); continue; }
#define report_return_value(x) { printf("bad (%s:%d)\n", __FILE__, __LINE__); return (x); }

#define VECTOR_FOR(type, var, vec) type var = vector_get((vec), 0); for (unsigned i = 0; i < vec->size; ++i, var = vector_get((vec), i))
#define deep_free_syntax_vector(vec, var) if (vec) { VECTOR_FOR(syntax_component_t*, var, (vec)) free_syntax(var, tlu); vector_delete((vec)); }
#define SYMBOL_TABLE_FOR_ENTRIES_START(KEY_VAR, VALUE_VAR, CONTAINER) \
    for (unsigned i = 0; i < (CONTAINER)->capacity; ++i) \
    { \
        if (!(CONTAINER)->key[i]) continue; \
        char* KEY_VAR = (CONTAINER)->key[i]; \
        symbol_t* VALUE_VAR = (CONTAINER)->value[i]; \

#define SYMBOL_TABLE_FOR_ENTRIES_END }

#define MAX_ERROR_LENGTH 512
#define MAX_STRINGIFIED_INTEGER_LENGTH 30
#define LINUX_MAX_PATH_LENGTH 4096

#define STACKFRAME_ALIGNMENT 16
#define STRUCT_UNION_ALIGNMENT 8
#define ALIGN(x, req) ((x) + ((req) - ((x) % (req))))

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

typedef unsigned long long regid_t;

#define INVALID_VREGID ((regid_t) (0))

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
    T_PUNCTUATOR,
    T_NO_ELEMENTS
} token_type_t;

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

typedef struct symbol_table_t symbol_table_t;
typedef struct syntax_traverser syntax_traverser_t;
typedef struct analysis_error analysis_error_t;
typedef struct syntax_component_t syntax_component_t;
typedef struct preprocessing_token preprocessing_token_t;
typedef struct token token_t;
typedef struct ir_insn ir_insn_t;
typedef struct x86_insn x86_insn_t;
typedef struct symbol_t symbol_t;
typedef struct designation designation_t;
typedef struct vector_t vector_t;

typedef struct program_options
{
    bool iflag;
    bool ppflag;
    bool pflag;
    bool aflag;
} program_options_t;

typedef struct preprocessing_token
{
    preprocessor_token_type_t type;
    unsigned row, col;
    preprocessing_token_t* prev;
    preprocessing_token_t* next;
    bool can_start_directive;
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
            char* value;
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

        // PPT_OTHER
        unsigned char other;
    };
} preprocessing_token_t;

typedef struct preprocessing_table
{
    char** key;
    preprocessing_token_t** v_repl_list;
    vector_t** v_id_list;
    unsigned size;
    unsigned capacity;
} preprocessing_table_t;

typedef struct preprocessing_settings
{
    char* filepath;
    char* error;
    preprocessing_table_t* table;
} preprocessing_settings_t;

typedef struct token
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
            char* value_reg;
            int* value_wide;
        } string_literal;

        // T_PUNCTUATOR
        punctuator_type_t punctuator;
    };
} token_t;

typedef struct tokenizing_settings
{
    char* filepath;
    char* error;
} tokenizing_settings_t;

typedef struct allocator_options
{
    size_t no_volatile;
    size_t no_nonvolatile;
    regid_t (*procregmap)(long long);
} allocator_options_t;

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

typedef enum constexpr_type
{
    CE_ANY,
    CE_INTEGER,
    CE_ARITHMETIC,
    CE_ADDRESS
} constexpr_type_t;

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

typedef enum locator_type
{
    L_OFFSET = 0,
    L_LABEL,
    L_ARRAY
} locator_type_t;

typedef enum linkage
{
    LK_EXTERNAL,
    LK_INTERNAL,
    LK_NONE
} linkage_t;

typedef struct locator
{
    locator_type_t type;
    union
    {
        long long stack_offset;
        char* label;
        struct
        {
            regid_t base_reg;
            regid_t offset_reg;
            int scale;
        } array;
    };
} locator_t;

typedef enum ir_insn_operand_type
{
    IIOP_VIRTUAL_REGISTER,
    IIOP_PHYSICAL_REGISTER,
    IIOP_IDENTIFIER,
    IIOP_LABEL,
    IIOP_IMMEDIATE,
    IIOP_FLOAT,
    IIOP_STRING_LITERAL,
    IIOP_DESIGNATION,
    IIOP_TYPE
} ir_insn_operand_type_t;

typedef struct ir_insn_operand
{
    ir_insn_operand_type_t type;
    bool result;
    union
    {
        regid_t vreg;
        regid_t preg;
        symbol_t* id_symbol;
        unsigned long long immediate;
        long double fl;
        char* label;
        struct
        {
            char* normal;
            int* wide;
            unsigned long long length;
        } string_literal;
        struct
        {
            symbol_t* base;
            designation_t* desig;
        } designation;
        c_type_t* ct;
    };
} ir_insn_operand_t;

typedef enum ir_insn_type
{
    II_UNKNOWN = 0,
    II_FUNCTION_LABEL,
    II_LOCAL_LABEL,
    II_LOAD,
    II_LOAD_ADDRESS,
    II_RETURN,
    II_STORE_ADDRESS,
    II_SUBSCRIPT,
    II_SUBSCRIPT_ADDRESS,
    II_MEMBER,
    II_MEMBER_ADDRESS,
    II_ADDITION,
    II_SUBTRACTION,
    II_MULTIPLICATION,
    II_DIVISION,
    II_JUMP,
    II_JUMP_IF_ZERO,
    II_JUMP_NOT_ZERO,
    II_EQUALITY,
    II_MODULAR,
    II_LESS,
    II_GREATER,
    II_LESS_EQUAL,
    II_GREATER_EQUAL,
    II_CAST,
    II_DEREFERENCE,
    II_DEREFERENCE_ADDRESS,
    II_NOT,
    II_FUNCTION_CALL,
    II_LEAVE,
    II_ENDPROC,
    II_DECLARE,

    // LOW-LEVEL INSTRUCTIONS
    II_RETAIN,
    II_RESTORE,

    II_NO_ELEMENTS
} ir_insn_type_t;

typedef struct ir_insn ir_insn_t;

typedef struct ir_insn
{
    ir_insn_type_t type;
    ir_insn_t* function_label;
    c_type_t* ctype;
    size_t noops;
    ir_insn_operand_t** ops;
    ir_insn_t* prev;
    ir_insn_t* next;
    union
    {
        struct
        {
            size_t noparams;
            symbol_t** params;
            bool knr;
            bool ellipsis;
            linkage_t linkage;
        } function_label;
    } metadata;
} ir_insn_t;

typedef enum x86_operand_type
{
    X86OP_REGISTER,
    X86OP_PTR_REGISTER,
    X86OP_DEREF_REGISTER,
    X86OP_ARRAY,
    X86OP_LABEL,
    X86OP_LABEL_REF,
    X86OP_IMMEDIATE,
    X86OP_TEXT,
    X86OP_STRING
} x86_operand_type_t;

typedef struct x86_operand
{
    x86_operand_type_t type;
    union
    {
        regid_t reg;
        struct
        {
            regid_t reg_addr;
            long long offset;
        } deref_reg;
        struct
        {
            regid_t reg_base;
            regid_t reg_offset;
            long long scale;
        } array;
        char* label;
        char* text;
        char* string;
        unsigned long long immediate;
    };
} x86_operand_t;

typedef enum x86_insn_size
{
    X86SZ_BYTE,
    X86SZ_WORD,
    X86SZ_DWORD,
    X86SZ_QWORD
} x86_insn_size_t;

typedef enum x86_insn_type
{
    X86I_UNKNOWN,
    X86I_LABEL,
    X86I_MOV,
    X86I_LEA,
    X86I_CALL,
    X86I_PUSH,
    X86I_POP,
    X86I_ADD,
    X86I_SUB,
    X86I_LEAVE,
    X86I_RET,
    X86I_JMP,
    X86I_JE,
    X86I_JNE,
    X86I_CMP,
    X86I_SETE,
    X86I_SETL,
    X86I_XOR,
    X86I_NOT,

    /* directives */
    X86I_GLOBL,
    X86I_STRING,
    X86I_TEXT,
    X86I_DATA,
    X86I_SECTION,

    X86I_NO_ELEMENTS
} x86_insn_type_t;

typedef struct x86_insn
{
    x86_insn_type_t type;
    x86_insn_size_t size;
    x86_operand_t* op1;
    x86_operand_t* op2;
    x86_operand_t* op3;
    x86_insn_t* next;
} x86_insn_t;

typedef struct ir_opt_options
{
    bool inline_fcalls;
} ir_opt_options_t;

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

typedef enum storage_duration
{
    SD_UNKNOWN,
    SD_AUTOMATIC,
    SD_STATIC,
    SD_ALLOCATED
} storage_duration_t;

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
    SC_COMPOUND_LITERAL,
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
    SC_PRIMARY_EXPRESSION_IDENTIFIER,
    SC_DECLARATOR_IDENTIFIER,

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
    ir_insn_t* code;
    regid_t result_register;
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
        // SC_PRIMARY_EXPRESSION_IDENTIFIER
        // SC_DECLARATOR_IDENTIFIER
        char* id;

        // SC_STRING_LITERAL - strl
        struct
        {
            char* strl_reg;
            int* strl_wide;
            struct syntax_component_t* strl_length; // SC_INTEGER_CONSTANT
        };

        // SC_CHARACTER_CONSTANT - charc
        struct
        {
            int charc_value;
            bool charc_wide;
        };

        // SC_FLOATING_CONSTANT - floc
        long double floc;

        // SC_INTEGER_CONSTANT - intc
        unsigned long long intc;

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

        // SC_COMPOUND_LITERAL - cl
        struct
        {
            char* cl_id;
            struct syntax_component_t* cl_type_name; // SC_TYPE_NAME
            struct syntax_component_t* cl_inlist; // SC_INITIALIZER_LIST
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

typedef struct constexpr
{
    constexpr_type_t type;
    c_type_t* ct;
    union
    {
        unsigned long long ivalue;
        long double fvalue;
        syntax_component_t* addrexpr;
    };
} constexpr_t;

typedef struct designation
{
    constexpr_t* index;
    symbol_t* member;
    designation_t* next;
} designation_t;

typedef struct symbol_t
{
    syntax_component_t* declarer; // the declaring identifier for this symbol in the syntax tree
    c_type_t* type;
    c_namespace_t* ns;
    locator_t* loc;
    vector_t* designations;
    vector_t* initial_values;
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
extern const char* SYNTAX_COMPONENT_NAMES[SC_NO_ELEMENTS];
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
extern const char* C_NAMESPACE_CLASS_NAMES[7];
extern const char* PP_TOKEN_NAMES[PPT_NO_ELEMENTS];
extern const char* TOKEN_NAMES[T_NO_ELEMENTS];
extern const char* PUNCTUATOR_STRING_REPRS[P_NO_ELEMENTS];
extern const char* ANGLED_INCLUDE_SEARCH_DIRECTORIES[4];
extern const char* IR_INSN_NAMES[II_NO_ELEMENTS];
extern const char* X86_INSN_NAMES[X86I_NO_ELEMENTS];

/* lex.c */
preprocessing_token_t* lex(FILE* file, bool dump_error);
preprocessing_token_t* lex_raw(unsigned char* data, size_t length, bool dump_error);
void pp_token_delete(preprocessing_token_t* token);
void pp_token_delete_all(preprocessing_token_t* tokens);
void pp_token_print(preprocessing_token_t* token, int (*printer)(const char* fmt, ...));
preprocessing_token_t* pp_token_copy(preprocessing_token_t* token);
preprocessing_token_t* pp_token_copy_range(preprocessing_token_t* start, preprocessing_token_t* end);
void pp_token_normal_print_range(preprocessing_token_t* token, preprocessing_token_t* end, int (*printer)(const char* fmt, ...));

/* preprocess.c */
bool preprocess(preprocessing_token_t** tokens, preprocessing_settings_t* settings);
void charconvert(preprocessing_token_t* tokens);
void strlitconcat(preprocessing_token_t* tokens);

/* parse.c */
syntax_component_t* parse(token_t* toks);

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
bool scope_is_block(syntax_component_t* scope);
storage_duration_t symbol_get_storage_duration(symbol_t* sy);
linkage_t symbol_get_linkage(symbol_t* sy);
void symbol_print(symbol_t* sy, int (*printer)(const char*, ...));
void symbol_delete(symbol_t* sy);
char* symbol_get_name(symbol_t* sy);
symbol_t* symbol_init_initializer(symbol_t* sy);
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
bool syntax_is_declarator_type(syntax_component_type_t type);
bool syntax_is_expression_type(syntax_component_type_t type);
bool syntax_is_abstract_declarator_type(syntax_component_type_t type);
bool syntax_is_typedef_name(syntax_component_t* id);
bool syntax_is_lvalue(syntax_component_t* syn);
bool can_evaluate(syntax_component_t* expr, constexpr_type_t ce_type);
bool syntax_is_vla(syntax_component_t* declr);
bool syntax_is_known_size_array(syntax_component_t* declr);
bool syntax_is_modifiable_lvalue(syntax_component_t* syn);
bool syntax_is_tentative_definition(syntax_component_t* id);
bool syntax_has_initializer(syntax_component_t* id);
size_t syntax_no_specifiers(vector_t* declspecs, syntax_component_type_t type);
void namespace_delete(c_namespace_t* ns);
bool namespace_equals(c_namespace_t* ns1, c_namespace_t* ns2);
void namespace_print(c_namespace_t* ns, int (*printer)(const char* fmt, ...));
syntax_component_t* syntax_get_enclosing(syntax_component_t* syn, syntax_component_type_t enclosing);
vector_t* syntax_get_declspecs(syntax_component_t* id);
syntax_component_t* syntax_get_declarator_identifier(syntax_component_t* declr);
syntax_component_t* syntax_get_declarator_function_definition(syntax_component_t* declr);
syntax_component_t* syntax_get_declarator_declaration(syntax_component_t* declr);
syntax_component_t* syntax_get_full_declarator(syntax_component_t* declr);
syntax_component_t* syntax_get_translation_unit(syntax_component_t* syn);
symbol_table_t* syntax_get_symbol_table(syntax_component_t* syn);
syntax_component_t* syntax_get_function_definition(syntax_component_t* syn);
c_namespace_t* syntax_get_namespace(syntax_component_t* id);
c_namespace_t get_basic_namespace(c_namespace_class_t nsc);
void print_syntax(syntax_component_t* s, int (*printer)(const char* fmt, ...));
bool syntax_has_specifier(vector_t* specifiers, syntax_component_type_t sc_type, int type);
void free_syntax(syntax_component_t* syn, syntax_component_t* tlu);
unsigned long long process_integer_constant(char* con, c_type_class_t* class);
long double process_floating_constant(char* con, c_type_class_t* class);
unsigned long long evaluate_constant_expression(syntax_component_t* expr, c_type_class_t* class, constexpr_type_t cexpr_type);
unsigned long long evaluate_enumeration_constant(syntax_component_t* enumr);
bool syntax_is_assignment_expression(syntax_component_type_t type);
bool syntax_is_identifier(syntax_component_type_t type);

/* type.c */
c_type_t* make_basic_type(c_type_class_t class);
c_type_t* integer_promotions(c_type_t* ct);
c_type_t* default_argument_promotions(c_type_t* ct);
void usual_arithmetic_conversions(c_type_t* t1, c_type_t* t2, c_type_t** conv_t1, c_type_t** conv_t2);
c_type_t* usual_arithmetic_conversions_result_type(c_type_t* t1, c_type_t* t2);
long long type_size(c_type_t* ct);
void type_delete(c_type_t* ct);
void symbol_type_delete(c_type_t* ct);
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
c_type_t* create_type(syntax_component_t* specifying, syntax_component_t* declr);
void type_humanized_print(c_type_t* ct, int (*printer)(const char*, ...));
c_type_t* type_copy(c_type_t* ct);
c_type_t* strip_qualifiers(c_type_t* ct);
c_namespace_t* make_basic_namespace(c_namespace_class_t class);
#define type_is_qualified(ct) (ct ? ((ct)->qualifiers != 0) : false)

/* util.c */
#define ends_with_ignore_case(str, substr) starts_ends_with_ignore_case(str, substr, true)
#define starts_with_ignore_case(str, substr) starts_ends_with_ignore_case(str, substr, false)

char* strdup(const char* str);
int* strdup_wide(const int* str);
int* strdup_widen(const char* str);
bool contains_substr(char* str, char* substr);
bool contains_char(char* str, char c);
bool streq(char* s1, char* s2);
bool streq_ignore_case(char* s1, char* s2);
int int_array_index_max(int* array, size_t length);
void print_int_array(int* array, size_t length);
bool starts_ends_with_ignore_case(char* str, char* substr, bool ends);
void repr_print(char* str, int (*printer)(const char* fmt, ...));
unsigned long hash(char* str);
char* get_directory_path(char* path);
char* get_file_name(char* path, bool m);
int contains(void** array, unsigned length, void* el, int (*c)(void*, void*));

int quickbuffer_printf(const char* fmt, ...);
void quickbuffer_setup(size_t size);
void quickbuffer_release(void);
char* quickbuffer(void);

int hexadecimal_digit_value(int c);
unsigned get_universal_character_hex_value(char* unichar, size_t length);
unsigned get_universal_character_utf8_encoding(unsigned value);

char* temp_filepath_gen(char* ext);

/* tokenize.c */

void token_delete(token_t* token);
void token_delete_all(token_t* token);
void token_print(token_t* token, int (*printer)(const char* fmt, ...));
token_t* tokenize(preprocessing_token_t* pp_tokens, tokenizing_settings_t* settings);

/* linearize.c */

locator_t* locator_copy(locator_t* loc);
void locator_print(locator_t* loc, int (*printer)(const char* fmt, ...));
void locator_delete(locator_t* loc);
void insn_delete(ir_insn_t* insn);
void insn_delete_all(ir_insn_t* insn);
void insert_ir_insn_before(ir_insn_t* location, ir_insn_t* inserting);
void insert_ir_insn_after(ir_insn_t* location, ir_insn_t* inserting);
void remove_ir_insn(ir_insn_t* insn);
void insn_clike_print(ir_insn_t* insn, int (*printer)(const char* fmt, ...));
void insn_clike_print_all(ir_insn_t* insn, int (*printer)(const char* fmt, ...));
ir_insn_t* linearize(syntax_component_t* tlu);
ir_insn_operand_t* make_preg_insn_operand(regid_t preg, bool result);
ir_insn_operand_t* make_identifier_insn_operand(symbol_t* sy);
void insn_operand_delete(ir_insn_operand_t* op);
ir_insn_t* make_1op(ir_insn_type_t type, c_type_t* ctype, ir_insn_operand_t* op);
ir_insn_t* make_2op(ir_insn_type_t type, c_type_t* ctype, ir_insn_operand_t* op1, ir_insn_operand_t* op2);

bool insn_operand_equals(ir_insn_operand_t* op1, ir_insn_operand_t* op2);

/* iropt.c */

ir_opt_options_t* ir_opt_profile_basic(void);
void ir_optimize(ir_insn_t* insns, ir_opt_options_t* options);

/* allocate.c */

void allocate(ir_insn_t* insns, allocator_options_t* options);

/* x86gen.c */

regid_t x86procregmap(long long index);

void x86_write_all(x86_insn_t* insns, FILE* file);
void x86_write(x86_insn_t* insn, FILE* file);
void x86_insn_delete(x86_insn_t* insn);
void x86_insn_delete_all(x86_insn_t* insns);
x86_insn_t* x86_generate(ir_insn_t* insns, symbol_table_t* st);

/* constexpr.c */

constexpr_t* ce_make_integer(c_type_t* ct, unsigned long long value);
void constexpr_print(constexpr_t* ce, int (*printer)(const char* fmt, ...));
void constexpr_delete(constexpr_t* ce);
designation_t* syntax_to_designation(syntax_component_t* d);
designation_t* get_full_designation(syntax_component_t* d);
bool designation_equals(designation_t* d1, designation_t* d2);
designation_t* designation_copy(designation_t* desig);
void designation_print(designation_t* desig, int (*printer)(const char* fmt, ...));
void designation_delete(designation_t* desig);
void designation_delete_all(designation_t* desig);
designation_t* designation_concat(designation_t* d1, designation_t* d2);
constexpr_t* ce_evaluate(syntax_component_t* expr, constexpr_type_t type);

/* ecc.c */
program_options_t* get_program_options(void);

/* from somewhere */
bool in_debug(void);

#endif