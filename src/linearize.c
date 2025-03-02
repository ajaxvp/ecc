#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "ecc.h"

#define SETUP_LINEARIZE \
    ir_insn_t* dummy = calloc(1, sizeof *dummy); \
    dummy->type = II_UNKNOWN; \
    ir_insn_t* code = dummy;

#define ADD_CODE_EXPR(expr) (code->next = (expr), code->next->prev = code, code = code->next)
#define ADD_CODE(t, ...) ADD_CODE_EXPR(t(__VA_ARGS__))
#define COPY_CODE(s) code = copy_code_impl(syn, s, code)

#define FINALIZE_LINEARIZE \
    syn->code = dummy->next; \
    if (syn->code) syn->code->prev = NULL; \
    insn_delete(dummy);

#define LINEARIZING_TRAVERSER ((linearizing_syntax_traverser_t*) trav)
#define SYMBOL_TABLE ((syntax_get_translation_unit(syn))->tlu_st)

#define MAKE_REGISTER ++(LINEARIZING_TRAVERSER->next_vregid)

#define ENSURE_REFERENCE if (must_reference(syn)) code->ref = true;

typedef struct linearizing_syntax_traverser
{
    syntax_traverser_t base;
    ir_insn_t* function_label;
    regid_t next_vregid;
    unsigned long long next_llabel;
} linearizing_syntax_traverser_t;

locator_t* locator_copy(locator_t* loc)
{
    if (!loc) return NULL;
    locator_t* n = calloc(1, sizeof *loc);
    n->type = loc->type;
    switch (loc->type)
    {
        case L_OFFSET:
            n->stack_offset = loc->stack_offset;
            break;
        case L_LABEL:
            n->label = strdup(loc->label);
            break;
        case L_ARRAY:
            n->array.base_reg = loc->array.base_reg;
            n->array.offset_reg = loc->array.offset_reg;
            n->array.scale = loc->array.scale;
            break;
    }
    return n;
}

void locator_delete(locator_t* loc)
{
    if (!loc) return;
    switch (loc->type)
    {
        case L_LABEL:
            free(loc->label);
            break;
        default:
            break;
    }
    free(loc);
}

void locator_print(locator_t* loc, int (*printer)(const char* fmt, ...))
{
    if (!loc)
    {
        printer("(null)");
        return;
    }
    switch (loc->type)
    {
        case L_OFFSET:
            printer("local[%lld]", loc->stack_offset);
            break;
        case L_LABEL:
            printer("%s", loc->label);
            break;
        case L_ARRAY:
            printer("array[_%lld, _%lld, %d]", loc->array.base_reg, loc->array.offset_reg, loc->array.scale);
            break;
    }
}

void insn_operand_delete(ir_insn_operand_t* op)
{
    if (!op) return;
    switch (op->type)
    {
        case IIOP_LABEL:
            free(op->label);
            break;
        case IIOP_DESIGNATION:
            designation_delete_all(op->designation.desig);
            break;
        case IIOP_TYPE:
            type_delete(op->ct);
            break;
        default:
            break;
    }
    free(op);
}

ir_insn_operand_t* insn_operand_copy(ir_insn_operand_t* op)
{
    if (!op) return NULL;
    ir_insn_operand_t* n = calloc(1, sizeof *n);
    n->type = op->type;
    n->result = op->result;
    switch (op->type)
    {
        case IIOP_PHYSICAL_REGISTER:
            n->preg = op->preg;
            break;
        case IIOP_VIRTUAL_REGISTER:
            n->vreg = op->vreg;
            break;
        case IIOP_IMMEDIATE:
            n->immediate = op->immediate;
            break;
        case IIOP_IDENTIFIER:
            n->id_symbol = op->id_symbol;
            break;
        case IIOP_LABEL:
            n->label = strdup(op->label);
            break;
        case IIOP_FLOAT:
            n->fl = op->fl;
            break;
        case IIOP_STRING_LITERAL:
            n->string_literal.length = op->string_literal.length;
            if (op->string_literal.normal)
                n->string_literal.normal = strdup(op->string_literal.normal);
            else
                n->string_literal.wide = strdup_wide(op->string_literal.wide);
            break;
        case IIOP_DESIGNATION:
            n->designation.base = op->designation.base;
            n->designation.desig = designation_copy(op->designation.desig);
            break;
        case IIOP_TYPE:
            n->ct = type_copy(op->ct);
            break;
    }
    return n;
}

bool insn_operand_equals(ir_insn_operand_t* op1, ir_insn_operand_t* op2)
{
    if (!op1 && !op2)
        return true;
    if (!op1 || !op2)
        return false;
    if (op1->type != op2->type)
        return false;
    switch (op1->type)
    {
        case IIOP_PHYSICAL_REGISTER:
            return op1->preg == op2->preg;
        case IIOP_VIRTUAL_REGISTER:
            return op1->vreg == op2->vreg;
        case IIOP_LABEL:
            return streq(op1->label, op2->label);
        case IIOP_IMMEDIATE:
            return op1->immediate == op2->immediate;
        case IIOP_IDENTIFIER:
            return op1->id_symbol == op2->id_symbol;
        case IIOP_FLOAT:
            return op1->fl == op2->fl;
        case IIOP_STRING_LITERAL:
            if (op1->string_literal.length != op2->string_literal.length)
                return false;
            if (op1->string_literal.normal && op2->string_literal.wide)
                return false;
            if (op1->string_literal.wide && op2->string_literal.normal)
                return false;
            if (op1->string_literal.normal && strcmp(op1->string_literal.normal, op2->string_literal.normal))
                return false;
            if (op1->string_literal.wide && wcscmp(op1->string_literal.wide, op2->string_literal.wide))
                return false;
            return true;
        case IIOP_DESIGNATION:
            if (op1->designation.base != op2->designation.base)
                return false;
            if (!designation_equals(op1->designation.desig, op2->designation.desig))
                return false;
            return true;
        case IIOP_TYPE:
            return type_is_compatible(op1->ct, op2->ct);
        default:
            return false;
    }
}

