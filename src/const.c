#include "ecc.h"

const char* KEYWORDS[KW_ELEMENTS] = {
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "register",
    "restrict",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
    "_Bool",
    "_Complex",
    "_Imaginary"
};

const char* SYNTAX_COMPONENT_NAMES[SC_NO_ELEMENTS] = {
    "SC_UNKNOWN", // unk
    "SC_ERROR", // err
    "SC_TRANSLATION_UNIT", // tlu
    "SC_FUNCTION_DEFINITION", // fdef
    "SC_DECLARATION", // decl
    "SC_INIT_DECLARATOR", // ideclr
    "SC_STORAGE_CLASS_SPECIFIER", // scs
    "SC_BASIC_TYPE_SPECIFIER", // bts
    "SC_STRUCT_UNION_SPECIFIER", // sus
    "SC_ENUM_SPECIFIER",
    "SC_TYPE_QUALIFIER",
    "SC_FUNCTION_SPECIFIER",
    "SC_ENUMERATOR",
    "SC_IDENTIFIER",
    "SC_DECLARATOR",
    "SC_POINTER",
    "SC_ARRAY_DECLARATOR",
    "SC_FUNCTION_DECLARATOR",
    "SC_PARAMETER_DECLARATION",
    "SC_ABSTRACT_DECLARATOR",
    "SC_ABSTRACT_ARRAY_DECLARATOR",
    "SC_ABSTRACT_FUNCTION_DECLARATOR",
    "SC_LABELED_STATEMENT",
    "SC_COMPOUND_STATEMENT",
    "SC_EXPRESSION_STATEMENT",
    "SC_IF_STATEMENT",
    "SC_SWITCH_STATEMENT",
    "SC_DO_STATEMENT",
    "SC_WHILE_STATEMENT",
    "SC_FOR_STATEMENT",
    "SC_GOTO_STATEMENT",
    "SC_CONTINUE_STATEMENT",
    "SC_BREAK_STATEMENT",
    "SC_RETURN_STATEMENT",
    "SC_INITIALIZER_LIST",
    "SC_DESIGNATION",
    "SC_EXPRESSION",
    "SC_ASSIGNMENT_EXPRESSION",
    "SC_LOGICAL_OR_EXPRESSION",
    "SC_LOGICAL_AND_EXPRESSION",
    "SC_BITWISE_OR_EXPRESSION",
    "SC_BITWISE_XOR_EXPRESSION",
    "SC_BITWISE_AND_EXPRESSION",
    "SC_EQUALITY_EXPRESSION",
    "SC_INEQUALITY_EXPRESSION",
    "SC_GREATER_EQUAL_EXPRESSION",
    "SC_LESS_EQUAL_EXPRESSION",
    "SC_GREATER_EXPRESSION",
    "SC_LESS_EXPRESSION",
    "SC_BITWISE_RIGHT_EXPRESSION",
    "SC_BITWISE_LEFT_EXPRESSION",
    "SC_SUBTRACTION_EXPRESSION",
    "SC_ADDITION_EXPRESSION",
    "SC_MODULAR_EXPRESSION",
    "SC_DIVISION_EXPRESSION",
    "SC_MULTIPLICATION_EXPRESSION",
    "SC_CONDITIONAL_EXPRESSION",
    "SC_CAST_EXPRESSION",
    "SC_PREFIX_INCREMENT_EXPRESSION",
    "SC_PREFIX_DECREMENT_EXPRESSION",
    "SC_REFERENCE_EXPRESSION",
    "SC_DEREFERENCE_EXPRESSION",
    "SC_PLUS_EXPRESSION",
    "SC_MINUS_EXPRESSION",
    "SC_COMPLEMENT_EXPRESSION",
    "SC_NOT_EXPRESSION",
    "SC_SIZEOF_EXPRESSION",
    "SC_SIZEOF_TYPE_EXPRESSION",
    "SC_COMPOUND_LITERAL",
    "SC_POSTFIX_INCREMENT_EXPRESSION",
    "SC_POSTFIX_DECREMENT_EXPRESSION",
    "SC_DEREFERENCE_MEMBER_EXPRESSION",
    "SC_MEMBER_EXPRESSION",
    "SC_FUNCTION_CALL_EXPRESSION",
    "SC_SUBSCRIPT_EXPRESSION",
    "SC_TYPE_NAME",
    "SC_TYPEDEF_NAME",
    "SC_ENUMERATION_CONSTANT",
    "SC_FLOATING_CONSTANT",
    "SC_INTEGER_CONSTANT",
    "SC_CHARACTER_CONSTANT",
    "SC_STRING_LITERAL",
    "SC_STRUCT_DECLARATION",
    "SC_STRUCT_DECLARATOR",
    "SC_CONSTANT_EXPRESSION",
    "SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION",
    "SC_DIVISION_ASSIGNMENT_EXPRESSION",
    "SC_MODULAR_ASSIGNMENT_EXPRESSION",
    "SC_ADDITION_ASSIGNMENT_EXPRESSION",
    "SC_SUBTRACTION_ASSIGNMENT_EXPRESSION",
    "SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION",
    "SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION",
    "SC_BITWISE_AND_ASSIGNMENT_EXPRESSION",
    "SC_BITWISE_OR_ASSIGNMENT_EXPRESSION",
    "SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION"
};

