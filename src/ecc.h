#ifndef ECC_H
#define ECC_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define debug in_debug()

#define assert(x) ((x) ? 0 : (errorf("assertion failed (%s:%d)\n", __FILE__, __LINE__), exit(1), 1))
#define assert_fail assert(0)

#define VECTOR_FOR(type, var, vec) type var = (type) vector_get((vec), 0); for (unsigned i = 0; i < (vec)->size; ++i, var = (type) vector_get((vec), i))
#define deep_free_syntax_vector(vec, var) if (vec) { VECTOR_FOR(syntax_component_t*, var, (vec)) free_syntax(var, tlu); vector_delete((vec)); }
#define SYMBOL_TABLE_FOR_ENTRIES_START(KEY_VAR, VALUE_VAR, CONTAINER) \
    for (unsigned i = 0; i < (CONTAINER)->capacity; ++i) \
    { \
        if (!(CONTAINER)->key[i]) continue; \
        char* KEY_VAR = (CONTAINER)->key[i]; \
        symbol_t* VALUE_VAR = (CONTAINER)->value[i]; \

#define SYMBOL_TABLE_FOR_ENTRIES_END }

#define MAP_FOR(ktype, vtype, map) ktype k = (ktype) (map)->key[0]; vtype v = (vtype) (map)->value[0]; for (unsigned i = 0; i < (map)->capacity; ++i, k = (ktype) (map)->key[i], v = (vtype) (map)->value[i])
#define MAP_IS_BAD_KEY (!k || (void*) k == (void*) (-1))

#define MAX_ERROR_LENGTH 512
#define MAX_STRINGIFIED_INTEGER_LENGTH 30
#define LINUX_MAX_PATH_LENGTH 4096

#define STACKFRAME_ALIGNMENT 16
#define STRUCT_UNION_ALIGNMENT 8
#define ALIGN(x, req) ((x) + ((req) - ((x) % (req))))

#define UNSIGNED_CHAR_WIDTH 1
#define UNSIGNED_SHORT_INT_WIDTH 2
#define UNSIGNED_INT_WIDTH 4
#define UNSIGNED_LONG_LONG_INT_WIDTH 8
#define POINTER_WIDTH 8

#define FLOAT_WIDTH 4
#define DOUBLE_WIDTH 8
#define LONG_DOUBLE_WIDTH 12

#define BOOL_MIN (unsigned char) 0x0
#define CHAR_MIN (signed char) 0x80
#define SIGNED_CHAR_MIN (signed char) 0x80
#define UNSIGNED_CHAR_MIN (unsigned char) 0x0
#define SHORT_INT_MIN (short) 0x8000
#define UNSIGNED_SHORT_INT_MIN (unsigned short) 0x0
#define INT_MIN (unsigned short) 0x80000000
#define UNSIGNED_INT_MIN 0x0U
#define LONG_INT_MIN (long) 0x8000000000000000L
#define UNSIGNED_LONG_INT_MIN 0x0UL
#define LONG_LONG_INT_MIN (long long) 0x8000000000000000LL
#define UNSIGNED_LONG_LONG_INT_MIN 0x0ULL
#define FLOAT_MIN 1.17549435082228750796873653722224568e-38F
#define DOUBLE_MIN (double) 2.22507385850720138309023271733240406e-308L
#define LONG_DOUBLE_MIN 3.36210314311209350626267781732175260e-4932L

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
#define FLOAT_MAX 3.40282346638528859811704183484516925e+38F
#define DOUBLE_MAX (double) 1.79769313486231570814527423731704357e+308L
#define LONG_DOUBLE_MAX 1.18973149535723176502126385303097021e+4932L

#define C_TYPE_SIZE_T CTC_UNSIGNED_LONG_INT
#define C_TYPE_PTRSIZE_T CTC_LONG_INT
#define C_TYPE_WCHAR_T CTC_INT

#define C_TYPE_SIZE_T_WIDTH 8
#define C_TYPE_PTRSIZE_T_WIDTH 8
#define C_TYPE_WCHAR_T_WIDTH 4

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

typedef int (*comparator_t)(void*, void*);
typedef void (*deleter_t)(void*);
typedef unsigned long (*hash_function_t)(void*);

typedef unsigned long long regid_t;

#define INVALID_VREGID ((regid_t) (0))
#define NO_PHYSICAL_REGISTERS 512

typedef enum air_locale
{
    LOC_UNKNOWN,
    LOC_X86_64
} air_locale_t;

typedef enum x86_register
{
    X86R_RAX = 1,
    X86R_RDI,
    X86R_RSI,
    X86R_RDX,
    X86R_RCX,
    X86R_R8,
    X86R_R9,
    X86R_R10,
    X86R_R11,
    X86R_RBX,
    X86R_RSP,
    X86R_RBP,
    X86R_R12,
    X86R_R13,
    X86R_R14,
    X86R_R15,
    X86R_XMM0,
    X86R_XMM1,
    X86R_XMM2,
    X86R_XMM3,
    X86R_XMM4,
    X86R_XMM5,
    X86R_XMM6,
    X86R_XMM7
} x86_register_t;

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
    PPT_PLACEHOLDER,
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

typedef enum intrinsic_function
{
    IF_VA_ARG,
    IF_VA_COPY,
    IF_VA_END,
    IF_VA_START,
    IF_LSYS_READ,
    IF_LSYS_WRITE,
    IF_LSYS_OPEN,
    IF_LSYS_CLOSE,
    IF_LSYS_MMAP,

    IF_NO_ELEMENTS
} intrinsic_function_t;

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
typedef struct constexpr constexpr_t;

typedef struct program_options
{
    bool hflag;
    bool iflag;
    bool ppflag;
    bool pflag;
    bool aflag;
    bool xflag;
    bool llflag;
    bool aaflag;
    bool rflag;
    bool ssflag;
    bool cflag;
    char* oflag;
} program_options_t;

typedef struct init_address
{
    symbol_t* sy;
    uint64_t data_location;
} init_address_t;

typedef struct x86_asm_init_address
{
    char* label;
    uint64_t data_location;
} x86_asm_init_address_t;

struct preprocessing_token
{
    preprocessor_token_type_t type;
    unsigned row, col;
    preprocessing_token_t* prev;
    preprocessing_token_t* next;
    bool can_start_directive;
    bool argument_content;
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
};

typedef struct preprocessing_table
{
    char** key;
    preprocessing_token_t** v_repl_list;
    vector_t** v_id_list;
    bool* v_variadic_list; 
    unsigned size;
    unsigned capacity;
} preprocessing_table_t;

typedef struct preprocessing_settings
{
    time_t* translation_time;
    char* filepath;
    char* error;
    preprocessing_table_t* table;
} preprocessing_settings_t;

struct token
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
};

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