void insn_operand_print(ir_insn_operand_t* op, int (*printer)(const char* fmt, ...))
{
    if (!op) return;
    switch (op->type)
    {
        case IIOP_PHYSICAL_REGISTER:
            printer("_%llu", op->preg);
            break;
        case IIOP_VIRTUAL_REGISTER:
            printer("_%llu", op->vreg);
            break;
        case IIOP_IMMEDIATE:
            printer("%lld", op->immediate);
            break;
        case IIOP_IDENTIFIER:
            printer("%s", symbol_get_name(op->id_symbol));
            break;
        case IIOP_LABEL:
            printer("%s", op->label);
            break;
        case IIOP_FLOAT:
            printer("%lf", op->fl);
            break;
        case IIOP_STRING_LITERAL:
            if (op->string_literal.normal)
            {
                printer("\"");
                repr_print(op->string_literal.normal, printer);
                printer("\"");
            }
            else
                // TODO
                printer("(wide string literal)");
            break;
        case IIOP_DESIGNATION:
            printer("%s", symbol_get_name(op->designation.base));
            designation_print(op->designation.desig, printer);
            break;
        case IIOP_TYPE:
            type_humanized_print(op->ct, printer);
            break;
    }
}

ir_insn_t* insn_copy(ir_insn_t* insn)
{
    if (!insn) return NULL;
    ir_insn_t* n = calloc(1, sizeof *insn);
    n->type = insn->type;
    n->prev = insn->prev;
    n->next = insn->next;
    n->ctype = type_copy(insn->ctype);
    n->noops = insn->noops;
    n->function_label = insn->function_label;
    n->ops = calloc(insn->noops, sizeof(ir_insn_operand_t*));
    for (size_t i = 0; i < insn->noops; ++i)
        n->ops[i] = insn_operand_copy(insn->ops[i]);
    switch (insn->type)
    {
        case II_FUNCTION_LABEL:
            n->metadata.function_label.linkage = insn->metadata.function_label.linkage;
            n->metadata.function_label.ellipsis = insn->metadata.function_label.ellipsis;
            n->metadata.function_label.knr = insn->metadata.function_label.knr;
            n->metadata.function_label.noparams = insn->metadata.function_label.noparams;
            n->metadata.function_label.params = calloc(insn->metadata.function_label.noparams, sizeof(symbol_t*));
            memcpy(n->metadata.function_label.params, insn->metadata.function_label.params, sizeof(symbol_t*) * insn->metadata.function_label.noparams);
            break;
        default:
            break;
    }
    return n;
}

void insn_delete(ir_insn_t* insn)
{
    if (!insn) return;
    switch (insn->type)
    {
        case II_FUNCTION_LABEL:
            free(insn->metadata.function_label.params);
            break;
        default:
            break;
    }
    for (size_t i = 0; i < insn->noops; ++i)
        insn_operand_delete(insn->ops[i]);
    free(insn->ops);
    type_delete(insn->ctype);
    free(insn);
}

void insn_delete_all(ir_insn_t* insn)
{
    if (!insn) return;
    insn_delete_all(insn->next);
    insn_delete(insn);
}

// prev inserting location next
void insert_ir_insn_before(ir_insn_t* location, ir_insn_t* inserting)
{
    if (!location || !inserting) return;
    inserting->prev = location->prev;
    inserting->next = location;
    if (location->prev)
        location->prev->next = inserting;
    location->prev = inserting;
}

// prev location inserting next
void insert_ir_insn_after(ir_insn_t* location, ir_insn_t* inserting)
{
    if (!location || !inserting) return;
    inserting->next = location->next;
    inserting->prev = location;
    if (location->next)
        location->next->prev = inserting;
    location->next = inserting;
}

// prev insn next
void remove_ir_insn(ir_insn_t* insn)
{
    if (!insn) return;
    if (insn->prev)
        insn->prev->next = insn->next;
    if (insn->next)
        insn->next->prev = insn->prev;
    insn_delete(insn);
}