const char* IR_INSN_NAMES[II_NO_ELEMENTS] = {
    "II_UNKNOWN = 0",
    "II_FUNCTION_LABEL",
    "II_LOCAL_LABEL",
    "II_LOAD",
    "II_LOAD_ADDRESS",
    "II_RETURN",
    "II_STORE_ADDRESS",
    "II_SUBSCRIPT",
    "II_SUBSCRIPT_ADDRESS",
    "II_MEMBER",
    "II_MEMBER_ADDRESS",
    "II_ADDITION",
    "II_SUBTRACTION",
    "II_MULTIPLICATION",
    "II_DIVISION",
    "II_JUMP",
    "II_JUMP_IF_ZERO",
    "II_JUMP_NOT_ZERO",
    "II_EQUALITY",
    "II_MODULAR",
    "II_LESS",
    "II_GREATER",
    "II_LESS_EQUAL",
    "II_GREATER_EQUAL",
    "II_CAST",
    "II_DEREFERENCE",
    "II_NOT",
    "II_FUNCTION_CALL",
    "II_LEAVE",
    "II_ENDPROC",
    "II_DECLARE",
    "II_RETAIN",
    "II_RESTORE"
};

const char* C_TYPE_CLASS_NAMES[30] = {
    "_Bool",
    "char",
    "signed char",
    "short int",
    "int",
    "long int",
    "long long int",
    "unsigned char",
    "unsigned short int",
    "unsigned int",
    "unsigned long int",
    "unsigned long long int",
    "float",
    "double",
    "long double",
    "float _Complex",
    "double _Complex",
    "long double _Complex",
    "float _Imaginary",
    "double _Imaginary",
    "long double _Imaginary",
    "enum",
    "void",
    "array",
    "struct",
    "union",
    "function",
    "pointer",
    "label",
    "error"
};

const char* C_NAMESPACE_CLASS_NAMES[7] = {
    "NSC_LABEL",
    "NSC_STRUCT",
    "NSC_UNION",
    "NSC_ENUM",
    "NSC_STRUCT_MEMBER",
    "NSC_UNION_MEMBER",
    "NSC_ORDINARY"
};

const char* SPECIFIER_QUALIFIER_NAMES[9] = {
    "VOID",
    "ARITHMETIC_TYPE",
    "TYPEDEF",
    "STRUCT",
    "UNION",
    "ENUM",
    "STORAGE_CLASS",
    "FUNCTION",
    "QUALIFIER"
};

const char* ARITHMETIC_TYPE_NAMES[21] = {
    "CHAR",
    "SIGNED_CHAR",
    "UNSIGNED_CHAR",
    "SHORT_INT",
    "UNSIGNED_SHORT_INT",
    "INT",
    "UNSIGNED_INT",
    "LONG_INT",
    "UNSIGNED_LONG_INT",
    "LONG_LONG_INT",
    "UNSIGNED_LONG_LONG_INT",
    "FLOAT",
    "DOUBLE",
    "LONG_DOUBLE",
    "BOOL",
    "FLOAT_COMPLEX",
    "DOUBLE_COMPLEX",
    "LONG_DOUBLE_COMPLEX",
    "FLOAT_IMAGINARY",
    "DOUBLE_IMAGINARY",
    "LONG_DOUBLE_IMAGINARY"
};

unsigned ARITHMETIC_TYPE_SIZES[21] = {
    1,
    1,
    1,
    2,
    2,
    4,
    4,
    4,
    4,
    8,
    8,
    4,
    8,
    16,
    1,
    8,
    16,
    32,
    4,
    8,
    16
};

const char* BASIC_TYPE_SPECIFIER_NAMES[12] = {
    "void",
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",
    "signed",
    "unsigned",
    "bool",
    "_Complex",
    "_Imaginary"
};

const char* STORAGE_CLASS_NAMES[5] = {
    "typedef",
    "auto",
    "register",
    "static",
    "extern"
};

const char* TYPE_QUALIFIER_NAMES[3] = {
    "const",
    "restrict",
    "volatile"
};

const char* FUNCTION_SPECIFIER_NAMES[1] = {
    "inline"
};