struct vector_t
{
    void** data;
    unsigned capacity;
    unsigned size;
};

typedef struct c_type c_type_t;

struct c_type
{
    c_type_class_t class;
    unsigned char qualifiers;
    unsigned char function_specifiers;
    c_type_t* derived_from;
    union
    {
        struct
        {
            syntax_component_t* length_expression;
            uint64_t length;
            bool unspecified_size;
        } array;
        struct
        {
            char* name;
            vector_t* member_types; // <c_type_t*>
            vector_t* member_names; // <char*>
            vector_t* member_bitfields; // <syntax_component_t*>
            int64_t* member_bitfield_lengths;
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
};

typedef enum linkage
{
    LK_EXTERNAL,
    LK_INTERNAL,
    LK_NONE
} linkage_t;

typedef enum air_insn_operand_type {
    AOP_SYMBOL,
    AOP_REGISTER,
    AOP_INDIRECT_REGISTER,
    AOP_INDIRECT_SYMBOL,
    AOP_INTEGER_CONSTANT,
    AOP_FLOATING_CONSTANT,
    AOP_LABEL,
    AOP_TYPE,
} air_insn_operand_type_t;

typedef struct air_insn_operand {
    air_insn_operand_type_t type;
    c_type_t* ct;
    union
    {
        symbol_t* sy;
        regid_t reg;
        struct
        {
            regid_t id;
            regid_t roffset;
            long long offset;
            long long factor;
        } inreg;
        struct
        {
            symbol_t* sy;
            long long offset;
        } insy;
        unsigned long long ic;
        long double fc;
        struct
        {
            unsigned long long id;
            char disambiguator;
        } label;
        c_type_t* ct;
    } content;
} air_insn_operand_t;

// examples:
//  AIR_DECLARE:                     i32 x;
//  AIR_DECLARE_REGISTER:            i32 %eax;
//  AIR_LOAD:                        i32 _1 = x;
//  AIR_LOAD_ADDR:                   i32 _1 = &x + 16;
//  AIR_ADD:                         i32 _3 = _1 + _2;
//  AIR_RETURN:                      return _1;
//  AIR_FUNC_CALL:                   i32 _5 = _1(_4, _3, _2);
//  AIR_PHI:                         i32 _3 = phi(_1, _2);
//  AIR_NOP:                         <none>
//  AIR_ASSIGN:                      x = _1;
//  AIR_INDIRECT_ASSIGN:             *_1 = _2;
//  AIR_NEGATE:                      int _2 = -_1;
//  AIR_MULTIPLY:                    int _3 = _1 * _2;
//  AIR_DEREFERENCE:                 int _2 = *_1;
//  AIR_INDIRECT_ADD:                *_1 += _2;
//  AIR_INDIRECT_SUBTRACT:           *_1 -= _2;
//  AIR_INDIRECT_MULTIPLY:           *_1 *= _2;
//  AIR_INDIRECT_DIVIDE:             *_1 /= _2;
//  AIR_INDIRECT_MODULO:             *_1 %= _2;
//  AIR_INDIRECT_SHIFT_LEFT:         *_1 <<= _2;
//  AIR_INDIRECT_SHIFT_RIGHT:        *_1 >>= _2;
//  AIR_INDIRECT_SIGNED_SHIFT_RIGHT: *_1 >>>= _2;
//  AIR_INDIRECT_AND:                *_1 &= _2;
//  AIR_INDIRECT_XOR:                *_1 ^= _2;
//  AIR_INDIRECT_OR:                 *_1 |= _2;
//  AIR_POSATE:                      int _2 = +_1;
//  AIR_COMPLEMENT:                  int _2 = ~_1;
//  AIR_NOT:                         int _2 = !_1;
//  AIR_SEXT:                        int _2 = sext(_1);
//  AIR_ZEXT:                        int _2 = zext(_1);
//  AIR_S2D:                         double _2 = (double) _1;
//  AIR_D2S:                         float _2 = (float) _1;
//  AIR_DIVIDE:                      int _3 = _1 / _2;
//  AIR_MODULO:                      int _3 = _1 % _2;
//  AIR_SHIFT_LEFT:                  int _3 = _1 << _2;
//  AIR_SHIFT_RIGHT:                 int _3 = _1 >> _2;
//  AIR_SIGNED_SHIFT_RIGHT:          int _3 = _1 >>> _2;
//  AIR_LESS_EQUAL:                  int _3 = _1 <= _2;
//  AIR_LESS:                        int _3 = _1 < _2;
//  AIR_GREATER_EQUAL:               int _3 = _1 >= _2;
//  AIR_GREATER:                     int _3 = _1 > _2;
//  AIR_EQUAL:                       int _3 = _1 == _2;
//  AIR_INEQUAL:                     int _3 = _1 != _2;
//  AIR_AND:                         int _3 = _1 & _2;
//  AIR_XOR:                         int _3 = _1 ^ _2;
//  AIR_OR:                          int _3 = _1 | _2;
//  AIR_JZ:                          jz(.L1, _1);
//  AIR_JNZ:                         jnz(.L1, _1);
//  AIR_JMP:                         jmp(.L1);
//  AIR_LABEL:                       .L1:
//  AIR_DIRECT_SHIFT_LEFT:           _1 <<= _2;
//  AIR_DIRECT_OR:                   _1 |= _2;
//  AIR_DIRECT_XOR:                  _1 ^= _2;

typedef enum air_insn_type {
    AIR_DECLARE,
    AIR_DECLARE_REGISTER,
    AIR_LOAD,
    AIR_LOAD_ADDR,
    AIR_ADD,
    AIR_SUBTRACT,
    AIR_RETURN,
    AIR_FUNC_CALL,
    AIR_PHI,
    AIR_NOP,
    AIR_ASSIGN,
    AIR_NEGATE,
    AIR_MULTIPLY,
    AIR_DIRECT_ADD,
    AIR_DIRECT_SUBTRACT,
    AIR_DIRECT_MULTIPLY,
    AIR_DIRECT_DIVIDE,
    AIR_DIRECT_MODULO,
    AIR_DIRECT_SHIFT_LEFT,
    AIR_DIRECT_SHIFT_RIGHT,
    AIR_DIRECT_SIGNED_SHIFT_RIGHT,
    AIR_DIRECT_AND,
    AIR_DIRECT_XOR,
    AIR_DIRECT_OR,
    AIR_POSATE,
    AIR_COMPLEMENT,
    AIR_NOT,
    AIR_SEXT,
    AIR_ZEXT,
    AIR_S2D,
    AIR_D2S,
    AIR_S2SI,
    AIR_S2UI,
    AIR_D2SI,
    AIR_D2UI,
    AIR_SI2S,
    AIR_UI2S,
    AIR_SI2D,
    AIR_UI2D,
    AIR_DIVIDE,
    AIR_MODULO,
    AIR_SHIFT_LEFT,
    AIR_SHIFT_RIGHT,
    AIR_SIGNED_SHIFT_RIGHT,
    AIR_LESS_EQUAL,
    AIR_LESS,
    AIR_GREATER_EQUAL,
    AIR_GREATER,
    AIR_EQUAL,
    AIR_INEQUAL,
    AIR_AND,
    AIR_XOR,
    AIR_OR,
    AIR_JZ,
    AIR_JNZ,
    AIR_JMP,
    AIR_LABEL,
    AIR_PUSH,
    AIR_VA_ARG,
    AIR_VA_START,
    AIR_VA_END,
    AIR_SEQUENCE_POINT,
    AIR_MEMSET,
    AIR_BLIP,
    AIR_LSYSCALL
} air_insn_type_t;

typedef struct air_insn air_insn_t;

struct air_insn {
    air_insn_type_t type;
    c_type_t* ct;
    air_insn_t* prev;
    air_insn_t* next;
    size_t noops;
    air_insn_operand_t** ops;
    struct {
        // function calls that return structs have C type "pointer to struct." this disambiguates struct returns from ptr to struct returns. 
        bool fcall_sret;
    } metadata;
};

typedef struct air_data {
    bool readonly;
    symbol_t* sy;
    unsigned char* data;
    vector_t* addresses;
} air_data_t;

typedef struct air_routine {
    symbol_t* sy;
    air_insn_t* insns;
    symbol_t* retptr;
    bool uses_varargs;
} air_routine_t;

typedef struct air {
    vector_t* rodata; // <air_data_t*>
    vector_t* data; // <air_data_t*>
    vector_t* routines; // <air_routine_t*>
    symbol_table_t* st;
    air_locale_t locale;

    regid_t next_available_temporary;
    unsigned long long next_available_lv;
    symbol_t* sse32_negater;
    symbol_t* sse64_negater;
} air_t;

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

typedef enum x86_insn_size
{
    X86SZ_NONE,
    X86SZ_BYTE,
    X86SZ_WORD,
    X86SZ_DWORD,
    X86SZ_QWORD
} x86_insn_size_t;

typedef struct x86_operand
{
    x86_operand_type_t type;
    x86_insn_size_t size;
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
            long long offset;
        } array;
        char* label;
        struct
        {
            char* label;
            long long offset;
        } label_ref;
        char* text;
        char* string;
        unsigned long long immediate;
    };
} x86_operand_t;