void insn_clike_print(ir_insn_t* insn, int (*printer)(const char* fmt, ...))
{
    if (!insn)
    {
        printer("(null)");
        return;
    }
    #define INDENT_START printer("    ")
    #define SPACE printer(" ")
    #define EQUALS printer(" = ")
    #define SEMICOLON_END printer(";")
    #define GENERIC_BINOP_CASE(t, operator) \
        case t: \
            INDENT_START; \
            type_humanized_print(insn->ctype, printer); \
            SPACE; \
            insn_operand_print(insn->ops[0], printer); \
            EQUALS; \
            insn_operand_print(insn->ops[1], printer); \
            printer(operator); \
            insn_operand_print(insn->ops[2], printer); \
            SEMICOLON_END; \
            break;
    #define GENERIC_UNOP_CASE(t, operator) \
        case t: \
            INDENT_START; \
            type_humanized_print(insn->ctype, printer); \
            SPACE; \
            insn_operand_print(insn->ops[0], printer); \
            EQUALS; \
            printer(operator); \
            insn_operand_print(insn->ops[1], printer); \
            SEMICOLON_END; \
            break;
    #define ADDRESS_OF_IF_TYPE(ty) if (insn->type == ty) printer("&");
    switch (insn->type)
    {
        case II_LOAD:
        case II_LOAD_ADDRESS:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer);
            EQUALS;
            ADDRESS_OF_IF_TYPE(II_LOAD_ADDRESS);
            insn_operand_print(insn->ops[1], printer);
            SEMICOLON_END;
            break;
        case II_STORE_ADDRESS:
            INDENT_START;
            if (insn->ops[0]->type == IIOP_VIRTUAL_REGISTER || insn->ops[0]->type == IIOP_PHYSICAL_REGISTER)
                printer("*");
            insn_operand_print(insn->ops[0], printer);
            EQUALS;
            insn_operand_print(insn->ops[1], printer);
            SEMICOLON_END;
            break;
        case II_LOCAL_LABEL:
            insn_operand_print(insn->ops[0], printer);
            printer(":");
            break;
        case II_FUNCTION_LABEL:
            insn_operand_print(insn->ops[0], printer);
            printer(":");
            break;
        case II_RETURN:
            INDENT_START;
            printer("return ");
            insn_operand_print(insn->ops[0], printer);
            SEMICOLON_END;
            break;
        GENERIC_BINOP_CASE(II_ADDITION, " + ")
        GENERIC_BINOP_CASE(II_SUBTRACTION, " - ")
        GENERIC_BINOP_CASE(II_MULTIPLICATION, " * ")
        GENERIC_BINOP_CASE(II_DIVISION, " / ")
        GENERIC_BINOP_CASE(II_EQUALITY, " == ")
        GENERIC_BINOP_CASE(II_MODULAR, " %% ")
        GENERIC_BINOP_CASE(II_LESS, " < ")
        GENERIC_BINOP_CASE(II_GREATER, " > ")
        GENERIC_BINOP_CASE(II_LESS_EQUAL, " <= ")
        GENERIC_BINOP_CASE(II_GREATER_EQUAL, " >= ")
        case II_SUBSCRIPT:
        case II_SUBSCRIPT_ADDRESS:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer);
            EQUALS;
            ADDRESS_OF_IF_TYPE(II_SUBSCRIPT_ADDRESS);
            insn_operand_print(insn->ops[1], printer);
            printer("[");
            insn_operand_print(insn->ops[2], printer);
            printer("]");
            SEMICOLON_END;
            break;
        case II_MEMBER:
        case II_MEMBER_ADDRESS:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer);
            EQUALS;
            ADDRESS_OF_IF_TYPE(II_MEMBER_ADDRESS);
            insn_operand_print(insn->ops[1], printer);
            printer("->");
            insn_operand_print(insn->ops[2], printer);
            SEMICOLON_END;
            break;
        case II_DEREFERENCE:
        case II_DEREFERENCE_ADDRESS:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer); 
            EQUALS;
            ADDRESS_OF_IF_TYPE(II_DEREFERENCE_ADDRESS);
            printer("*");
            insn_operand_print(insn->ops[1], printer);
            SEMICOLON_END;
            break;
        GENERIC_UNOP_CASE(II_NOT, "!")
        case II_JUMP_IF_ZERO:
            INDENT_START;
            printer("jz by ");
            insn_operand_print(insn->ops[0], printer);
            printer(" to ");
            insn_operand_print(insn->ops[1], printer);
            SEMICOLON_END;
            break;
        case II_JUMP_NOT_ZERO:
            INDENT_START;
            printer("jnz by ");
            insn_operand_print(insn->ops[0], printer);
            printer(" to ");
            insn_operand_print(insn->ops[1], printer);
            SEMICOLON_END;
            break;
        case II_JUMP:
            INDENT_START;
            printer("jmp ");
            insn_operand_print(insn->ops[0], printer);
            SEMICOLON_END;
            break;
        case II_FUNCTION_CALL:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer);
            EQUALS;
            insn_operand_print(insn->ops[1], printer);
            printer("(");
            for (size_t i = 2; i < insn->noops; ++i)
            {
                if (i != 2)
                    printer(", ");
                insn_operand_print(insn->ops[i], printer);
            }
            printer(")");
            SEMICOLON_END;
            break;
        case II_RETAIN:
            INDENT_START;
            printer("retain ");
            insn_operand_print(insn->ops[0], printer);
            SEMICOLON_END;
            break;
        case II_RESTORE:
            INDENT_START;
            printer("restore ");
            insn_operand_print(insn->ops[0], printer);
            SEMICOLON_END;
            break;
        case II_LEAVE:
            INDENT_START;
            printer("leave");
            SEMICOLON_END;
            break;
        case II_ENDPROC:
            INDENT_START;
            printer("endproc");
            SEMICOLON_END;
            break;
        case II_CAST:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer);
            EQUALS;
            printer("(");
            insn_operand_print(insn->ops[1], printer);
            printer(") ");
            insn_operand_print(insn->ops[2], printer);
            SEMICOLON_END;
            break;
        case II_DECLARE:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            SPACE;
            insn_operand_print(insn->ops[0], printer);
            SEMICOLON_END;
            break;
        default:
            break;
    }
    #undef INDENT_START
    #undef SEMICOLON_END
    #undef SPACE
    #undef WALRUS
    #undef REF_IF_NEEDED
    #undef GENERIC_BINOP_CASE
}

void insn_clike_print_all(ir_insn_t* insn, int (*printer)(const char* fmt, ...))
{
    for (; insn; insn = insn->next)
    {
        insn_clike_print(insn, printer);
        printer("\n");
    }
}

ir_insn_operand_t* make_label_insn_operand(char* label)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_LABEL;
    op->label = strdup(label);
    return op;
}

ir_insn_operand_t* make_vreg_insn_operand(regid_t vreg, bool result)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_VIRTUAL_REGISTER;
    op->vreg = vreg;
    op->result = result;
    return op;
}

ir_insn_operand_t* make_preg_insn_operand(regid_t preg, bool result)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_PHYSICAL_REGISTER;
    op->preg = preg;
    op->result = result;
    return op;
}

ir_insn_operand_t* make_immediate_insn_operand(unsigned long long immediate)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_IMMEDIATE;
    op->immediate = immediate;
    return op;
}

ir_insn_operand_t* make_type_insn_operand(c_type_t* ct)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_TYPE;
    op->ct = type_copy(ct);
    return op;
}

ir_insn_operand_t* make_string_literal_insn_operand(char* literal, unsigned long long length)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_STRING_LITERAL;
    op->string_literal.normal = strdup(literal);
    op->string_literal.length = length;
    return op;
}

ir_insn_operand_t* make_wide_string_literal_insn_operand(int* literal, unsigned long long length)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_STRING_LITERAL;
    op->string_literal.wide = strdup_wide(literal);
    op->string_literal.length = length;
    return op;
}

ir_insn_operand_t* make_identifier_insn_operand(symbol_t* sy)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_IDENTIFIER;
    op->id_symbol = sy;
    return op;
}

