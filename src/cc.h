#ifndef CC_H
#define CC_H

#include <stdbool.h>
#include <stdio.h>

#define terminate(fmt, ...) { errorf(fmt, ## __VA_ARGS__); exit(1); }
#define numargs(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define VECTOR_FOR(type, var, vec) type var = vector_get((vec), 0); for (unsigned i = 0; i < vec->size; ++i, var = vector_get((vec), i))

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
    BTS_IMAGINARY
} basic_type_specifier_t;

typedef enum type_qualifier
{
    TQ_CONST = 0,
    TQ_RESTRICT,
    TQ_VOLATILE
} type_qualifier_t;

typedef enum function_specifier
{
    FS_INLINE = 0
} function_specifier_t;

typedef enum struct_or_union
{
    SOU_STRUCT = 0,
    SOU_UNION
} struct_or_union_t;

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
    SC_STRING_LITERAL
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
// unless otherwise specified, vectors will not contain null pointers.
typedef struct syntax_component_t
{
    syntax_component_type_t type;
    unsigned row, col;
    union
    {
        // SC_TRANSLATION_UNIT - tlu
        struct
        {
            vector_t* tlu_external_declarations;
            vector_t* tlu_errors; // <syntax_component_t> (SC_ERROR)
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

        // SC_FLOATING_CONSTANT - floc
        long double floc;

        // SC_INTEGER_CONSTANT - intc
        long long intc;

        // SC_CHARACTER_CONSTANT - chrc
        int chrc;

        // SC_STRING_LITERAL - strl
        char* strl;

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
        // SC_DEREFERENCE_MEMBER_EXPRESSION
        // SC_MEMBER_EXPRESSION
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
            vector_t* tn_declarator; // <syntax_component_t> (SC_ABSTRACT_DECLARATOR)
        };

        // SC_ERROR - err
        struct
        {
            char* err_message;
            int err_depth;
        };
    };
} syntax_component_t;

typedef struct symbol_t
{
    struct symbol_t* shaded;
    syntax_component_t* scope;
    syntax_component_t* declaration;
    syntax_component_t* declarator;
    char* label;
    int offset;
} symbol_t;

typedef struct symbol_table_t
{
    char** key;
    symbol_t** value;
    unsigned size;
    unsigned capacity;
} symbol_table_t;

typedef struct emission_details_t
{
    FILE* out;
    syntax_component_t* unit;
    symbol_table_t* symbols;
} emission_details_t;

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
extern const char* SYNTAX_COMPONENT_NAMES[16];
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

/* lex.c */
lexer_token_t* lex(FILE* file);
void lex_delete(lexer_token_t* start);
void print_token(lexer_token_t* tok, int (*printer)(const char* fmt, ...));
bool is_assignment_operator_token(lexer_token_t* tok);
bool is_unary_operator_token(lexer_token_t* tok);

/* parse.c */
syntax_component_t* parse(lexer_token_t* toks);

/* emit.c */
bool emit(syntax_component_t* unit, FILE* out);

/* syntax.c */
syntax_component_t* find_declarator_identifier(syntax_component_t* declarator);
unsigned count_specifiers(syntax_component_t* declaration, unsigned type);
unsigned get_declaration_size(syntax_component_t* s);
void find_typedef(syntax_component_t** declaration_ref, syntax_component_t** declarator_ref, syntax_component_t* unit, char* identifier);
void print_syntax(syntax_component_t* s, int (*printer)(const char* fmt, ...));
bool syntax_has_specifier(vector_t* specifiers, syntax_component_type_t sc_type, int type);
void free_syntax(syntax_component_t* syn);

#endif