typedef enum x86_insn_type
{
    X86I_UNKNOWN,
    X86I_LABEL,
    X86I_LEA,
    X86I_CALL,
    X86I_PUSH,
    X86I_POP,
    X86I_LEAVE,
    X86I_RET,
    X86I_JMP,
    X86I_JE,
    X86I_JNE,
    X86I_JNB,
    X86I_JS,
    X86I_CMP,
    X86I_SETE,
    X86I_SETNE,
    X86I_SETLE,
    X86I_SETL,
    X86I_SETGE,
    X86I_SETG,
    X86I_SETA,
    X86I_SETNB,
    X86I_SETP,
    X86I_SETNP,
    X86I_AND,
    X86I_OR,
    X86I_NOT,
    X86I_NOP,
    X86I_NEG,
    
    X86I_MOV,
    X86I_MOVZX,
    X86I_MOVSX,
    X86I_MOVSS,
    X86I_MOVSD,

    X86I_ADD,
    X86I_ADDSS,
    X86I_ADDSD,

    X86I_SUB,
    X86I_SUBSS,
    X86I_SUBSD,

    X86I_MUL,
    X86I_IMUL,
    X86I_MULSS,
    X86I_MULSD,

    X86I_DIV,
    X86I_IDIV,
    X86I_DIVSS,
    X86I_DIVSD,

    X86I_XOR,
    X86I_XORPS,
    X86I_XORPD,

    X86I_SHL,
    X86I_SHR,
    X86I_SAR,

    X86I_SKIP,

    X86I_CVTSS2SD,
    X86I_CVTSD2SS,

    X86I_CVTTSS2SI,
    X86I_CVTTSD2SI,

    X86I_CVTSI2SS,
    X86I_CVTSI2SD,

    X86I_COMISS,
    X86I_COMISD,

    X86I_UCOMISS,
    X86I_UCOMISD,

    X86I_TEST,
    X86I_PTEST,

    X86I_STC,

    X86I_ROR,

    X86I_REP_STOSB,

    X86I_SYSCALL,

    X86I_NO_ELEMENTS
} x86_insn_type_t;