ir_insn_operand_t* make_designation_insn_operand(symbol_t* base, designation_t* desig)
{
    ir_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = IIOP_DESIGNATION;
    op->designation.base = base;
    op->designation.desig = desig;
    return op;
}

ir_insn_t* copy_code_impl(syntax_component_t* dest, syntax_component_t* src, ir_insn_t* start)
{
    if (!src || !dest || !start)
        return NULL;
    if (!src->code)
        return start;
    for (ir_insn_t* insn = src->code; insn; insn = insn->next)
        start->next = insn_copy(insn), start->next->prev = start, start = start->next;
    return start;
}

ir_insn_t* make_basic_insn(ir_insn_type_t type, size_t noops)
{
    ir_insn_t* insn = calloc(1, sizeof *insn);
    insn->type = type;
    insn->ctype = NULL;
    insn->noops = noops;
    insn->ops = calloc(noops, sizeof(ir_insn_operand_t*));
    return insn;
}

ir_insn_t* make_1op(ir_insn_type_t type, c_type_t* ctype, ir_insn_operand_t* op)
{
    ir_insn_t* insn = make_basic_insn(type, 1);
    insn->ctype = type_copy(ctype);
    insn->ops[0] = op;
    return insn;
}

ir_insn_t* make_2op(ir_insn_type_t type, c_type_t* ctype, ir_insn_operand_t* op1, ir_insn_operand_t* op2)
{
    ir_insn_t* insn = make_basic_insn(type, 2);
    insn->ctype = type_copy(ctype);
    insn->ops[0] = op1;
    insn->ops[1] = op2;
    return insn;
}

ir_insn_t* make_3op(ir_insn_type_t type, c_type_t* ctype, ir_insn_operand_t* op1, ir_insn_operand_t* op2, ir_insn_operand_t* op3)
{
    ir_insn_t* insn = make_basic_insn(type, 3);
    insn->ctype = type_copy(ctype);
    insn->ops[0] = op1;
    insn->ops[1] = op2;
    insn->ops[2] = op3;
    return insn;
}

ir_insn_t* make_function_call_insn(c_type_t* ctype, ir_insn_operand_t** ops, size_t noops)
{
    ir_insn_t* insn = calloc(1, sizeof *insn);
    insn->type = II_FUNCTION_CALL;
    insn->ctype = type_copy(ctype);
    insn->noops = noops;
    insn->ops = ops;
    return insn;
}

ir_insn_t* make_function_label_insn(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* fdeclr = syn->fdef_declarator;
    syntax_component_t* id = syntax_get_declarator_identifier(fdeclr);
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    ir_insn_t* insn = make_basic_insn(II_FUNCTION_LABEL, 1);
    insn->ctype = type_copy(sy->type);
    insn->ops[0] = make_identifier_insn_operand(sy);
    insn->metadata.function_label.ellipsis = fdeclr->fdeclr_ellipsis;
    insn->metadata.function_label.knr = fdeclr->fdeclr_parameter_declarations == NULL;
    insn->metadata.function_label.linkage = symbol_get_linkage(sy);
    if (insn->metadata.function_label.knr && syn->fdef_knr_declarations->size)
    {
        insn->metadata.function_label.noparams = syn->fdef_knr_declarations->size;
        insn->metadata.function_label.params = calloc(insn->metadata.function_label.noparams, sizeof(symbol_t*));
        size_t k = 0;
        VECTOR_FOR(syntax_component_t*, knrdecl, syn->fdef_knr_declarations)
        {
            VECTOR_FOR(syntax_component_t*, declr, knrdecl->decl_init_declarators)
            {
                syntax_component_t* knrid = syntax_get_declarator_identifier(declr);
                if (!knrid)
                {
                    ++k;
                    continue;
                }
                insn->metadata.function_label.params[k++] = symbol_table_get_syn_id(SYMBOL_TABLE, knrid);
            }
        }
    }
    if (!insn->metadata.function_label.knr)
    {
        insn->metadata.function_label.noparams = fdeclr->fdeclr_parameter_declarations->size;
        insn->metadata.function_label.params = calloc(insn->metadata.function_label.noparams, sizeof(symbol_t*));
        VECTOR_FOR(syntax_component_t*, pdecl, fdeclr->fdeclr_parameter_declarations)
        {
            syntax_component_t* paramid = syntax_get_declarator_identifier(pdecl->pdecl_declr);
            if (!paramid) continue;
            insn->metadata.function_label.params[i] = symbol_table_get_syn_id(SYMBOL_TABLE, paramid);
        }
    }
    return insn;
}

/*

in the following conditions, the syn x must acquire an address:
  x in &x
  x in x++
  x in x--
  x in x.m
  x in x = y
  x in x += y (et. al. compound assignment operators)

*/
bool expression_needs_address(syntax_component_t* syn)
{
    if (!syn) return false;
    if (!syn->parent) return false;
    syntax_component_type_t type = syn->parent->type;
    return type == SC_REFERENCE_EXPRESSION ||
        type == SC_PREFIX_INCREMENT_EXPRESSION ||
        type == SC_PREFIX_DECREMENT_EXPRESSION ||
        type == SC_POSTFIX_INCREMENT_EXPRESSION ||
        type == SC_POSTFIX_DECREMENT_EXPRESSION ||
        (type == SC_MEMBER_EXPRESSION && syn->parent->memexpr_expression == syn) ||
        (syntax_is_assignment_expression(type) && syn->parent->bexpr_lhs == syn) ||
        (type == SC_INIT_DECLARATOR && syn->parent->ideclr_declarator == syn);
}

char* make_local_label(syntax_traverser_t* trav)
{
    char label[MAX_STRINGIFIED_INTEGER_LENGTH + 3];
    snprintf(label, MAX_STRINGIFIED_INTEGER_LENGTH + 3, ".L%llu", ++(LINEARIZING_TRAVERSER->next_llabel));
    return strdup(label);
}

void linearize_translation_unit_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, edecl, syn->tlu_external_declarations)
        COPY_CODE(edecl);
    FINALIZE_LINEARIZE;
}

