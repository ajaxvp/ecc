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

const char* SYNTAX_COMPONENT_NAMES[18] = {
    "TRANSLATION_UNIT",
    "DECLARATION",
    "SPECIFIER_QUALIFIER",
    "DECLARATOR",
    "INITIALIZER",
    "STRUCT_UNION",
    "ENUM",
    "ENUMERATOR",
    "DESIGNATOR",
    "FUNCTION_DEFINITION",
    "STATEMENT",
    "EXPRESSION",
    "TYPE_NAME",
    "ABSTRACT_DECLARATOR",
    "",
    "PARAMETER_DECLARATION",
    "",
    "POINTER"
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

const char* STORAGE_CLASS_NAMES[5] = {
    "TYPEDEF",
    "AUTO",
    "REGISTER",
    "STATIC",
    "EXTERN"
};

const char* QUALIFIER_NAMES[3] = {
    "CONST",
    "VOLATILE",
    "RESTRICT"
};

const char* FUNCTION_SPECIFIER_NAMES[1] = {
    "INLINE"
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