struct x86_insn
{
    x86_insn_type_t type;
    x86_insn_size_t size;
    x86_operand_t* op1;
    x86_operand_t* op2;
    x86_operand_t* op3;
    x86_insn_t* next;
};

typedef struct x86_asm_data
{
    size_t alignment;
    char* label;
    bool readonly;
    unsigned char* data;
    vector_t* addresses;
    size_t length;
} x86_asm_data_t;

#define USED_NONVOLATILES_RBX (uint16_t) 0x0001
#define USED_NONVOLATILES_R12 (uint16_t) 0x0002
#define USED_NONVOLATILES_R13 (uint16_t) 0x0004
#define USED_NONVOLATILES_R14 (uint16_t) 0x0008
#define USED_NONVOLATILES_R15 (uint16_t) 0x0010

typedef struct x86_asm_routine
{
    uint64_t id;
    bool global;
    char* label;
    long long stackalloc;
    bool uses_varargs;
    uint16_t used_nonvolatiles;
    x86_insn_t* insns;
} x86_asm_routine_t;

typedef struct x86_asm_file
{
    vector_t* rodata; // vector_t<x86_asm_data_t>
    vector_t* data; // vector_t<x86_asm_data_t>
    vector_t* routines; // vector_t<x86_asm_routine_t>
    symbol_table_t* st;
    air_t* air;

    size_t next_constant_local_label;
    uint64_t next_routine_id;
    symbol_t* sse32_zero_checker;
    symbol_t* sse64_zero_checker;
    symbol_t* sse32_i64_limit;
    symbol_t* sse64_i64_limit;
} x86_asm_file_t;

typedef struct opt1_options
{
    bool inline_fcalls;
    bool remove_fcall_passing_lifetimes;
    bool inline_integer_constant;
} opt1_options_t;

typedef struct opt4_options
{
    bool remove_same_reg_moves;
    bool xor_zero_moves;
} opt4_options_t;

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

#define FS_B_INLINE (1 << FS_INLINE)

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
    SC_INTRINSIC_CALL_EXPRESSION,
    SC_PRIMARY_EXPRESSION_ENUMERATION_CONSTANT,

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
struct syntax_component_t
{
    syntax_component_type_t type;
    unsigned row, col;
    struct syntax_component_t* parent;