void linearize_function_definition_before(syntax_traverser_t* trav, syntax_component_t* syn)
{
    LINEARIZING_TRAVERSER->function_label = make_function_label_insn(trav, syn);
}

void linearize_function_definition_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    LINEARIZING_TRAVERSER->function_label = NULL;
    if (!syn->fdef_declarator) report_return;
    syntax_component_t* id = syntax_get_declarator_identifier(syn->fdef_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    ADD_CODE(make_1op, II_FUNCTION_LABEL, NULL, make_identifier_insn_operand(sy));
    COPY_CODE(syn->fdef_declarator);
    COPY_CODE(syn->fdef_body);
    if (!code || code->type != II_LEAVE)
        ADD_CODE(make_basic_insn, II_LEAVE, 0);
    ADD_CODE(make_basic_insn, II_ENDPROC, 0);
    FINALIZE_LINEARIZE;
}

void linearize_compound_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, stmt, syn->cstmt_block_items)
        COPY_CODE(stmt);
    FINALIZE_LINEARIZE;
}

void linearize_return_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* expr = syn->retstmt_expression;
    if (expr)
    {
        COPY_CODE(expr);
        regid_t reg = expr->result_register;
        ADD_CODE(make_1op, II_RETURN, expr->ctype, make_vreg_insn_operand(reg, false));
    }
    ADD_CODE(make_basic_insn, II_LEAVE, 0);
    FINALIZE_LINEARIZE;
}

void linearize_expression_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->estmt_expression);
    FINALIZE_LINEARIZE;
}

void linearize_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, expr, syn->expr_expressions)
        COPY_CODE(expr);
    if (syn->expr_expressions)
    {
        syntax_component_t* last = vector_get(syn->expr_expressions, syn->expr_expressions->size - 1);
        syn->result_register = last->result_register;
    }
    FINALIZE_LINEARIZE;
}

void linearize_integer_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_2op, II_LOAD, syn->ctype, make_vreg_insn_operand(syn->result_register, true), make_immediate_insn_operand(syn->intc));
    FINALIZE_LINEARIZE;
}

void linearize_string_literal_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = MAKE_REGISTER;
    if (syn->strl_reg)
        ADD_CODE(make_2op, II_LOAD, syn->ctype, make_vreg_insn_operand(syn->result_register, true), make_string_literal_insn_operand(syn->strl_reg, syn->strl_length->intc));
    else
        ADD_CODE(make_2op, II_LOAD, syn->ctype, make_vreg_insn_operand(syn->result_register, true), make_wide_string_literal_insn_operand(syn->strl_wide, syn->strl_length->intc));
    FINALIZE_LINEARIZE;
}

// identifiers will:
//  - do nothing if they are part of or are a declarator
//  - do nothing if they are the right hand side of a member or deref. member expression 
//  - load the value of the object they designate if it is not used in an lvalue context
//  - load the address of the object they designate if it is used in an lvalue context
void linearize_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (syn->parent &&
        (syn->parent->type == SC_MEMBER_EXPRESSION || syn->parent->type == SC_DEREFERENCE_MEMBER_EXPRESSION) &&
        syn->parent->memexpr_id == syn)
        return;
    SETUP_LINEARIZE;
    c_namespace_t* ns = syntax_get_namespace(syn);
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, syn, ns);
    namespace_delete(ns);
    if (!sy) report_return;
    if (sy->declarer != syn)
    {
        syn->result_register = MAKE_REGISTER;
        c_type_t* ptr = calloc(1, sizeof *ptr);
        ptr->class = CTC_POINTER;
        ptr->derived_from = type_copy(syn->ctype);
        ADD_CODE(make_2op, expression_needs_address(syn) ? II_LOAD_ADDRESS : II_LOAD,
            expression_needs_address(syn) ? ptr : syn->ctype,
            make_vreg_insn_operand(syn->result_register, true), make_identifier_insn_operand(sy));
        type_delete(ptr);
    }
    else
        ADD_CODE(make_1op, II_DECLARE, sy->type, make_identifier_insn_operand(sy));
    FINALIZE_LINEARIZE;
}

void linearize_array_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = syn->adeclr_direct->result_register;
    COPY_CODE(syn->adeclr_direct);
    // TODO: probably need to alloca for VLAs
    FINALIZE_LINEARIZE;
}

void linearize_function_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->fdeclr_direct);
    VECTOR_FOR(syntax_component_t*, pdecl, syn->fdeclr_parameter_declarations)
        COPY_CODE(pdecl);
    FINALIZE_LINEARIZE;
}

void linearize_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->declr_direct);
    FINALIZE_LINEARIZE;
}

void linearize_initializer_list_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* idel = syn;
    for (; idel && idel->type != SC_INIT_DECLARATOR && idel->type != SC_COMPOUND_LITERAL; idel = idel->parent);
    if (!idel) report_return;
    symbol_t* identifying = NULL;
    if (idel->type == SC_INIT_DECLARATOR)
        identifying = symbol_table_get_syn_id(SYMBOL_TABLE, syntax_get_declarator_identifier(idel));
    else if (idel->type == SC_COMPOUND_LITERAL)
        identifying = symbol_table_get_syn_id(SYMBOL_TABLE, idel);
    if (!identifying) report_return;
    VECTOR_FOR(syntax_component_t*, d, syn->inlist_designations)
    {
        syntax_component_t* init = vector_get(syn->inlist_initializers, i);
        COPY_CODE(init);
        // initializer lists don't get a store instruction
        if (init->type == SC_INITIALIZER_LIST)
            continue;
        designation_t* desig = get_full_designation(d);
        // TODO: cast
        ADD_CODE(make_2op, II_STORE_ADDRESS, NULL,
            make_designation_insn_operand(identifying, desig),
            make_vreg_insn_operand(init->result_register, false));
    }
    FINALIZE_LINEARIZE;
}

