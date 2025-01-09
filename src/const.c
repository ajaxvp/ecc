const char* KEYWORDS[37] = {
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

const char* SYNTAX_COMPONENT_NAMES[82] = {
    "SC_UNKNOWN",
    "SC_ERROR",
    "SC_TRANSLATION_UNIT",
    "SC_FUNCTION_DEFINITION",
    "SC_DECLARATION",
    "SC_INIT_DECLARATOR",
    "SC_STORAGE_CLASS_SPECIFIER",
    "SC_BASIC_TYPE_SPECIFIER",
    "SC_STRUCT_UNION_SPECIFIER",
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
    "SC_INITIALIZER_LIST_EXPRESSION",
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
    "SC_STRING_LITERAL"
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