    // additional information
    c_type_t* ctype;
    bool lost_lvalue;
    air_insn_t* code;

    // type-specific additional info for linear IR transformation

    // expression types
    regid_t expr_reg;
    regid_t result_register;

    // loop types
    uint64_t continue_label_no;

    // switch & loop types
    uint64_t break_label_no;

    // initializer types
    int64_t initializer_offset;
    c_type_t* initializer_ctype;

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
            int enumr_value;
        };

        // id (general identifier)
        // SC_IDENTIFIER
        // SC_TYPEDEF_NAME
        // SC_PRIMARY_EXPRESSION_IDENTIFIER
        // SC_DECLARATOR_IDENTIFIER
        // SC_PRIMARY_EXPRESSION_ENUMERATION_CONSTANT
        char* id;

        // SC_STRING_LITERAL - strl
        struct
        {
            char* strl_id;
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
        struct
        {
            long double floc;
            char* floc_id;
        };

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
            // additional info
            unsigned long long lstmt_uid;
            unsigned long long lstmt_value;
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

            // static analysis
            vector_t* swstmt_cases; // <syntax_component_t> (SC_LABELED_STATEMENT)
            syntax_component_t* swstmt_default; // SC_LABELED_STATEMENT
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
            bool inlist_has_semantics;
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

        // SC_INTRINSIC_CALL_EXPRESSION - icallexpr
        struct
        {
            char* icallexpr_name;
            vector_t* icallexpr_args; // <syntax_component> (SC_ASSIGNMENT_EXPRESSION | SC_TYPE_NAME)
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
};

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

typedef enum constexpr_type
{
    CE_INTEGER,
    CE_ARITHMETIC,
    CE_ADDRESS
} constexpr_type_t;

struct constexpr
{
    constexpr_type_t type;
    c_type_t* ct;
    char* error;
    uint32_t err_row;
    uint32_t err_col;
    union
    {
        uint8_t* data;
        struct
        {
            symbol_t* sy; // null if the constant is a null pointer
            bool negative_offset;
            int64_t offset;
        } addr;
    } content;
};

struct symbol_t
{
    syntax_component_t* declarer; // the declaring identifier for this symbol in the syntax tree
    c_type_t* type;
    c_namespace_t* ns;
    uint64_t disambiguator; // disambiguating number for symbols in a symbol table which can share the same identifier but not on the assembly level
    long long stack_offset;
    char* name; // explicit name, if needed
    storage_duration_t sd; // explicit storage duration, if needed
    uint8_t* data; // initializing content for this symbol, if needed
    vector_t* addresses; // symbol and location information about addresses in the initializing content, if needed
    struct symbol_t* next; // next symbol in list (if in a list, otherwise NULL)
};

struct symbol_table_t
{
    char** key;
    symbol_t** value;
    unsigned size;
    unsigned capacity;
    vector_t* unique_types; // <c_type_t*>
};

struct analysis_error
{
    unsigned row, col;
    char* message;
    bool warning;
    analysis_error_t* next;
};

typedef void (*traversal_function)(syntax_traverser_t* trav, syntax_component_t* syn);

struct syntax_traverser
{
    syntax_component_t* tlu;

    traversal_function default_before;
    traversal_function default_after;

    traversal_function before[SC_NO_ELEMENTS];
    traversal_function after[SC_NO_ELEMENTS];
};

typedef struct map_t
{
    void** key;
    void** value;
    size_t size;
    size_t capacity;
    int (*comparator)(void*, void*);
    unsigned long (*hash)(void*);
    int (*key_printer)(void* key, int (*printer)(const char* fmt, ...));
    int (*value_printer)(void* value, int (*printer)(const char* fmt, ...));
    void (*key_deleter)(void* key);
    void (*value_deleter)(void* value);
} map_t;

typedef struct graph
{
    map_t* lists; // map_t<void*, set_t<void*>*>*
    int (*comparator)(void*, void*);
    unsigned long (*hash)(void*);
    void (*vertex_deleter)(void*);
} graph_t;

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
buffer_t* buffer_append_wide(buffer_t* b, int c);
buffer_t* buffer_append_str(buffer_t* b, char* str);
buffer_t* buffer_pop(buffer_t* b);
void buffer_delete(buffer_t* b);
char* buffer_export(buffer_t* b);
int* buffer_export_wide(buffer_t* b);

/* vector.c */
vector_t* vector_init(void);
vector_t* vector_add(vector_t* v, void* el);
vector_t* vector_add_if_new(vector_t* v, void* el, int (*c)(void*, void*));
void* vector_get(vector_t* v, unsigned index);
int vector_contains(vector_t* v, void* el, int (*c)(void*, void*));
vector_t* vector_copy(vector_t* v);
vector_t* vector_deep_copy(vector_t* v, void* (*copy_member)(void*));
bool vector_equals(vector_t* v1, vector_t* v2, bool (*equals)(void*, void*));
void vector_delete(vector_t* v);
void vector_deep_delete(vector_t* v, void (*deleter)(void*));
void* vector_pop(vector_t* v);
void* vector_peek(vector_t* v);
void vector_concat(vector_t* v, vector_t* u);
void vector_merge(vector_t* v, vector_t* u, int (*c)(void*, void*));

/* map.c */
map_t* map_init(int (*comparator)(void*, void*), unsigned long (*hash)(void*));
void map_set_printers(map_t* m,
    int (*key_printer)(void* key, int (*printer)(const char* fmt, ...)),
    int (*value_printer)(void* value, int (*printer)(const char* fmt, ...)));
void map_set_deleters(map_t* m, void (*key_deleter)(void* key), void (*value_deleter)(void* value));
void* map_add(map_t* m, void* key, void* value);
void* map_remove(map_t* m, void* key);
bool map_contains_key(map_t* m, void* key);
void* map_get(map_t* m, void* key);
void* map_get_or_add(map_t* m, void* key, void* value);
void map_delete(map_t* m);
void map_print(map_t* m, int (*printer)(const char*, ...));

map_t* set_init(int (*comparator)(void*, void*), unsigned long (*hash)(void*));
void set_set_deleter(map_t* m, void (*deleter)(void*));
bool set_add(map_t* m, void* key);
bool set_remove(map_t* m, void* key);
bool set_contains(map_t* m, void* key);
void set_delete(map_t* m);
void set_print(map_t* m, int (*printer)(const char*, ...));
void* set_get(map_t* m, void *key);

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
extern const char* X86_INSN_NAMES[X86I_NO_ELEMENTS];
extern const char* X86_64_BYTE_REGISTERS[];
extern const char* X86_64_WORD_REGISTERS[];
extern const char* X86_64_DOUBLE_REGISTERS[];
extern const char* X86_64_QUAD_REGISTERS[];
extern const char* X86_64_SSE_REGISTERS[];
extern const char* INTRINSIC_FUNCTION_NAMES[];

/* lex.c */
preprocessing_token_t* lex(FILE* file, bool dump_error);
preprocessing_token_t* lex_raw(unsigned char* data, size_t length, bool dump_error, bool start_in_include);
void pp_token_delete(preprocessing_token_t* token);
void pp_token_delete_content(preprocessing_token_t* token);
void pp_token_delete_all(preprocessing_token_t* tokens);
void pp_token_print(preprocessing_token_t* token, int (*printer)(const char* fmt, ...));
void pp_token_print_all(preprocessing_token_t* tokens, int (*printer)(const char* fmt, ...));
preprocessing_token_t* pp_token_copy(preprocessing_token_t* token);
preprocessing_token_t* pp_token_copy_range(preprocessing_token_t* start, preprocessing_token_t* end);
int pp_token_normal_snprint(char* buffer, size_t maxlen, preprocessing_token_t* token, int (*snprinter)(char*, size_t, const char* fmt, ...));
void pp_token_normal_print_range(preprocessing_token_t* token, preprocessing_token_t* end, int (*printer)(const char* fmt, ...));
bool pp_token_equals(preprocessing_token_t* t1, preprocessing_token_t* t2);
char* pp_token_stringify_range(preprocessing_token_t* start, preprocessing_token_t* end);

/* preprocess.c */
bool preprocess(preprocessing_token_t** tokens, preprocessing_settings_t* settings);
void strlitconcat(preprocessing_token_t* tokens);

/* parse.c */
syntax_component_t* parse_if_directive_expression(token_t* tokens, char* error);
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

/* symbol.c */
symbol_t* symbol_init(syntax_component_t* declarer);
syntax_component_t* symbol_get_scope(symbol_t* sy);
bool scope_is_block(syntax_component_t* scope);
storage_duration_t symbol_get_storage_duration(symbol_t* sy);
linkage_t symbol_get_linkage(symbol_t* sy);
void symbol_print(symbol_t* sy, int (*printer)(const char*, ...));
void symbol_delete(symbol_t* sy);
char* symbol_get_name(symbol_t* sy);
char* symbol_get_disambiguated_name(symbol_t* sy);
symbol_table_t* symbol_table_init(void);
symbol_t* symbol_table_add(symbol_table_t* t, char* k, symbol_t* sy);
symbol_t* symbol_table_get_all(symbol_table_t* t, char* k);
symbol_t* symbol_table_get_syn_id(symbol_table_t* t, syntax_component_t* id);
symbol_t* symbol_table_lookup(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns);
symbol_t* symbol_table_count(symbol_table_t* t, syntax_component_t* id, c_namespace_t* ns, vector_t** symbols, bool* first);
symbol_t* symbol_table_remove(symbol_table_t* t, syntax_component_t* id);
void symbol_table_print(symbol_table_t* t, int (*printer)(const char*, ...));
void symbol_table_delete(symbol_table_t* t, bool free_contents);
symbol_t* symbol_table_get_by_classes(symbol_table_t* t, char* k, c_type_class_t ctc, c_namespace_class_t nsc);
init_address_t* init_address_copy(init_address_t* addr);

/* syntax.c */
bool syntax_is_declarator_type(syntax_component_type_t type);
bool syntax_is_expression_type(syntax_component_type_t type);
bool syntax_is_abstract_declarator_type(syntax_component_type_t type);
bool syntax_is_relational_expression_type(syntax_component_type_t type);
bool syntax_is_equality_expression_type(syntax_component_type_t type);
bool syntax_is_typedef_name(syntax_component_t* id);
bool syntax_is_lvalue(syntax_component_t* syn);
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
syntax_component_t* syntax_get_function_declarator(syntax_component_t* declr);
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
unsigned long long evaluate_enumeration_constant(syntax_component_t* enumr);
bool syntax_is_assignment_expression(syntax_component_type_t type);
bool syntax_is_identifier(syntax_component_type_t type);
bool syntax_is_in_lvalue_context(syntax_component_t* syn);
bool syntax_contains_subelement(syntax_component_t* syn, syntax_component_type_t type);
int64_t syntax_get_full_initialization_offset(syntax_component_t* initializer);

/* type.c */
c_type_t* make_basic_type(c_type_class_t class);
c_type_t* make_reference_type(c_type_t* ct);
c_type_t* integer_promotions(c_type_t* ct);
c_type_t* default_argument_promotions(c_type_t* ct);
void usual_arithmetic_conversions(c_type_t* t1, c_type_t* t2, c_type_t** conv_t1, c_type_t** conv_t2);
c_type_t* usual_arithmetic_conversions_result_type(c_type_t* t1, c_type_t* t2);
void type_get_struct_union_member_info(c_type_t* ct, char* name, long long* index, int64_t* offset);
bool type_has_flexible_array_member(c_type_t* ct);
long long type_alignment(c_type_t* ct);
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
bool type_is_sse_floating_type(c_type_class_t class);
bool type_is_sse_floating(c_type_t* ct);
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
bool type_is_sua_type(c_type_class_t class);
bool type_is_sua(c_type_t* ct);
bool type_is_character_type(c_type_class_t class);
bool type_is_character(c_type_t* ct);
bool type_is_wchar_compatible(c_type_t* ct);
bool type_is_vla(c_type_t* ct);
bool type_is_function_inline(c_type_t* ct);
int64_t type_get_array_length(c_type_t* ct);
analysis_error_t* type(syntax_component_t* tlu);
c_type_t* create_type_with_errors(analysis_error_t* errors, syntax_component_t* specifying, syntax_component_t* declr);
c_type_t* create_type(syntax_component_t* specifying, syntax_component_t* declr);
void type_humanized_print(c_type_t* ct, int (*printer)(const char*, ...));
c_type_t* type_copy(c_type_t* ct);
c_type_t* strip_qualifiers(c_type_t* ct);
c_namespace_t* make_basic_namespace(c_namespace_class_t class);
#define type_is_qualified(ct) (ct ? ((ct)->qualifiers != 0) : false)

/* util.c */
#define ends_with_ignore_case(str, substr) starts_ends_with_ignore_case(str, substr, true)
#define starts_with_ignore_case(str, substr) starts_ends_with_ignore_case(str, substr, false)
#define ends_with(str, substr) starts_ends_with(str, substr, true)
#define starts_with(str, substr) starts_ends_with(str, substr, false)

char* strdup(const char* str);
int* strdup_wide(const int* str);
int* strdup_widen(const char* str);
char* substrdup(const char* str, size_t start, size_t end);
bool contains_substr(char* str, char* substr);
bool contains_char(char* str, char c);
bool streq(char* s1, char* s2);
bool streq_ignore_case(char* s1, char* s2);
int int_array_index_max(int* array, size_t length);
void print_int_array(int* array, size_t length);
bool starts_ends_with_ignore_case(char* str, char* substr, bool ends);
bool starts_ends_with(char* str, char* substr, bool ends);
void repr_print(char* str, int (*printer)(const char* fmt, ...));
unsigned long hash(char* str);
char* get_directory_path(char* path);
char* get_file_name(char* path, bool m);
int contains(void** array, unsigned length, void* el, int (*c)(void*, void*));
int regid_comparator(regid_t r1, regid_t r2);
unsigned long regid_hash(regid_t x);
int regid_print(regid_t reg, int (*printer)(const char*, ...));

int quickbuffer_printf(const char* fmt, ...);
void quickbuffer_setup(size_t size);
void quickbuffer_release(void);
char* quickbuffer(void);

int hexadecimal_digit_value(int c);
unsigned get_universal_character_hex_value(char* unichar, size_t length);
unsigned get_universal_character_utf8_encoding(unsigned value);

char* temp_filepath_gen(char* ext);
char* replace_extension(char* filepath, char* ext);

/* tokenize.c */

void token_delete(token_t* token);
void token_delete_all(token_t* token);
void token_print(token_t* token, int (*printer)(const char* fmt, ...));
token_t* tokenize_sequence(preprocessing_token_t* pp_tokens, preprocessing_token_t* end, tokenizing_settings_t* settings);
token_t* tokenize(preprocessing_token_t* pp_tokens, tokenizing_settings_t* settings);

/* air.c */

air_t* airinize(syntax_component_t* tlu);
void air_delete(air_t* air);
void air_print(air_t* air, int (*printer)(const char* fmt, ...));
void air_insn_print(air_insn_t* insn, air_t* air, int (*printer)(const char* fmt, ...));
void air_insn_delete_all(air_insn_t* insns);
air_insn_t* air_insn_insert_after(air_insn_t* insn, air_insn_t* inserting);
air_insn_t* air_insn_insert_before(air_insn_t* insn, air_insn_t* inserting);
air_insn_t* air_insn_move_before(air_insn_t* insn, air_insn_t* inserting);
air_insn_t* air_insn_move_after(air_insn_t* insn, air_insn_t* inserting);
air_insn_t* air_insn_remove(air_insn_t* insn);
air_insn_operand_t* air_insn_operand_copy(air_insn_operand_t* op);
void air_insn_operand_delete(air_insn_operand_t* op);
air_insn_operand_t* air_insn_symbol_operand_init(symbol_t* sy);
air_insn_operand_t* air_insn_register_operand_init(regid_t reg);
air_insn_operand_t* air_insn_indirect_register_operand_init(regid_t reg, long long offset, regid_t roffset, long long factor);
air_insn_operand_t* air_insn_indirect_symbol_operand_init(symbol_t* sy, long long offset);
air_insn_operand_t* air_insn_integer_constant_operand_init(unsigned long long ic);
air_insn_operand_t* air_insn_floating_constant_operand_init(long double fc);
air_insn_operand_t* air_insn_label_operand_init(unsigned long long label, char disambiguator);
air_insn_t* air_insn_init(air_insn_type_t type, size_t noops);
bool air_insn_creates_temporary(air_insn_t* insn);
air_insn_t* air_insn_find_temporary_definition_above(regid_t tmp, air_insn_t* start);
air_insn_t* air_insn_find_temporary_definition_below(regid_t tmp, air_insn_t* start);
air_insn_t* air_insn_find_temporary_definition_from_insn(regid_t tmp, air_insn_t* start);
air_insn_t* air_insn_find_temporary_definition(regid_t tmp, air_routine_t* routine);
bool air_insn_assigns(air_insn_t* insn);
bool air_insn_uses(air_insn_t* insn, regid_t reg);
bool air_insn_produces_side_effect(air_insn_t* insn);

/* localize.c */
void localize(air_t* air, air_locale_t locale);

/* opt1.c */

opt1_options_t* opt1_profile_basic(void);
void opt1(air_t* air, opt1_options_t* options);

/* opt4.c */
opt4_options_t* opt4_profile_basic(void);
void opt4(x86_asm_file_t* file, opt4_options_t* options);

/* allocate.c */

void allocate(air_t* air);

/* x86asm.c */

x86_asm_file_t* x86_generate(air_t* air, symbol_table_t* st);
void x86_asm_file_write(x86_asm_file_t* file, FILE* out);
void x86_asm_file_delete(x86_asm_file_t* file);
bool x86_64_c_type_registers_compatible(c_type_t* t1, c_type_t* t2);
void x86_operand_delete(x86_operand_t* op);
bool x86_64_is_integer_register(regid_t reg);
bool x86_64_is_sse_register(regid_t reg);

/* constexpr.c */

void constexpr_delete(constexpr_t* ce);
void constexpr_print_value(constexpr_t* ce, int (*printer)(const char*, ...));
bool constexpr_evaluation_succeeded(constexpr_t* ce);
bool constexpr_can_evaluate_integer(syntax_component_t* expr);
bool constexpr_can_evaluate_arithmetic(syntax_component_t* expr);
bool constexpr_can_evaluate_address(syntax_component_t* expr);
constexpr_t* constexpr_evaluate_integer(syntax_component_t* expr);
constexpr_t* constexpr_evaluate_arithmetic(syntax_component_t* expr);
constexpr_t* constexpr_evaluate_address(syntax_component_t* expr);
constexpr_t* constexpr_evaluate(syntax_component_t* expr);
void constexpr_convert(constexpr_t* ce, c_type_t* to);
void constexpr_convert_class(constexpr_t* ce, c_type_class_t class);
int64_t constexpr_as_i64(constexpr_t* ce);
uint64_t constexpr_as_u64(constexpr_t* ce);
int32_t constexpr_as_i32(constexpr_t* ce);
bool constexpr_equals_zero(constexpr_t* ce);
bool constexpr_can_evaluate(syntax_component_t* expr);

/* ecc.c */
program_options_t* get_program_options(void);

/* graph.c */

void graph_delete(graph_t* graph);
graph_t* graph_init(int (*comparator)(void*, void*), unsigned long (*hash)(void*), void (*vertex_deleter)(void*));
bool graph_add_vertex(graph_t* graph, void* item);
bool graph_add_edge(graph_t* graph, void* from, void* to);
bool graph_remove_vertex(graph_t* graph, void* item);
bool graph_remove_edge(graph_t* graph, void* from, void* to);
bool graph_has_edge(graph_t* graph, void* from, void* to);

/* from somewhere */
bool in_debug(void);

#endif