void linearize_init_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* id = syntax_get_declarator_identifier(syn->ideclr_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;
    COPY_CODE(syn->ideclr_declarator);
    // no init declarator code gets loaded if the identifier it creates has static storage duration
    if (symbol_get_storage_duration(sy) == SD_STATIC)
        return;
    if (syn->ideclr_initializer)
    {
        syn->result_register = syn->ideclr_initializer->result_register;
        COPY_CODE(syn->ideclr_initializer);
        if (syn->ideclr_initializer->type != SC_INITIALIZER_LIST)
        {
            if (!type_is_compatible(sy->type, syn->ideclr_initializer->ctype))
            {
                ADD_CODE(make_3op, II_CAST, sy->type,
                    make_vreg_insn_operand(syn->result_register = MAKE_REGISTER, true),
                    make_type_insn_operand(sy->type),
                    make_vreg_insn_operand(syn->ideclr_initializer->result_register, false));
            }
            ADD_CODE(make_2op, II_STORE_ADDRESS, sy->type,
                make_identifier_insn_operand(sy),
                make_vreg_insn_operand(syn->result_register, false));
        }
    }
    FINALIZE_LINEARIZE;
}

void linearize_compound_literal_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    if (!sy) report_return;
    // no compound literal code gets loaded if the identifier it creates has static storage duration
    if (symbol_get_storage_duration(sy) == SD_STATIC)
        return;
    syn->result_register = MAKE_REGISTER;
    COPY_CODE(syn->cl_inlist);
    c_type_t* ptr = calloc(1, sizeof *ptr);
    ptr->class = CTC_POINTER;
    ptr->derived_from = type_copy(syn->ctype);
    ADD_CODE(make_2op, expression_needs_address(syn) ? II_LOAD_ADDRESS : II_LOAD,
        expression_needs_address(syn) ? ptr : syn->ctype,
        make_vreg_insn_operand(syn->result_register, true),
        make_identifier_insn_operand(sy));
    FINALIZE_LINEARIZE;
}

void linearize_assignment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_rhs);
    COPY_CODE(syn->bexpr_lhs);
    syn->result_register = syn->bexpr_rhs->result_register;
    if (!type_is_compatible(syn->bexpr_lhs->ctype, syn->bexpr_rhs->ctype))
    {
        ADD_CODE(make_3op, II_CAST, syn->ctype,
            make_vreg_insn_operand(syn->result_register = MAKE_REGISTER, true),
            make_type_insn_operand(syn->ctype),
            make_vreg_insn_operand(syn->bexpr_rhs->result_register, false));
    }
    ADD_CODE(make_2op, II_STORE_ADDRESS, syn->ctype,
        make_vreg_insn_operand(syn->bexpr_lhs->result_register, false),
        make_vreg_insn_operand(syn->result_register, true));
    FINALIZE_LINEARIZE;
}

void linearize_parameter_declaration_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->pdecl_declr);
    FINALIZE_LINEARIZE;
}

void linearize_declaration_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, ideclr, syn->decl_init_declarators)
        COPY_CODE(ideclr);
    FINALIZE_LINEARIZE;
}

static ir_insn_type_t binary_expression_type_map(syntax_component_type_t type)
{
    switch (type)
    {
        case SC_ADDITION_EXPRESSION: return II_ADDITION;
        case SC_SUBTRACTION_EXPRESSION: return II_SUBTRACTION;
        case SC_MULTIPLICATION_EXPRESSION: return II_MULTIPLICATION;
        case SC_DIVISION_EXPRESSION: return II_DIVISION;
        case SC_EQUALITY_EXPRESSION: return II_EQUALITY;
        case SC_LESS_EXPRESSION: return II_LESS;
        case SC_GREATER_EXPRESSION: return II_GREATER;
        case SC_LESS_EQUAL_EXPRESSION: return II_LESS_EQUAL;
        case SC_GREATER_EQUAL_EXPRESSION: return II_GREATER_EQUAL;
        case SC_MODULAR_EXPRESSION: return II_MODULAR;
        default:
            report_return_value(II_UNKNOWN);
    }
}

void linearize_binary_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_3op, binary_expression_type_map(syn->type), syn->ctype,
        make_vreg_insn_operand(syn->result_register, true), 
        make_vreg_insn_operand(syn->bexpr_lhs->result_register, false),
        make_vreg_insn_operand(syn->bexpr_rhs->result_register, false));
    FINALIZE_LINEARIZE;
}

void linearize_subscript_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* lhs = syn->subsexpr_expression;
    syntax_component_t* rhs = syn->subsexpr_index_expression;
    COPY_CODE(lhs);
    COPY_CODE(rhs);
    syn->result_register = MAKE_REGISTER;
    c_type_t* ptr = calloc(1, sizeof *ptr);
    ptr->class = CTC_POINTER;
    ptr->derived_from = type_copy(syn->ctype);
    ADD_CODE(make_3op, expression_needs_address(syn) ? II_SUBSCRIPT_ADDRESS : II_SUBSCRIPT,
        expression_needs_address(syn) ? ptr : syn->ctype,
        make_vreg_insn_operand(syn->result_register, true),
        make_vreg_insn_operand(lhs->result_register, false),
        make_vreg_insn_operand(rhs->result_register, false));
    type_delete(ptr);
    FINALIZE_LINEARIZE;
}

void linearize_member_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* lhs = syn->memexpr_expression;
    syntax_component_t* rhs = syn->memexpr_id;
    COPY_CODE(lhs);
    syn->result_register = MAKE_REGISTER;
    c_namespace_t* ns = syntax_get_namespace(rhs);
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, rhs, ns);
    namespace_delete(ns);
    if (!sy) report_return;
    c_type_t* ptr = calloc(1, sizeof *ptr);
    ptr->class = CTC_POINTER;
    ptr->derived_from = type_copy(syn->ctype);
    ADD_CODE(make_3op, expression_needs_address(syn) ? II_MEMBER_ADDRESS : II_MEMBER,
        expression_needs_address(syn) ? ptr : syn->ctype,
        make_vreg_insn_operand(syn->result_register, true),
        make_vreg_insn_operand(lhs->result_register, false),
        make_identifier_insn_operand(sy));
    type_delete(ptr);
    FINALIZE_LINEARIZE;
}