const char* DECLARATOR_NAMES[5] = {
    "IDENTIFIER",
    "NEST",
    "POINTER",
    "ARRAY",
    "FUNCTION"
};

const char* INITIALIZER_NAMES[3] = {
    "EXPRESSION",
    "LIST",
    "DESIGNATION"
};

const char* STATEMENT_NAMES[6] = {
    "LABELED",
    "COMPOUND",
    "EXPRESSION",
    "SELECTION",
    "ITERATION",
    "JUMP"
};

const char* STATEMENT_LABELED_NAMES[3] = {
    "IDENTIFIER",
    "CASE",
    "DEFAULT"
};

const char* STATEMENT_SELECTION_NAMES[3] = {
    "IF",
    "IF_ELSE",
    "SWITCH"
};

const char* STATEMENT_ITERATION_NAMES[4] = {
    "WHILE",
    "DO_WHILE",
    "FOR_EXPRESSION",
    "FOR_DECLARATION"
};

const char* STATEMENT_JUMP_NAMES[4] = {
    "GOTO",
    "CONTINUE",
    "BREAK",
    "RETURN"
};

const char* EXPRESSION_NAMES[17] = {
    "ASSIGNMENT_LIST",
    "ASSIGNMENT",
    "CONDITIONAL",
    "LOGICAL_OR",
    "LOGICAL_AND",
    "OR",
    "XOR",
    "AND",
    "EQUALITY",
    "RELATIONAL",
    "SHIFT",
    "ADDITIVE",
    "MULTIPLICATIVE",
    "CAST",
    "UNARY",
    "POSTFIX",
    "PRIMARY"
};

const char* EXPRESSION_POSTFIX_NAMES[7] = {
    "COMPOUND_LITERAL",
    "SUBSCRIPT",
    "FUNCTION_CALL",
    "MEMBER",
    "PTR_MEMBER",
    "INCREMENT",
    "DECREMENT"
};

const char* EXPRESSION_PRIMARY_NAMES[5] = {
    "IDENTIFIER",
    "INTEGER_CONSTANT",
    "FLOATING_CONSTANT",
    "STRING_LITERAL",
    "NEST"
};

const char* ABSTRACT_DECLARATOR_NAMES[4] = {
    "POINTER",
    "ARRAY",
    "VLA",
    "FUNCTION"
};

const char* BOOL_NAMES[2] = {
    "false",
    "true"
};

const char* LEXER_TOKEN_NAMES[8] = {
    "KEYWORD",
    "IDENTIFIER",
    "OPERATOR",
    "SEPARATOR",
    "INTEGER_CONSTANT",
    "FLOATING_CONSTANT",
    "CHARACTER_CONSTANT",
    "STRING_CONSTANT"
};

const char* PP_TOKEN_NAMES[PPT_NO_ELEMENTS] = {
    "PPT_HEADER_NAME",
    "PPT_IDENTIFIER",
    "PPT_PP_NUMBER",
    "PPT_CHARACTER_CONSTANT",
    "PPT_STRING_LITERAL",
    "PPT_PUNCTUATOR",
    "PPT_OTHER",
    "PPT_COMMENT",
    "PPT_WHITESPACE"
};

const char* TOKEN_NAMES[T_NO_ELEMENTS] = {
    "T_KEYWORD",
    "T_IDENTIFIER",
    "T_INTEGER_CONSTANT",
    "T_FLOATING_CONSTANT",
    "T_CHARACTER_CONSTANT",
    "T_STRING_LITERAL",
    "T_PUNCTUATOR"
};

const char* PUNCTUATOR_STRING_REPRS[P_NO_ELEMENTS] = {
    "[",
    "]",
    "(",
    ")",
    "{",
    "}",
    ".",
    "->",
    "++",
    "--",
    "&",
    "*",
    "+",
    "-",
    "~",
    "!",
    "/",
    "%",
    "<<",
    ">>",
    "<",
    ">",
    "<=",
    ">=",
    "==",
    "!=",
    "^",
    "|",
    "&&",
    "||",
    "?",
    ":",
    ";",
    "...",
    "=",
    "*=",
    "/=",
    "%=",
    "+=",
    "-=",
    "<<=",
    ">>=",
    "&=",
    "^=",
    "|=",
    ",",
    "#",
    "##",
    "<:",
    ":>",
    "<%",
    "%>",
    "%:",
    "%:%:"
};

// use gcc libc essentially
const char* ANGLED_INCLUDE_SEARCH_DIRECTORIES[] = {
    "/usr/include",
    "/usr/lib/gcc/x86_64-linux-gnu/12/include",
    "/usr/local/include",
    "/usr/include/x86_64-linux-gnu"
};