void linearize_if_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    // compare
    COPY_CODE(syn->ifstmt_condition);
    // jump if zero to else
    char* else_label = make_local_label(trav);
    char* end_label = make_local_label(trav);
    ADD_CODE(make_2op, II_JUMP_IF_ZERO, syn->ifstmt_condition->ctype,
        make_vreg_insn_operand(syn->ifstmt_condition->result_register, false),
        make_label_insn_operand(else_label));
    // if body
    COPY_CODE(syn->ifstmt_body);
    if (syn->ifstmt_else)
    {
        // unconditional jump to after
        ADD_CODE(make_1op, II_JUMP, NULL, make_label_insn_operand(end_label));
    }
    // label to start else body
    ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(else_label));
    if (syn->ifstmt_else)
    {
        // else body
        COPY_CODE(syn->ifstmt_else);
        // label for end of if statement
        ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(end_label));
    }
    free(else_label);
    free(end_label);
    FINALIZE_LINEARIZE;
}

/*

for (init; condition; post)
    body

becomes:

    init
    jmp condition label
body label:
    body
    post
condition label:
    condition
    jump not zero to body label

*/
void linearize_for_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->forstmt_init);
    char* body_label = make_local_label(trav);
    char* cond_label = make_local_label(trav);
    ADD_CODE(make_1op, II_JUMP, NULL, make_label_insn_operand(cond_label));
    ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(body_label));
    COPY_CODE(syn->forstmt_body);
    COPY_CODE(syn->forstmt_post);
    ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(cond_label));
    COPY_CODE(syn->forstmt_condition);
    ADD_CODE(make_2op, II_JUMP_NOT_ZERO, syn->forstmt_condition->ctype,
        make_vreg_insn_operand(syn->forstmt_condition->result_register, false),
        make_label_insn_operand(body_label));
    free(body_label);
    free(cond_label);
    FINALIZE_LINEARIZE;
}

/*

while (condition)
    body

becomes:

    jmp condition label
body label:
    body
condition label:
    condition
    jump not zero to body label

*/
void linearize_while_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    char* body_label = make_local_label(trav);
    char* cond_label = make_local_label(trav);
    ADD_CODE(make_1op, II_JUMP, NULL, make_label_insn_operand(cond_label));
    ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(body_label));
    COPY_CODE(syn->whstmt_body);
    ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(cond_label));
    COPY_CODE(syn->whstmt_condition);
    ADD_CODE(make_2op, II_JUMP_NOT_ZERO, syn->whstmt_condition->ctype,
        make_vreg_insn_operand(syn->whstmt_condition->result_register, false),
        make_label_insn_operand(body_label));
    free(body_label);
    free(cond_label);
    FINALIZE_LINEARIZE;
}

/*

do
    body
while (condition);

becomes:

body label:
    body
    condition
    jump not zero to body label

*/
void linearize_do_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    char* body_label = make_local_label(trav);
    ADD_CODE(make_1op, II_LOCAL_LABEL, NULL, make_label_insn_operand(body_label));
    COPY_CODE(syn->dostmt_body);
    COPY_CODE(syn->dostmt_condition);
    ADD_CODE(make_2op, II_JUMP_NOT_ZERO, syn->dostmt_condition->ctype,
        make_vreg_insn_operand(syn->dostmt_condition->result_register, false),
        make_label_insn_operand(body_label));
    free(body_label);
    FINALIZE_LINEARIZE;
}

void linearize_step_expression_after(syntax_traverser_t* trav, syntax_component_t* syn, bool prefix, bool inc)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->uexpr_operand);
    regid_t valuereg = MAKE_REGISTER;
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_2op, II_DEREFERENCE, syn->ctype,
        make_vreg_insn_operand(valuereg, true),
        make_vreg_insn_operand(syn->uexpr_operand->result_register, false));
    ADD_CODE(make_3op, inc ? II_ADDITION : II_SUBTRACTION, syn->ctype,
        make_vreg_insn_operand(syn->result_register, true),
        make_vreg_insn_operand(valuereg, false),
        make_immediate_insn_operand(1));
    ADD_CODE(make_2op, II_STORE_ADDRESS, syn->ctype,
        make_vreg_insn_operand(syn->uexpr_operand->result_register, false),
        make_vreg_insn_operand(syn->result_register, false));
    if (!prefix)
        syn->result_register = valuereg;
    FINALIZE_LINEARIZE;
}

void linearize_prefix_increment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    return linearize_step_expression_after(trav, syn, true, true);
}

void linearize_prefix_decrement_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    return linearize_step_expression_after(trav, syn, true, false);
}

void linearize_postfix_increment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    return linearize_step_expression_after(trav, syn, false, true);
}

void linearize_postfix_decrement_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    return linearize_step_expression_after(trav, syn, false, false);
}

void linearize_function_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    c_type_t* ftype = syn->fcallexpr_expression->ctype->derived_from;
    long long noargs = syn->fcallexpr_args->size;
    ir_insn_operand_t** args = calloc(noargs + 2, sizeof(ir_insn_operand_t*));
    args[0] = make_vreg_insn_operand(syn->result_register = MAKE_REGISTER, true);
    args[1] = make_vreg_insn_operand(syn->fcallexpr_expression->result_register, false);
    for (long long i = noargs - 1; i >= 0; --i)
    {
        syntax_component_t* arg = vector_get(syn->fcallexpr_args, i);
        COPY_CODE(arg);
        regid_t result = arg->result_register;
        if (ftype->function.param_types)
        {
            c_type_t* ptype = vector_get(ftype->function.param_types, i);
            bool vararg = ptype == NULL;

            if (vararg)
                ptype = default_argument_promotions(arg->ctype);
            
            if (!type_is_compatible(arg->ctype, ptype))
            {
                ADD_CODE(make_3op, II_CAST, ptype,
                    make_vreg_insn_operand(result = MAKE_REGISTER, true),
                    make_type_insn_operand(ptype),
                    make_vreg_insn_operand(arg->result_register, false));
            }

            if (vararg)
                type_delete(ptype);
        }
        args[i + 2] = make_vreg_insn_operand(result, false);
    }
    COPY_CODE(syn->fcallexpr_expression);
    ADD_CODE(make_function_call_insn, syn->ctype, args, noargs + 2);
    FINALIZE_LINEARIZE;
}

static ir_insn_type_t unary_expression_type_map(syntax_component_type_t type)
{
    switch (type)
    {
        case SC_NOT_EXPRESSION: return II_NOT;
        default:
            report_return_value(II_UNKNOWN);
    }
}

void linearize_unary_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->uexpr_operand);
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_2op, unary_expression_type_map(syn->type), syn->ctype,
        make_vreg_insn_operand(syn->result_register, true), 
        make_vreg_insn_operand(syn->uexpr_operand->result_register, false));
    FINALIZE_LINEARIZE;
}

void linearize_cast_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->caexpr_operand);
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_3op, II_CAST, syn->ctype,
        make_vreg_insn_operand(syn->result_register, true),
        make_type_insn_operand(syn->ctype),
        make_vreg_insn_operand(syn->caexpr_operand->result_register, false));
    FINALIZE_LINEARIZE;
}

void linearize_labeled_statement_after(syntax_component_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    if (syn->lstmt_id)
    {
        // TODO
    }
    COPY_CODE(syn->lstmt_stmt);
    FINALIZE_LINEARIZE;
}

void linearize_dereference_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->uexpr_operand);
    syn->result_register = MAKE_REGISTER;
    c_type_t* ptr = calloc(1, sizeof *ptr);
    ptr->class = CTC_POINTER;
    ptr->derived_from = type_copy(syn->ctype);
    ADD_CODE(make_2op, expression_needs_address(syn) ? II_DEREFERENCE_ADDRESS : II_DEREFERENCE,
        expression_needs_address(syn) ? ptr : syn->ctype,
        make_vreg_insn_operand(syn->result_register, true),
        make_vreg_insn_operand(syn->uexpr_operand->result_register, false));
    type_delete(ptr);
    FINALIZE_LINEARIZE;
}

void linearize_no_action_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    // does nothing. for syntax elements which need not be linearized
    // (assuredly, that's why the warning below is set up)
}

void linearize_default_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    warnf("no linearization procedure built for syntax elements of type %s\n", SYNTAX_COMPONENT_NAMES[syn->type]);
}

ir_insn_t* linearize(syntax_component_t* tlu)
{
    if (!tlu) return NULL;
    syntax_traverser_t* trav = traverse_init(tlu, sizeof(linearizing_syntax_traverser_t));

    trav->after[SC_TRANSLATION_UNIT] = linearize_translation_unit_after;
    trav->before[SC_FUNCTION_DEFINITION] = linearize_function_definition_before;
    trav->after[SC_FUNCTION_DEFINITION] = linearize_function_definition_after;
    trav->after[SC_COMPOUND_STATEMENT] = linearize_compound_statement_after;
    trav->after[SC_RETURN_STATEMENT] = linearize_return_statement_after;
    trav->after[SC_EXPRESSION_STATEMENT] = linearize_expression_statement_after;
    trav->after[SC_EXPRESSION] = linearize_expression_after;
    trav->after[SC_INTEGER_CONSTANT] = linearize_integer_constant_after;
    trav->after[SC_STRING_LITERAL] = linearize_string_literal_after;
    trav->after[SC_IDENTIFIER] = linearize_identifier_after;
    trav->after[SC_DECLARATOR] = linearize_declarator_after;
    trav->after[SC_INIT_DECLARATOR] = linearize_init_declarator_after;
    trav->after[SC_ARRAY_DECLARATOR] = linearize_array_declarator_after;
    trav->after[SC_FUNCTION_DECLARATOR] = linearize_function_declarator_after;
    trav->after[SC_DECLARATION] = linearize_declaration_after;
    trav->after[SC_ADDITION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_SUBTRACTION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_MULTIPLICATION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_DIVISION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_MODULAR_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_LESS_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_GREATER_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_LESS_EQUAL_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_GREATER_EQUAL_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_EQUALITY_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_SUBSCRIPT_EXPRESSION] = linearize_subscript_expression_after;
    trav->after[SC_IF_STATEMENT] = linearize_if_statement_after;
    trav->after[SC_FOR_STATEMENT] = linearize_for_statement_after;
    trav->after[SC_WHILE_STATEMENT] = linearize_while_statement_after;
    trav->after[SC_DO_STATEMENT] = linearize_do_statement_after;
    trav->after[SC_FUNCTION_CALL_EXPRESSION] = linearize_function_call_expression_after;
    trav->after[SC_MEMBER_EXPRESSION] = linearize_member_expression_after;
    trav->after[SC_INITIALIZER_LIST] = linearize_initializer_list_after;
    trav->after[SC_COMPOUND_LITERAL] = linearize_compound_literal_after;
    trav->after[SC_PREFIX_INCREMENT_EXPRESSION] = linearize_prefix_increment_expression_after;
    trav->after[SC_PREFIX_DECREMENT_EXPRESSION] = linearize_prefix_decrement_expression_after;
    trav->after[SC_POSTFIX_INCREMENT_EXPRESSION] = linearize_postfix_increment_expression_after;
    trav->after[SC_POSTFIX_DECREMENT_EXPRESSION] = linearize_postfix_decrement_expression_after;
    trav->after[SC_DEREFERENCE_EXPRESSION] = linearize_dereference_expression_after;
    trav->after[SC_NOT_EXPRESSION] = linearize_unary_expression_after;
    trav->after[SC_CAST_EXPRESSION] = linearize_cast_expression_after;
    trav->after[SC_PARAMETER_DECLARATION] = linearize_parameter_declaration_after;

    trav->after[SC_ABSTRACT_DECLARATOR] = linearize_no_action_after;
    trav->after[SC_POINTER] = linearize_no_action_after;
    trav->after[SC_BASIC_TYPE_SPECIFIER] = linearize_no_action_after;

    trav->default_after = linearize_default_after;

    traverse(trav);
    traverse_delete(trav);
    return tlu->code;
}
