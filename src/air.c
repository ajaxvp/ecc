#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ecc.h"

typedef struct airinizing_syntax_traverser
{
    syntax_traverser_t base;
    air_t* air;
    air_routine_t* croutine;
    unsigned long long next_label;
} airinizing_syntax_traverser_t;

#define AIRINIZING_TRAVERSER ((airinizing_syntax_traverser_t*) trav)
#define SYMBOL_TABLE ((syntax_get_translation_unit(syn))->tlu_st)
#define NEXT_VIRTUAL_REGISTER (AIRINIZING_TRAVERSER->air->next_available_temporary++)
#define NEXT_LABEL (AIRINIZING_TRAVERSER->next_label++)

void air_data_delete(air_data_t* ad)
{
    if (!ad) return;
    vector_deep_delete(ad->addresses, free);
    free(ad->data);
    free(ad);
}

void air_insn_operand_delete(air_insn_operand_t* op)
{
    if (!op) return;
    type_delete(op->ct);
    free(op);
}

void air_insn_delete(air_insn_t* insn)
{
    if (!insn) return;
    for (size_t i = 0; i < insn->noops; ++i)
        air_insn_operand_delete(insn->ops[i]);
    free(insn->ops);
    type_delete(insn->ct);
    free(insn);
}

void air_insn_delete_all(air_insn_t* insns)
{
    if (!insns) return;
    air_insn_delete_all(insns->next);
    air_insn_delete(insns);
}

void air_routine_delete(air_routine_t* routine)
{
    if (!routine) return;
    air_insn_delete_all(routine->insns);
    free(routine);
}

void air_delete(air_t* air)
{
    if (!air) return;
    vector_deep_delete(air->rodata, (void (*)(void*)) air_data_delete);
    vector_deep_delete(air->data, (void (*)(void*)) air_data_delete);
    vector_deep_delete(air->routines, (void (*)(void*)) air_routine_delete);
    free(air);
}

void air_data_print(air_data_t* ad, air_t* air, int (*printer)(const char* fmt, ...))
{
    if (!ad)
    {
        printer("null\n");
        return;
    }
    if (ad->readonly)
        printer("readonly ");
    type_humanized_print(ad->sy->type, printer);
    printer(" %s {\n", symbol_get_name(ad->sy));
    long long size = type_size(ad->sy->type);
    if (size == -1)
        report_return;
    for (long long i = 0, j = 0; i < size;)
    {
        if (i % 16 == 0)
            printer("    ");
        if (ad->addresses && j < ad->addresses->size)
        {
            init_address_t* ia = vector_get(ad->addresses, j);
            if (i == ia->data_location)
            {
                ++j;
                if (i % 16 != 0)
                    printer("\n");
                if (ia->sy)
                    printer("&%s + %lld", symbol_get_name(ia->sy), *((int64_t*) (ad->data + ia->data_location)));
                else
                    printer("0x%llX", *((int64_t*) (ad->data + ia->data_location)));
                if (i % 16 != 0)
                    printer("\n");
                i += POINTER_WIDTH;
                continue;
            }
        }
        printer("%02X ", ad->data[i++]);
        if (i < size && i % 16 == 0)
            printer("\n");
    }
    printer("\n}\n");
}

void register_print(regid_t reg, c_type_t* ct, air_t* air, int (*printer)(const char* fmt, ...))
{
    if (reg == INVALID_VREGID)
        printer("(invalid register)");
    else if (reg <= NO_PHYSICAL_REGISTERS)
    {
        switch (air->locale)
        {
            case LOC_X86_64:
                if (type_is_real_floating(ct))
                {
                    printer("%%%s", X86_64_SSE_REGISTERS[reg - X86R_XMM0]);
                    break;
                }
                long long size = type_size(ct);
                const char** list = NULL;
                if (size == 1)
                    list = X86_64_BYTE_REGISTERS;
                else if (size == 2)
                    list = X86_64_WORD_REGISTERS;
                else if (size == 4)
                    list = X86_64_DOUBLE_REGISTERS;
                else
                    list = X86_64_QUAD_REGISTERS;
                printer("%%%s", list[reg - X86R_RAX]);
                break;
            default:
                printer("r%llu", reg);
                break;
        }
    }
    else
        printer("_%llu", reg - NO_PHYSICAL_REGISTERS);
}

void air_insn_operand_print(air_insn_operand_t* op, c_type_t* ict, air_t* air, int (*printer)(const char* fmt, ...))
{
    if (!op)
    {
        printer("null");
        return;
    }
    switch (op->type)
    {
        case AOP_REGISTER:
        case AOP_INDIRECT_REGISTER:
            if (op->type == AOP_INDIRECT_REGISTER)
            {
                printer("*");
                if (op->content.inreg.factor != 1 || op->content.inreg.offset != 0 || op->content.inreg.roffset != INVALID_VREGID)
                    printer("(");
            }
            register_print(op->content.reg, op->ct ? op->ct : ict, air, printer);
            if (op->type == AOP_INDIRECT_REGISTER)
            {
                if (op->content.inreg.roffset != INVALID_VREGID)
                {
                    printer(" + ");
                    register_print(op->content.inreg.roffset, op->ct ? op->ct : ict, air, printer);
                }
                if (op->content.inreg.offset != 0)
                    printer(" + %lld", op->content.inreg.offset);
                if (op->content.inreg.factor != 1)
                    printer(" * %lld", op->content.inreg.factor);
                if (op->content.inreg.factor != 1 || op->content.inreg.offset != 0 || op->content.inreg.roffset != INVALID_VREGID)
                    printer(")");
            }
            break;
        case AOP_INDIRECT_SYMBOL:
            printer("%s", symbol_get_name(op->content.insy.sy));
            if (op->content.insy.offset != 0)
                printer(" + %lld", op->content.insy.offset);
            break;
        case AOP_INTEGER_CONSTANT:
            printer("%llu", op->content.ic);
            break;
        case AOP_FLOATING_CONSTANT:
            printer("%Lf", op->content.fc);
            break;
        case AOP_SYMBOL:
            printer("%s", symbol_get_name(op->content.sy));
            break;
        case AOP_LABEL:
            printer(".L%c%llu", op->content.label.disambiguator, op->content.label.id);
            break;
        case AOP_TYPE:
            type_humanized_print(op->content.ct, printer);
            break;
    }
}

void air_insn_print(air_insn_t* insn, air_t* air, int (*printer)(const char* fmt, ...))
{
    if (!insn)
    {
        printer("null");
        return;
    }
    #define SPACE printer(" ");
    #define EQUALS printer(" = ");
    #define SEMICOLON printer(";");
    #define LPAREN printer("(");
    #define RPAREN printer(")");
    #define AMP printer("&");
    #define ASTERISK printer("*");
    #define COMMA printer(", ");
    #define OP(x) air_insn_operand_print(insn->ops[x], insn->ops[x]->ct ? insn->ops[x]->ct : insn->ct, air, printer);
    #define TYPE type_humanized_print(insn->ct, printer); printer(" ");
    #define OPERATOR(x) printer(" " #x " ");
    switch (insn->type)
    {
        case AIR_DECLARE:
            type_humanized_print(insn->ops[0]->content.sy->type, printer);
            printer(" ");
            OP(0) SEMICOLON
            break;
        case AIR_DECLARE_REGISTER:
            TYPE OP(0) SEMICOLON
            break;
        case AIR_FUNC_CALL:
            if (air->locale == LOC_X86_64 && insn->ops[0]->type == AOP_REGISTER && insn->ops[0]->content.reg == INVALID_VREGID)
            {
                printer("call"); LPAREN OP(1) RPAREN SEMICOLON
                break;
            }
            TYPE OP(0) EQUALS
            if (insn->metadata.fcall_sret)
                AMP
            OP(1) LPAREN
            for (size_t i = 2; i < insn->noops; ++i)
            {
                if (i != 2) COMMA
                OP(i)
            }
            RPAREN SEMICOLON
            break;
        case AIR_LOAD:
            TYPE OP(0) EQUALS OP(1) SEMICOLON
            break;
        case AIR_LOAD_ADDR:
            TYPE OP(0) EQUALS AMP OP(1) SEMICOLON
            break;
        case AIR_RETURN:
            printer("return");
            if (insn->noops >= 1)
            {
                SPACE OP(0)
            }
            SEMICOLON
            break;
        case AIR_PHI:
            TYPE OP(0) EQUALS printer("phi"); LPAREN
            for (size_t i = 1; i < insn->noops; ++i)
            {
                if (i != 1) COMMA
                OP(i)
            }
            RPAREN SEMICOLON
            break;
        case AIR_NOP:
            break;
        case AIR_ASSIGN:
            OP(0) EQUALS OP(1) SEMICOLON
            break;
        case AIR_NEGATE:
            TYPE OP(0) EQUALS printer("-"); OP(1) SEMICOLON
            break;
        case AIR_POSATE:
            TYPE OP(0) EQUALS printer("+"); OP(1) SEMICOLON
            break;
        case AIR_COMPLEMENT:
            TYPE OP(0) EQUALS printer("~"); OP(1) SEMICOLON
            break;
        case AIR_NOT:
            TYPE OP(0) EQUALS printer("!"); OP(1) SEMICOLON
            break;
        case AIR_MULTIPLY:
            TYPE OP(0) EQUALS OP(1) OPERATOR(*) OP(2) SEMICOLON
            break;
        case AIR_SEXT:
            TYPE OP(0) EQUALS printer("sext"); LPAREN OP(1) RPAREN SEMICOLON
            break;
        case AIR_ZEXT:
            TYPE OP(0) EQUALS printer("zext"); LPAREN OP(1) RPAREN SEMICOLON
            break;
        case AIR_S2D:
        case AIR_SI2D:
        case AIR_UI2D:
            TYPE OP(0) EQUALS printer("(double) "); OP(1) SEMICOLON
            break;
        case AIR_D2S:
        case AIR_SI2S:
        case AIR_UI2S:
            TYPE OP(0) EQUALS printer("(float) "); OP(1) SEMICOLON
            break;
        case AIR_S2SI:
        case AIR_D2SI:
            TYPE OP(0) EQUALS printer("(long long int) "); OP(1) SEMICOLON
            break;
        case AIR_S2UI:
        case AIR_D2UI:
            TYPE OP(0) EQUALS printer("(unsigned long long int) "); OP(1) SEMICOLON
            break;
        case AIR_ADD:
            TYPE OP(0) EQUALS OP(1) OPERATOR(+) OP(2) SEMICOLON
            break;
        case AIR_SUBTRACT:
            TYPE OP(0) EQUALS OP(1) OPERATOR(-) OP(2) SEMICOLON
            break;
        case AIR_DIVIDE:
        case AIR_MODULO:
            TYPE OP(0) EQUALS
            if (air->locale == LOC_X86_64 && insn->ops[1]->type == AOP_REGISTER && insn->ops[1]->content.reg == INVALID_VREGID)
            {
                register_print(X86R_RDX, insn->ct, air, printer);
                printf(":");
                register_print(X86R_RAX, insn->ct, air, printer);
            }
            else
            {
                OP(1)
            }
            if (insn->type == AIR_DIVIDE)
                OPERATOR(/)
            else
                OPERATOR(%%)
            OP(2) SEMICOLON
            break;
        case AIR_SHIFT_LEFT:
            TYPE OP(0) EQUALS OP(1) OPERATOR(<<) OP(2) SEMICOLON
            break;
        case AIR_SHIFT_RIGHT:
            TYPE OP(0) EQUALS OP(1) OPERATOR(>>>) OP(2) SEMICOLON
            break;
        case AIR_SIGNED_SHIFT_RIGHT:
            TYPE OP(0) EQUALS OP(1) OPERATOR(>>) OP(2) SEMICOLON
            break;
        case AIR_LESS_EQUAL:
            TYPE OP(0) EQUALS OP(1) OPERATOR(<=) OP(2) SEMICOLON
            break;
        case AIR_LESS:
            TYPE OP(0) EQUALS OP(1) OPERATOR(<) OP(2) SEMICOLON
            break;
        case AIR_GREATER_EQUAL:
            TYPE OP(0) EQUALS OP(1) OPERATOR(>=) OP(2) SEMICOLON
            break;
        case AIR_GREATER:
            TYPE OP(0) EQUALS OP(1) OPERATOR(>) OP(2) SEMICOLON
            break;
        case AIR_EQUAL:
            TYPE OP(0) EQUALS OP(1) OPERATOR(==) OP(2) SEMICOLON
            break;
        case AIR_INEQUAL:
            TYPE OP(0) EQUALS OP(1) OPERATOR(!=) OP(2) SEMICOLON
            break;
        case AIR_AND:
            TYPE OP(0) EQUALS OP(1) OPERATOR(&) OP(2) SEMICOLON
            break;
        case AIR_XOR:
            TYPE OP(0) EQUALS OP(1) OPERATOR(^) OP(2) SEMICOLON
            break;
        case AIR_OR:
            TYPE OP(0) EQUALS OP(1) OPERATOR(|) OP(2) SEMICOLON
            break;
        case AIR_JZ:
            printer("jz"); LPAREN OP(0) COMMA OP(1) RPAREN SEMICOLON
            break;
        case AIR_JNZ:
            printer("jnz"); LPAREN OP(0) COMMA OP(1) RPAREN SEMICOLON
            break;
        case AIR_JMP:
            printer("jmp"); LPAREN OP(0) RPAREN SEMICOLON
            break;
        case AIR_LABEL:
            OP(0) printer(":");
            break;
        case AIR_DIRECT_ADD:
            OP(0) OPERATOR(+=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_SUBTRACT:
            OP(0) OPERATOR(-=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_MULTIPLY:
            OP(0) OPERATOR(*=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_DIVIDE:
            OP(0) OPERATOR(/=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_MODULO:
            OP(0) OPERATOR(%%=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_SHIFT_LEFT:
            OP(0) OPERATOR(<<=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_SHIFT_RIGHT:
            OP(0) OPERATOR(>>>=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
            OP(0) OPERATOR(>>=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_AND:
            OP(0) OPERATOR(&=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_XOR:
            OP(0) OPERATOR(^=) OP(1) SEMICOLON
            break;
        case AIR_DIRECT_OR:
            OP(0) OPERATOR(|=) OP(1) SEMICOLON
            break;
        case AIR_PUSH:
            printer("push"); LPAREN OP(0) RPAREN SEMICOLON
            break;
        case AIR_VA_ARG:
            TYPE OP(0) EQUALS printer("va_arg"); LPAREN OP(1) RPAREN SEMICOLON
            break;
        case AIR_VA_START:
            TYPE OP(0) EQUALS printer("va_start"); LPAREN OP(1) RPAREN SEMICOLON
            break;
        case AIR_VA_END:
            TYPE OP(0) EQUALS printer("va_end"); LPAREN OP(1) RPAREN SEMICOLON
            break;
        case AIR_MEMSET:
            printer("memset"); LPAREN OP(0) COMMA OP(1) COMMA OP(2) RPAREN SEMICOLON
            break;
        case AIR_BLIP:
            printer("blip "); OP(0) SEMICOLON
            break;
        case AIR_SEQUENCE_POINT:
            printer("<sequence point>");
            break;
    }
    #undef SPACE
    #undef EQUALS
    #undef SEMICOLON
    #undef LPAREN
    #undef RPAREN
    #undef AMP
    #undef COMMA
    #undef OP
    #undef TYPE
    #undef OPERATOR
}

void air_routine_print(air_routine_t* routine, air_t* air, int (*printer)(const char* fmt, ...))
{
    if (!routine)
    {
        printer("null\n");
        return;
    }
    type_humanized_print(routine->sy->type->derived_from, printer);
    printer(" %s(", symbol_get_name(routine->sy));
    syntax_component_t* declr = syntax_get_full_declarator(routine->sy->declarer);
    VECTOR_FOR(c_type_t*, pt, routine->sy->type->function.param_types)
    {
        if (i) printer(", ");
        type_humanized_print(pt, printer);
        syntax_component_t* pdecl = vector_get(declr->fdeclr_parameter_declarations, i);
        if (!pdecl) continue;
        syntax_component_t* pid = syntax_get_declarator_identifier(pdecl->pdecl_declr);
        if (!pid) continue;
        printer(" %s", pid->id);
    }
    if (routine->sy->type->function.variadic)
        printer(", ...");
    printer(") {\n");
    for (air_insn_t* insn = routine->insns; insn; insn = insn->next)
    {
        if (insn->type == AIR_NOP) continue;
        if (insn->type != AIR_LABEL)
            printer("    ");
        air_insn_print(insn, air, printf);
        printer("\n");
    }
    printer("}\n");
}

void air_print(air_t* air, int (*printer)(const char* fmt, ...))
{
    if (!air)
    {
        printer("null\n");
        return;
    }
    VECTOR_FOR(air_data_t*, rd, air->rodata)
    {
        air_data_print(rd, air, printer);
        printer("\n");
    }
    VECTOR_FOR(air_data_t*, d, air->data)
    {
        air_data_print(d, air, printer);
        printer("\n");
    }
    VECTOR_FOR(air_routine_t*, routine, air->routines)
    {
        air_routine_print(routine, air, printer);
        printer("\n");
    }
}

air_insn_operand_t* air_insn_operand_init(air_insn_operand_type_t type)
{
    air_insn_operand_t* op = calloc(1, sizeof *op);
    op->type = type;
    return op;
}

air_insn_operand_t* air_insn_operand_copy(air_insn_operand_t* op)
{
    if (!op) return NULL;
    air_insn_operand_t* n = calloc(1, sizeof *n);
    n->type = op->type;
    n->ct = type_copy(op->ct);
    switch (n->type)
    {
        case AOP_SYMBOL:
            n->content.sy = op->content.sy;
            break;
        case AOP_REGISTER:
            n->content.reg = op->content.reg;
            break;
        case AOP_INDIRECT_REGISTER:
            n->content.inreg.id = op->content.inreg.id;
            n->content.inreg.roffset = op->content.inreg.roffset;
            n->content.inreg.offset = op->content.inreg.offset;
            n->content.inreg.factor = op->content.inreg.factor;
            break;
        case AOP_INTEGER_CONSTANT:
            n->content.ic = op->content.ic;
            break;
        case AOP_FLOATING_CONSTANT:
            n->content.fc = op->content.fc;
            break;
        case AOP_LABEL:
            n->content.label.disambiguator = op->content.label.disambiguator;
            n->content.label.id = op->content.label.id;
            break;
        case AOP_INDIRECT_SYMBOL:
            n->content.insy.sy = op->content.insy.sy;
            n->content.insy.offset = op->content.insy.offset;
            break;
        case AOP_TYPE:
            n->content.ct = type_copy(op->content.ct);
            break;
    }
    return n;
}

air_insn_operand_t* air_insn_symbol_operand_init(symbol_t* sy)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_SYMBOL);
    op->content.sy = sy;
    return op;
}

air_insn_operand_t* air_insn_register_operand_init(regid_t reg)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_REGISTER);
    op->content.reg = reg;
    return op;
}

air_insn_operand_t* air_insn_type_operand_init(c_type_t* ct)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_TYPE);
    op->content.ct = type_copy(ct);
    return op;
}

air_insn_operand_t* air_insn_indirect_register_operand_init(regid_t reg, long long offset, regid_t roffset, long long factor)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_INDIRECT_REGISTER);
    op->content.inreg.id = reg;
    op->content.inreg.offset = offset;
    op->content.inreg.roffset = roffset;
    op->content.inreg.factor = factor;
    return op;
}

air_insn_operand_t* air_insn_indirect_symbol_operand_init(symbol_t* sy, long long offset)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_INDIRECT_SYMBOL);
    op->content.insy.sy = sy;
    op->content.insy.offset = offset;
    return op;
}

air_insn_operand_t* air_insn_integer_constant_operand_init(unsigned long long ic)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_INTEGER_CONSTANT);
    op->content.ic = ic;
    return op;
}

air_insn_operand_t* air_insn_floating_constant_operand_init(long double fc)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_FLOATING_CONSTANT);
    op->content.fc = fc;
    return op;
}

air_insn_operand_t* air_insn_label_operand_init(unsigned long long label, char disambiguator)
{
    air_insn_operand_t* op = air_insn_operand_init(AOP_LABEL);
    op->content.label.id = label;
    op->content.label.disambiguator = disambiguator;
    return op;
}

air_insn_t* air_insn_init(air_insn_type_t type, size_t noops)
{
    air_insn_t* insn = calloc(1, sizeof *insn);
    insn->type = type;
    insn->noops = noops;
    insn->ops = calloc(noops, sizeof(air_insn_operand_t));
    return insn;
}

air_insn_t* air_insn_copy(air_insn_t* insn)
{
    if (!insn) return NULL;
    air_insn_t* n = calloc(1, sizeof *n);
    n->type = insn->type;
    n->ct = type_copy(insn->ct);
    n->prev = insn->prev;
    n->next = insn->next;
    n->noops = insn->noops;
    n->ops = calloc(n->noops, sizeof(air_insn_operand_t));
    n->metadata.fcall_sret = insn->metadata.fcall_sret;
    for (size_t i = 0; i < n->noops; ++i)
        n->ops[i] = air_insn_operand_copy(insn->ops[i]);
    return n;
}

bool air_insn_creates_temporary(air_insn_t* insn)
{
    if (!insn) return false;
    switch (insn->type)
    {
        case AIR_DECLARE:
        case AIR_RETURN:
        case AIR_NOP:
        case AIR_ASSIGN:
        case AIR_DIRECT_ADD:
        case AIR_DIRECT_SUBTRACT:
        case AIR_DIRECT_MULTIPLY:
        case AIR_DIRECT_DIVIDE:
        case AIR_DIRECT_MODULO:
        case AIR_DIRECT_SHIFT_LEFT:
        case AIR_DIRECT_SHIFT_RIGHT:
        case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
        case AIR_DIRECT_AND:
        case AIR_DIRECT_XOR:
        case AIR_DIRECT_OR:
        case AIR_JZ:
        case AIR_JNZ:
        case AIR_JMP:
        case AIR_LABEL:
        case AIR_PUSH:
        case AIR_SEQUENCE_POINT:
        case AIR_MEMSET:
            return false;
        case AIR_LOAD:
        case AIR_LOAD_ADDR:
        case AIR_DECLARE_REGISTER:
        case AIR_BLIP:
        case AIR_ADD:
        case AIR_SUBTRACT:
        case AIR_FUNC_CALL:
        case AIR_PHI:
        case AIR_NEGATE:
        case AIR_MULTIPLY:
        case AIR_POSATE:
        case AIR_COMPLEMENT:
        case AIR_NOT:
        case AIR_SEXT:
        case AIR_ZEXT:
        case AIR_S2D:
        case AIR_D2S:
        case AIR_S2SI:
        case AIR_S2UI:
        case AIR_D2SI:
        case AIR_D2UI:
        case AIR_SI2S:
        case AIR_UI2S:
        case AIR_SI2D:
        case AIR_UI2D:
        case AIR_DIVIDE:
        case AIR_MODULO:
        case AIR_SHIFT_LEFT:
        case AIR_SHIFT_RIGHT:
        case AIR_SIGNED_SHIFT_RIGHT:
        case AIR_LESS_EQUAL:
        case AIR_LESS:
        case AIR_GREATER_EQUAL:
        case AIR_GREATER:
        case AIR_EQUAL:
        case AIR_INEQUAL:
        case AIR_AND:
        case AIR_XOR:
        case AIR_OR:
        case AIR_VA_ARG:
        case AIR_VA_END:
        case AIR_VA_START:
            return true;
    }
    return false;
}

bool air_insn_assigns(air_insn_t* insn)
{
    if (!insn) return false;
    switch (insn->type)
    {
        case AIR_DECLARE:
        case AIR_DECLARE_REGISTER:
        case AIR_BLIP:
        case AIR_RETURN:
        case AIR_NOP:
        case AIR_JZ:
        case AIR_JNZ:
        case AIR_JMP:
        case AIR_LABEL:
        case AIR_PUSH:
        case AIR_SEQUENCE_POINT:
        case AIR_MEMSET:
            return false;
        case AIR_ASSIGN:
        case AIR_DIRECT_ADD:
        case AIR_DIRECT_SUBTRACT:
        case AIR_DIRECT_MULTIPLY:
        case AIR_DIRECT_DIVIDE:
        case AIR_DIRECT_MODULO:
        case AIR_DIRECT_SHIFT_LEFT:
        case AIR_DIRECT_SHIFT_RIGHT:
        case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
        case AIR_DIRECT_AND:
        case AIR_DIRECT_XOR:
        case AIR_DIRECT_OR:
        case AIR_LOAD:
        case AIR_LOAD_ADDR:
        case AIR_ADD:
        case AIR_SUBTRACT:
        case AIR_FUNC_CALL:
        case AIR_PHI:
        case AIR_NEGATE:
        case AIR_MULTIPLY:
        case AIR_POSATE:
        case AIR_COMPLEMENT:
        case AIR_NOT:
        case AIR_SEXT:
        case AIR_ZEXT:
        case AIR_S2D:
        case AIR_D2S:
        case AIR_S2SI:
        case AIR_S2UI:
        case AIR_D2SI:
        case AIR_D2UI:
        case AIR_SI2S:
        case AIR_UI2S:
        case AIR_SI2D:
        case AIR_UI2D:
        case AIR_DIVIDE:
        case AIR_MODULO:
        case AIR_SHIFT_LEFT:
        case AIR_SHIFT_RIGHT:
        case AIR_SIGNED_SHIFT_RIGHT:
        case AIR_LESS_EQUAL:
        case AIR_LESS:
        case AIR_GREATER_EQUAL:
        case AIR_GREATER:
        case AIR_EQUAL:
        case AIR_INEQUAL:
        case AIR_AND:
        case AIR_XOR:
        case AIR_OR:
        case AIR_VA_ARG:
        case AIR_VA_END:
        case AIR_VA_START:
            return true;
    }
    return false;
}

bool air_insn_produces_side_effect(air_insn_t* insn)
{
    if (!insn) return false;
    if (insn->type == AIR_FUNC_CALL)
        return true;
    // could modify an object
    switch (insn->type)
    {
        case AIR_VA_ARG:
        case AIR_VA_END:
        case AIR_VA_START:
            return true;
        case AIR_ASSIGN:
        case AIR_DIRECT_ADD:
        case AIR_DIRECT_SUBTRACT:
        case AIR_DIRECT_MULTIPLY:
        case AIR_DIRECT_DIVIDE:
        case AIR_DIRECT_MODULO:
        case AIR_DIRECT_SHIFT_LEFT:
        case AIR_DIRECT_SHIFT_RIGHT:
        case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
        case AIR_DIRECT_AND:
        case AIR_DIRECT_XOR:
        case AIR_DIRECT_OR:
            air_insn_operand_type_t optype = insn->ops[0]->type;
            if (optype == AOP_INDIRECT_REGISTER ||
                optype == AOP_INDIRECT_SYMBOL ||
                optype == AOP_SYMBOL)
                return true;
            break;
        default:
            break;
    }
    // could access a volatile object
    // TODO
    switch (insn->type)
    {
        case AIR_RETURN:
        case AIR_JZ:
        case AIR_JNZ:
        case AIR_PUSH:
        case AIR_ASSIGN:
        case AIR_DIRECT_ADD:
        case AIR_DIRECT_SUBTRACT:
        case AIR_DIRECT_MULTIPLY:
        case AIR_DIRECT_DIVIDE:
        case AIR_DIRECT_MODULO:
        case AIR_DIRECT_SHIFT_LEFT:
        case AIR_DIRECT_SHIFT_RIGHT:
        case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
        case AIR_DIRECT_AND:
        case AIR_DIRECT_XOR:
        case AIR_DIRECT_OR:
        case AIR_LOAD:
        case AIR_ADD:
        case AIR_SUBTRACT:
        case AIR_FUNC_CALL:
        case AIR_PHI:
        case AIR_NEGATE:
        case AIR_MULTIPLY:
        case AIR_POSATE:
        case AIR_COMPLEMENT:
        case AIR_NOT:
        case AIR_SEXT:
        case AIR_ZEXT:
        case AIR_S2D:
        case AIR_D2S:
        case AIR_S2SI:
        case AIR_S2UI:
        case AIR_D2SI:
        case AIR_D2UI:
        case AIR_SI2S:
        case AIR_UI2S:
        case AIR_SI2D:
        case AIR_UI2D:
        case AIR_DIVIDE:
        case AIR_MODULO:
        case AIR_SHIFT_LEFT:
        case AIR_SHIFT_RIGHT:
        case AIR_SIGNED_SHIFT_RIGHT:
        case AIR_LESS_EQUAL:
        case AIR_LESS:
        case AIR_GREATER_EQUAL:
        case AIR_GREATER:
        case AIR_EQUAL:
        case AIR_INEQUAL:
        case AIR_AND:
        case AIR_XOR:
        case AIR_OR:
        case AIR_VA_ARG:
        case AIR_VA_END:
        case AIR_VA_START:
        default:
            break;
    }
    return false;
}

bool air_insn_uses(air_insn_t* insn, regid_t reg)
{
    if (!insn) return false;
    for (size_t i = 0; i < insn->noops; ++i)
    {
        air_insn_operand_t* op = insn->ops[i];
        if (!op) continue;
        switch (op->type)
        {
            case AOP_REGISTER:
                if (op->content.reg == reg)
                    return true;
                break;
            case AOP_INDIRECT_REGISTER:
                if (op->content.inreg.id == reg)
                    return true;
                if (op->content.inreg.roffset == reg)
                    return true;
                break;
            default:
                break;
        }
    }
    return false;
}

air_insn_t* air_insn_find_temporary_definition_above(regid_t tmp, air_insn_t* start)
{
    for (; start; start = start->prev)
    {
        if (!air_insn_creates_temporary(start)) continue;
        air_insn_operand_t* op = start->ops[0];
        if (op->type != AOP_REGISTER) report_return_value(NULL);
        if (op->content.reg == tmp)
            return start;
    }
    return NULL;
}

air_insn_t* air_insn_find_temporary_definition_below(regid_t tmp, air_insn_t* start)
{
    for (; start; start = start->next)
    {
        if (!air_insn_creates_temporary(start)) continue;
        air_insn_operand_t* op = start->ops[0];
        if (op->type != AOP_REGISTER) report_return_value(NULL);
        if (op->content.reg == tmp)
            return start;
    }
    return NULL;
}

air_insn_t* air_insn_find_temporary_definition_from_insn(regid_t tmp, air_insn_t* start)
{
    air_insn_t* def = air_insn_find_temporary_definition_above(tmp, start);
    if (def) return def;
    return air_insn_find_temporary_definition_below(tmp, start);
}

air_insn_t* air_insn_find_temporary_definition(regid_t tmp, air_routine_t* routine)
{
    for (air_insn_t* insn = routine->insns; insn; insn = insn->next)
    {
        if (!air_insn_creates_temporary(insn)) continue;
        air_insn_operand_t* op = insn->ops[0];
        if (op->type != AOP_REGISTER) report_return_value(NULL);
        if (op->content.reg == tmp)
            return insn;
    }
    return NULL;
}

// prev insn inserting
air_insn_t* air_insn_insert_before(air_insn_t* insn, air_insn_t* inserting)
{
    if (!insn || !inserting) return NULL;
    air_insn_t* prev = inserting->prev;
    inserting->prev = insn;
    insn->next = inserting;
    insn->prev = prev;
    if (prev)
        prev->next = insn;
    return insn;
}

// insert after inserting
air_insn_t* air_insn_insert_after(air_insn_t* insn, air_insn_t* inserting)
{
    if (!insn || !inserting) return NULL;
    air_insn_t* next = inserting->next;
    inserting->next = insn;
    insn->prev = inserting;
    insn->next = next;
    if (next)
        next->prev = insn;
    return insn;
}

static void air_insn_remove_from_list(air_insn_t* insn)
{
    if (!insn) return;

    if (insn->prev)
        insn->prev->next = insn->next;

    if (insn->next)
        insn->next->prev = insn->prev;
    
    insn->prev = NULL;
    insn->next = NULL;
}

air_insn_t* air_insn_move_before(air_insn_t* insn, air_insn_t* inserting)
{
    if (!insn || !inserting) return NULL;

    air_insn_remove_from_list(insn);
    air_insn_insert_before(insn, inserting);

    return insn;
}

air_insn_t* air_insn_move_after(air_insn_t* insn, air_insn_t* inserting)
{
    if (!insn || !inserting) return NULL;

    air_insn_remove_from_list(insn);
    air_insn_insert_after(insn, inserting);

    return insn;
}

// prev insn next
air_insn_t* air_insn_remove(air_insn_t* insn)
{
    if (!insn) return NULL;

    air_insn_t* prev = insn->prev;

    air_insn_remove_from_list(insn);
    air_insn_delete(insn);

    return prev;
}

static air_insn_t* copy_code_impl(syntax_component_t* src, air_insn_t* start)
{
    if (!start)
        return NULL;
    if (!src)
        return start;
    if (!src->code)
        return start;
    for (air_insn_t* insn = src->code; insn; insn = insn->next)
        start->next = air_insn_copy(insn), start->next->prev = start, start = start->next;
    return start;
}

static air_insn_t* copy_air_insn_sequence(air_insn_t* code)
{
    if (!code) return NULL;
    air_insn_t* head = air_insn_copy(code);
    head->next = copy_air_insn_sequence(code->next);
    if (head->next) head->next->prev = head;
    return head;
}

#define SETUP_LINEARIZE \
    air_insn_t* dummy = calloc(1, sizeof *dummy); \
    dummy->type = AIR_NOP; \
    air_insn_t* code = dummy;

#define COPY_CODE(s) code = copy_code_impl(s, code)

#define FINALIZE_LINEARIZE \
    syn->code = dummy->next; \
    if (syn->code) syn->code->prev = NULL; \
    air_insn_delete(dummy);

#define ADD_CODE(insn) (code->next = (insn), code->next->prev = code, code = code->next)

#define ADD_SEQUENCE_POINT ADD_CODE(air_insn_init(AIR_SEQUENCE_POINT, 0))

// signed -> larger integer: sign extension
// signed -> smaller integer or same: truncate
// unsigned -> larger integer: zero extension
// unsigned -> smaller integer or same: truncate
// float -> double: xor double reg then cvtss2sd
// double -> float: xor float reg then cvtsd2ss
// float -> signed integer type: cvttss2si
// float -> unsigned integer type: brother
// double -> signed integer type: cvttsd2si
// double -> unsigned integer type: brother
// signed integer type -> float: cvtsi2ss
// unsigned integer type -> float: oh brother...
// signed integer type -> double: cvtsi2sd
// unsigned integer type -> double: oh brother againe
static regid_t convert(syntax_traverser_t* trav, c_type_t* from, c_type_t* to, regid_t reg, air_insn_t** c)
{
    air_insn_t* code = *c;
    air_insn_type_t type = AIR_NOP;

    bool ff = from->class == CTC_FLOAT;
    bool fd = from->class == CTC_DOUBLE || from->class == CTC_LONG_DOUBLE;
    bool tf = to->class == CTC_FLOAT;
    bool td = to->class == CTC_DOUBLE || to->class == CTC_LONG_DOUBLE;

    int operands = 2;
    
    if ((type_is_signed_integer(from) || from->class == CTC_CHAR) && type_is_integer(to) && 
        get_integer_conversion_rank(to) > get_integer_conversion_rank(from))
        type = AIR_SEXT;
    else if (type_is_unsigned_integer(from) && (type_is_integer(to) || to->class == CTC_CHAR) && 
        get_integer_conversion_rank(to) > get_integer_conversion_rank(from))
        type = AIR_ZEXT;
    else if (ff & td)
        type = AIR_S2D;
    else if (fd && tf)
        type = AIR_D2S;
    else if (ff && type_is_signed_integer(to))
        type = AIR_S2SI;
    else if (ff && type_is_unsigned_integer(to))
        type = AIR_S2UI;
    else if (fd && type_is_signed_integer(to))
        type = AIR_D2SI;
    else if (fd && type_is_unsigned_integer(to))
        type = AIR_D2UI;
    else if (type_is_signed_integer(from) && tf)
        type = AIR_SI2S;
    else if (type_is_unsigned_integer(from) && tf)
        type = AIR_UI2S;
    else if (type_is_signed_integer(from) && td)
        type = AIR_SI2D;
    else if (type_is_unsigned_integer(from) && td)
        type = AIR_UI2D;
    
    regid_t result = reg;
    if (type != AIR_NOP)
    {
        air_insn_t* insn = air_insn_init(type, operands);
        insn->ct = type_copy(to);
        result = NEXT_VIRTUAL_REGISTER;
        insn->ops[0] = air_insn_register_operand_init(result);
        insn->ops[1] = air_insn_register_operand_init(reg);
        insn->ops[1]->ct = type_copy(from);
        ADD_CODE(insn);
    }
    *c = code;
    return result;
}

static void linearize_function_definition_before(syntax_traverser_t* trav, syntax_component_t* syn)
{
    air_routine_t* routine = calloc(1, sizeof *routine);

    syntax_component_t* declr = syn->fdef_declarator;
    if (!declr) report_return;

    syntax_component_t* id = syntax_get_declarator_identifier(declr);
    if (!id) report_return;

    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;

    routine->sy = sy;

    vector_add(AIRINIZING_TRAVERSER->air->routines, routine);
    AIRINIZING_TRAVERSER->croutine = routine;
}

static void linearize_function_definition_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->fdef_body);
    FINALIZE_LINEARIZE;

    air_routine_t* routine = AIRINIZING_TRAVERSER->croutine;
    routine->insns = air_insn_init(AIR_NOP, 0);
    routine->insns->next = copy_air_insn_sequence(syn->code);
    if (routine->insns->next)
        routine->insns->next->prev = routine->insns;

    // implicit return 0 for main
    if (streq(symbol_get_name(routine->sy), "main"))
    {
        air_insn_t* last = routine->insns;
        for (; last->next; last = last->next);

        if (last->type != AIR_RETURN)
        {
            regid_t reg = NEXT_VIRTUAL_REGISTER;
            air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
            ld->ct = make_basic_type(CTC_INT);
            ld->ops[0] = air_insn_register_operand_init(reg);
            ld->ops[1] = air_insn_integer_constant_operand_init(0);
            last = air_insn_insert_after(ld, last);
            air_insn_t* ret = air_insn_init(AIR_RETURN, 1);
            ret->ct = make_basic_type(CTC_INT);
            ret->ops[0] = air_insn_register_operand_init(reg);
            last = air_insn_insert_after(ret, last);
        }
    }

    AIRINIZING_TRAVERSER->croutine = NULL;
}

static void linearize_static_declarator_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    if (!sy) report_return;
    // TODO
}

static void linearize_automatic_declarator_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    if (!sy) report_return;
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_DECLARE, 1);
    insn->ops[0] = air_insn_symbol_operand_init(sy);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_declarator_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    if (!sy) report_return;
    storage_duration_t sd = symbol_get_storage_duration(sy);
    if (sd == SD_STATIC)
        linearize_static_declarator_identifier_after(trav, syn);
    else
        linearize_automatic_declarator_identifier_after(trav, syn);
}

/*

exceptions to """typical""" pexpr identifier semantics (i.e., loading into a tempreg):
    - non-VLA typed sizeof, e.g., sizeof(x[0])          do nothing
    - declarators (unless it's a VLA maybe lolllll)     do nothing
    - lvalue context                                    

*/

static void linearize_primary_expression_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_namespace_t* ns = syntax_get_namespace(syn);
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, syn, ns);
    namespace_delete(ns);
    if (!sy) report_return;
    SETUP_LINEARIZE;
    air_insn_t* insn = NULL;
    if (!syntax_is_in_lvalue_context(syn) && !type_is_sua(sy->type) && sy->type->class != CTC_FUNCTION)
    {
        insn = air_insn_init(AIR_LOAD, 2);
        insn->ct = type_copy(syn->ctype);
    }
    else
    {
        insn = air_insn_init(AIR_LOAD_ADDR, 2);
        insn->ct = make_reference_type(sy->type);
    }
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_symbol_operand_init(sy);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_primary_expression_enumeration_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    c_namespace_t* ns = syntax_get_namespace(syn);
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, syn, ns);
    namespace_delete(ns);
    if (!sy) report_return;
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_integer_constant_operand_init(sy->declarer->parent->enumr_value);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_integer_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_integer_constant_operand_init(syn->intc);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_character_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_integer_constant_operand_init(syn->charc_value);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_floating_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    air_t* air = AIRINIZING_TRAVERSER->air;
    air_data_t* data = calloc(1, sizeof *data);
    data->readonly = true;
    data->sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    switch (syn->ctype->class)
    {
        case CTC_FLOAT:
            data->data = malloc(FLOAT_WIDTH);
            *((float*) (data->data)) = (float) syn->floc;
            break;
        case CTC_DOUBLE:
            data->data = malloc(DOUBLE_WIDTH);
            *((double*) (data->data)) = (double) syn->floc;
            break;
        case CTC_LONG_DOUBLE:
            data->data = malloc(LONG_DOUBLE_WIDTH);
            *((long double*) (data->data)) = (long double) syn->floc;
            break;
        default: report_return;
    }
    vector_add(air->rodata, data);

    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_symbol_operand_init(data->sy);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, expr, syn->expr_expressions)
    {
        COPY_CODE(expr);
        if (i != syn->expr_expressions->size - 1)
            ADD_SEQUENCE_POINT;
    }
    if (syn->expr_expressions)
    {
        syntax_component_t* last = vector_get(syn->expr_expressions, syn->expr_expressions->size - 1);
        syn->expr_reg = last->expr_reg;
    }
    FINALIZE_LINEARIZE;
}

static void linearize_expression_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->estmt_expression);
    ADD_SEQUENCE_POINT;
    FINALIZE_LINEARIZE;
}

static void linearize_return_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    if (syn->retstmt_expression)
    {
        COPY_CODE(syn->retstmt_expression);
        ADD_SEQUENCE_POINT;
        regid_t reg = syn->retstmt_expression->expr_reg;

        syntax_component_t* fdef = syntax_get_enclosing(syn, SC_FUNCTION_DEFINITION);
        if (!fdef) report_return;
        syntax_component_t* fdef_id = syntax_get_declarator_identifier(fdef->fdef_declarator);
        if (!fdef_id) report_return;
        symbol_t* fsy = symbol_table_get_syn_id(SYMBOL_TABLE, fdef_id);
        if (!fsy) report_return;

        reg = convert(trav, syn->retstmt_expression->ctype, fsy->type->derived_from, reg, &code);

        air_insn_t* insn = air_insn_init(AIR_RETURN, 1);
        if (syn->retstmt_expression->ctype->class == CTC_STRUCTURE ||
            syn->retstmt_expression->ctype->class == CTC_UNION)
            insn->ops[0] = air_insn_indirect_register_operand_init(reg, 0, INVALID_VREGID, 1);
        else
            insn->ops[0] = air_insn_register_operand_init(reg);
        ADD_CODE(insn);
    }
    else
    {
        air_insn_t* insn = air_insn_init(AIR_RETURN, 0);
        ADD_CODE(insn);
    }
    FINALIZE_LINEARIZE;
}

static void linearize_compound_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, stmt, syn->cstmt_block_items)
        COPY_CODE(stmt);
    FINALIZE_LINEARIZE;
}

static void linearize_function_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->fdeclr_direct);
    FINALIZE_LINEARIZE;
}

static void linearize_array_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->adeclr_direct);
    // TODO: VLAs (ugh)
    FINALIZE_LINEARIZE;
}

static void initialize_string_literal(syntax_component_t* strl, symbol_t* sy, int64_t base_offset, air_insn_t** c)
{
    air_insn_t* code = *c;
    int64_t array_length = type_get_array_length(sy->type);
    unsigned long long length = strl->strl_length->intc + 1;
    if (array_length != -1)
        length = min(length, array_length);
    char* str = strl->strl_reg;
    for (unsigned long long copied = 0; copied < length;)
    {
        unsigned long long remaining = length - copied;
        c_type_class_t class = CTC_UNSIGNED_LONG_LONG_INT;
        if (remaining < UNSIGNED_SHORT_INT_WIDTH)
            class = CTC_UNSIGNED_CHAR;
        else if (remaining < UNSIGNED_INT_WIDTH)
            class = CTC_UNSIGNED_SHORT_INT;
        else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
            class = CTC_UNSIGNED_INT;
        air_insn_t* loaddest = air_insn_init(AIR_ASSIGN, 2);
        loaddest->ct = make_basic_type(class);
        loaddest->ops[0] = air_insn_indirect_symbol_operand_init(sy, base_offset + copied);
        unsigned long long value = 0;
        switch (class)
        {
            case CTC_UNSIGNED_CHAR:
                value = *((unsigned char*) (str + copied));
                break;
            case CTC_UNSIGNED_SHORT_INT:
                value = *((unsigned short*) (str + copied));
                break;
            case CTC_UNSIGNED_INT:
                value = *((unsigned*) (str + copied));
                break;
            case CTC_UNSIGNED_LONG_LONG_INT:
                value = *((unsigned long long*) (str + copied));
                break;
            default:
                report_return;
        }
        loaddest->ops[1] = air_insn_integer_constant_operand_init(value);
        ADD_CODE(loaddest);
        copied += type_size(loaddest->ct);
    }
    *c = code;
}

static void initialize(syntax_traverser_t* trav, syntax_component_t* initializer, symbol_t* sy, int64_t base_offset, air_insn_t** c)
{
    if (initializer->type == SC_INITIALIZER_LIST)
    {
        VECTOR_FOR(syntax_component_t*, init, initializer->inlist_initializers)
            initialize(trav, init, sy, base_offset + initializer->initializer_offset, c);
        return;
    }
    air_insn_t* code = *c;

    c_type_t* ct = initializer->initializer_ctype;

    if (type_is_scalar(ct))
    {
        COPY_CODE(initializer);
        regid_t reg = convert(trav, initializer->ctype, initializer->initializer_ctype, initializer->expr_reg, &code);
        air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
        assign->ct = type_copy(ct);
        assign->ops[0] = air_insn_indirect_symbol_operand_init(sy, base_offset + initializer->initializer_offset);
        assign->ops[1] = air_insn_register_operand_init(reg);
        ADD_CODE(assign);
    }
    else if (initializer->type == SC_STRING_LITERAL && initializer->strl_reg && ct->class == CTC_ARRAY && type_is_character(ct->derived_from))
        initialize_string_literal(initializer, sy, base_offset + initializer->initializer_offset, &code);
    else if (initializer->type == SC_STRING_LITERAL && initializer->strl_wide && ct->class == CTC_ARRAY && type_is_wchar_compatible(ct->derived_from))
        // TODO
        report_return;

    *c = code;
}

static void linearize_initializer_list_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* enclosing = syn->parent;
    if (!enclosing) report_return;

    // nested initializer lists are handled in the highest initializer list
    if (enclosing->type == SC_INITIALIZER_LIST)
    {
        SETUP_LINEARIZE;
        ADD_SEQUENCE_POINT;
        FINALIZE_LINEARIZE;
        return;
    }

    symbol_t* sy = NULL;
    if (enclosing->type == SC_INIT_DECLARATOR)
    {
        syntax_component_t* id = syntax_get_declarator_identifier(enclosing->ideclr_declarator);
        if (!id) report_return;
        sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    }
    else if (enclosing->type == SC_COMPOUND_LITERAL)
        sy = symbol_table_get_syn_id(SYMBOL_TABLE, enclosing);
    if (!sy) report_return;

    bool is_scalar = type_is_scalar(sy->type);
    bool is_char_array = sy->type->class == CTC_ARRAY && type_is_character(sy->type->derived_from);
    bool is_wchar_array = sy->type->class == CTC_ARRAY && type_is_wchar_compatible(sy->type);

    if (syn->inlist_initializers->size == 1)
    {
        syntax_component_t* inner = vector_get(syn->inlist_initializers, 0);
        is_scalar = is_scalar && inner->type != SC_INITIALIZER_LIST && type_is_scalar(inner->ctype);
        is_char_array = is_char_array && inner->type == SC_STRING_LITERAL && inner->strl_reg;
        is_wchar_array = is_wchar_array && inner->type == SC_STRING_LITERAL && inner->strl_wide;
        // singly-nested braces special case is handled during init declarator linearization
        if (is_scalar || is_char_array || is_wchar_array)
        {
            syntax_component_t* init = vector_get(syn->inlist_initializers, 0);
            SETUP_LINEARIZE;
            COPY_CODE(init);
            FINALIZE_LINEARIZE;
            return;
        }
    }

    storage_duration_t sd = symbol_get_storage_duration(sy);

    if (sd == SD_STATIC)
        return;

    SETUP_LINEARIZE;

    air_insn_t* ms = air_insn_init(AIR_MEMSET, 3);
    ms->ct = make_reference_type(sy->type);
    ms->ops[0] = air_insn_integer_constant_operand_init(0);
    ms->ops[1] = air_insn_symbol_operand_init(sy);
    ms->ops[2] = air_insn_integer_constant_operand_init(type_size(sy->type));
    ADD_CODE(ms);

    initialize(trav, syn, sy, 0, &code);

    ADD_SEQUENCE_POINT;

    FINALIZE_LINEARIZE;
}

static void linearize_static_init_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy, air_insn_t** c)
{
    air_insn_t* code = *c;

    if (sy->type->class == CTC_FUNCTION ||
        syntax_has_specifier(syn->parent->decl_declaration_specifiers, SC_STORAGE_CLASS_SPECIFIER, SCS_TYPEDEF))
        return;

    air_data_t* data = calloc(1, sizeof *data);
    data->sy = sy;
    data->readonly = false;
    long long size = type_size(sy->type);
    data->data = calloc(size, sizeof(unsigned char));
    if (sy->data)
        memcpy(data->data, sy->data, size);
    data->addresses = vector_deep_copy(sy->addresses, (void* (*)(void*)) init_address_copy);

    vector_add(AIRINIZING_TRAVERSER->air->data, data);

    *c = code;
}

static void linearize_init_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* init = syn->ideclr_initializer;
    COPY_CODE(syn->ideclr_declarator);
    COPY_CODE(init);
    syntax_component_t* id = syntax_get_declarator_identifier(syn->ideclr_declarator);
    if (!id) report_return;
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
    if (!sy) report_return;

    storage_duration_t sd = symbol_get_storage_duration(sy);

    if (sd == SD_STATIC)
    {
        linearize_static_init_declarator_after(trav, syn, sy, &code);
        FINALIZE_LINEARIZE;
        return;
    }

    if (!init)
    {
        FINALIZE_LINEARIZE;
        return;
    }

    bool is_scalar = type_is_scalar(sy->type);
    bool is_char_array = sy->type->class == CTC_ARRAY && type_is_character(sy->type->derived_from);

    c_type_t* wct = make_basic_type(C_TYPE_WCHAR_T);
    bool is_wchar_array = sy->type->class == CTC_ARRAY && type_is_compatible(sy->type, wct);
    type_delete(wct);

    if (init->type == SC_INITIALIZER_LIST && init->inlist_initializers->size == 1)
    {
        syntax_component_t* inner = vector_get(init->inlist_initializers, 0);
        if (is_scalar && inner->type != SC_INITIALIZER_LIST && type_is_scalar(inner->ctype))
            init = inner;
        if (is_char_array && inner->type == SC_STRING_LITERAL && inner->strl_reg)
            init = inner;
        if (is_wchar_array && inner->type == SC_STRING_LITERAL && inner->strl_wide)
            init = inner;
    }

    if (init->type == SC_INITIALIZER_LIST)
    {
        FINALIZE_LINEARIZE;
        return;
    }

    // ISO: 6.7.8 (11)
    if (is_scalar)
    {
        regid_t reg = convert(trav, init->ctype, sy->type, init->expr_reg, &code);
        air_insn_t* insn = air_insn_init(AIR_ASSIGN, 2);
        insn->ct = type_copy(sy->type);
        insn->ops[0] = air_insn_symbol_operand_init(sy);
        insn->ops[1] = air_insn_register_operand_init(reg);
        ADD_CODE(insn);
    }

    // ISO: 6.7.8 (14)
    else if (is_char_array)
        initialize_string_literal(init, sy, 0, &code);

    // ISO: 6.7.8 (15)
    else if (is_wchar_array)
    {
        // TODO
    }

    // ISO: 6.7.8 (13)
    else
    {
        if (sy->type->class != CTC_STRUCTURE && sy->type->class != CTC_UNION) report_return;
        long long size = type_size(sy->type);
        for (long long copied = 0; copied < size;)
        {
            long long remaining = size - copied;
            c_type_class_t class = CTC_UNSIGNED_LONG_LONG_INT;
            if (remaining < UNSIGNED_SHORT_INT_WIDTH)
                class = CTC_UNSIGNED_CHAR;
            else if (remaining < UNSIGNED_INT_WIDTH)
                class = CTC_UNSIGNED_SHORT_INT;
            else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
                class = CTC_UNSIGNED_INT;
            air_insn_t* loadsrc = air_insn_init(AIR_LOAD, 2);
            loadsrc->ct = make_basic_type(class);
            regid_t srcreg = NEXT_VIRTUAL_REGISTER;
            loadsrc->ops[0] = air_insn_register_operand_init(srcreg);
            loadsrc->ops[1] = air_insn_indirect_register_operand_init(init->expr_reg, copied, INVALID_VREGID, 1);
            ADD_CODE(loadsrc);
            air_insn_t* loaddest = air_insn_init(AIR_ASSIGN, 2);
            loaddest->ct = make_basic_type(class);
            loaddest->ops[0] = air_insn_indirect_symbol_operand_init(sy, copied);
            loaddest->ops[1] = air_insn_register_operand_init(srcreg);
            ADD_CODE(loaddest);
            copied += type_size(loadsrc->ct);
        }
    }

    ADD_SEQUENCE_POINT;
    FINALIZE_LINEARIZE;
}

static void linearize_declaration_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, declspec, syn->decl_declaration_specifiers)
        COPY_CODE(declspec);
    VECTOR_FOR(syntax_component_t*, ideclr, syn->decl_init_declarators)
        COPY_CODE(ideclr);
    FINALIZE_LINEARIZE;
}

static void linearize_subscript_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    syntax_component_t* obj = syn->bexpr_lhs;
    syntax_component_t* idx = syn->bexpr_rhs;
    if (type_is_integer(obj->ctype))
    {
        syntax_component_t* tmp = obj;
        obj = idx;
        idx = tmp;
    }
    air_insn_t* sizeup = air_insn_init(AIR_MULTIPLY, 3);
    sizeup->ct = type_copy(idx->ctype);
    regid_t sureg = NEXT_VIRTUAL_REGISTER;
    sizeup->ops[0] = air_insn_register_operand_init(sureg);
    sizeup->ops[1] = air_insn_register_operand_init(idx->expr_reg);
    sizeup->ops[2] = air_insn_integer_constant_operand_init(type_size(obj->ctype->derived_from));
    ADD_CODE(sizeup);
    air_insn_t* insn = NULL;
    c_type_t* mt = obj->ctype->derived_from;
    if (syntax_is_in_lvalue_context(syn) ||
        // obtaining an aggregate or union via subscript will yield its offset in the array 
        type_is_sua(mt))
    {
        insn = air_insn_init(AIR_ADD, 3);
        if (mt->class == CTC_ARRAY)
            insn->ct = type_copy(syn->ctype);
        else if (mt->class == CTC_STRUCTURE || mt->class == CTC_UNION)
            insn->ct = make_reference_type(syn->ctype);
        else
            insn->ct = type_copy(obj->ctype);
        insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
        insn->ops[1] = air_insn_register_operand_init(syn->bexpr_lhs->expr_reg);
        insn->ops[2] = air_insn_register_operand_init(sureg);
    }
    else
    {
        insn = air_insn_init(AIR_LOAD, 2);
        insn->ct = type_copy(syn->ctype);
        insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
        insn->ops[1] = air_insn_indirect_register_operand_init(syn->bexpr_lhs->expr_reg, 0, sureg, 1);
    }
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_string_literal_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* parent = syn->parent;
    if (parent && parent->type == SC_INITIALIZER_LIST)
        parent = parent->parent;
    if (parent->type == SC_INIT_DECLARATOR)
    {
        syntax_component_t* id = syntax_get_declarator_identifier(parent->ideclr_declarator);
        symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
        if (!sy) report_return;
        if (sy->type->class == CTC_ARRAY && type_is_character(sy->type->derived_from))
            return;
    }
    air_t* air = AIRINIZING_TRAVERSER->air;
    air_data_t* data = calloc(1, sizeof *data);
    data->readonly = true;
    data->sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    if (syn->strl_reg)
    {
        data->data = calloc(syn->strl_length->intc + 1, sizeof(unsigned char));
        memcpy(data->data, syn->strl_reg, syn->strl_length->intc + 1);
    }
    else
    {
        data->data = calloc(syn->strl_length->intc + 1, sizeof(int));
        memcpy(data->data, syn->strl_wide, sizeof(int) * (syn->strl_length->intc + 1));
    }
    vector_add(air->rodata, data);
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_LOAD_ADDR, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_symbol_operand_init(data->sy);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static air_insn_t* add_function_call_arg(syntax_traverser_t* trav, syntax_component_t* syn, int i, air_insn_t* insn, air_insn_t* code)
{
    c_type_t* ftype = syn->fcallexpr_expression->ctype->derived_from;
    syntax_component_t* arg = vector_get(syn->fcallexpr_args, i);
    COPY_CODE(arg);
    regid_t reg = arg->expr_reg;
    if (ftype->function.param_types && i < ftype->function.param_types->size)
        reg = convert(trav, arg->ctype, vector_get(ftype->function.param_types, i), reg, &code);
    else
    {
        // default argument promotions
        c_type_t* at = integer_promotions(arg->ctype);
        if (at->class == CTC_FLOAT)
            at->class = CTC_DOUBLE;
        reg = convert(trav, arg->ctype, at, reg, &code);
        type_delete(at);
    }
    if (arg->ctype->class == CTC_STRUCTURE || arg->ctype->class == CTC_UNION)
        insn->ops[i + 2] = air_insn_indirect_register_operand_init(reg, 0, INVALID_VREGID, 1);
    else
        insn->ops[i + 2] = air_insn_register_operand_init(reg);
    return code;
}

static void linearize_function_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_FUNC_CALL, syn->fcallexpr_args->size + 2);
    for (int i = syn->fcallexpr_args->size - 1; i >= 0; --i)
    {
        syntax_component_t* arg = vector_get(syn->fcallexpr_args, i);
        if (!syntax_contains_subelement(arg, SC_FUNCTION_CALL_EXPRESSION))
            continue;
        code = add_function_call_arg(trav, syn, i, insn, code);
    }
    for (int i = 0; i < syn->fcallexpr_args->size; ++i)
    {
        if (insn->ops[i + 2])
            continue;
        code = add_function_call_arg(trav, syn, i, insn, code);
    }
    ADD_SEQUENCE_POINT;
    COPY_CODE(syn->fcallexpr_expression);
    if (syn->ctype->class == CTC_STRUCTURE || syn->ctype->class == CTC_UNION)
    {
        insn->ct = make_reference_type(syn->ctype);
        insn->metadata.fcall_sret = true;
    }
    else
        insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_register_operand_init(syn->fcallexpr_expression->expr_reg);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_member_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->memexpr_expression);
    int64_t offset = 0;
    long long idx = 0;
    c_type_t* ct = syn->memexpr_expression->ctype;
    if (syn->type == SC_DEREFERENCE_MEMBER_EXPRESSION)
        ct = ct->derived_from;
    type_get_struct_union_member_info(ct, syn->memexpr_id->id, &idx, &offset);
    c_type_t* mt = vector_get(ct->struct_union.member_types, idx);
    air_insn_t* insn = NULL;
    if (syntax_is_in_lvalue_context(syn) ||
        // accessing an aggregate or union type will just get the offset of that object within the struct
        type_is_sua(mt))
    {
        insn = air_insn_init(AIR_ADD, 3);
        if (mt->class == CTC_ARRAY)
            insn->ct = type_copy(syn->ctype);
        else
            insn->ct = make_reference_type(mt);
        insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
        insn->ops[1] = air_insn_register_operand_init(syn->memexpr_expression->expr_reg);
        insn->ops[2] = air_insn_integer_constant_operand_init(offset);
    }
    else
    {
        insn = air_insn_init(AIR_LOAD, 2);
        insn->ct = type_copy(syn->ctype);
        insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
        insn->ops[1] = air_insn_indirect_register_operand_init(syn->bexpr_lhs->expr_reg, offset, INVALID_VREGID, 1);
    }
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->declr_direct);
    FINALIZE_LINEARIZE;
}

static void linearize_increment_decrement_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->uexpr_operand);
    bool add = syn->type == SC_PREFIX_INCREMENT_EXPRESSION || syn->type == SC_POSTFIX_INCREMENT_EXPRESSION;
    bool prefix = syn->type == SC_PREFIX_INCREMENT_EXPRESSION || syn->type == SC_PREFIX_DECREMENT_EXPRESSION;
    air_insn_t* chg = air_insn_init(add ? AIR_DIRECT_ADD : AIR_DIRECT_SUBTRACT, 2);
    chg->ct = type_copy(syn->uexpr_operand->ctype);
    chg->ops[0] = air_insn_indirect_register_operand_init(syn->uexpr_operand->expr_reg, 0, INVALID_VREGID, 1);
    chg->ops[1] = air_insn_integer_constant_operand_init(syn->uexpr_operand->ctype->class == CTC_POINTER ? type_size(syn->uexpr_operand->ctype->derived_from) : 1);
    air_insn_t* access = air_insn_init(AIR_LOAD, 2);
    access->ct = type_copy(syn->uexpr_operand->ctype);
    access->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    access->ops[1] = air_insn_indirect_register_operand_init(syn->uexpr_operand->expr_reg, 0, INVALID_VREGID, 1);
    if (prefix)
    {
        ADD_CODE(chg);
        ADD_CODE(access);
    }
    else
    {
        ADD_CODE(access);
        ADD_CODE(chg);
    }
    FINALIZE_LINEARIZE;
}

static void linearize_automatic_compound_literal_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy)
{
    SETUP_LINEARIZE;
    air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
    decl->ct = type_copy(sy->type);
    decl->ops[0] = air_insn_symbol_operand_init(sy);
    ADD_CODE(decl);
    COPY_CODE(syn->cl_type_name);
    COPY_CODE(syn->cl_inlist);
    air_insn_t* insn = air_insn_init(AIR_LOAD_ADDR, 2);
    insn->ct = make_reference_type(sy->type);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_symbol_operand_init(sy);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_static_compound_literal_after(syntax_traverser_t* trav, syntax_component_t* syn, symbol_t* sy)
{
    air_data_t* data = calloc(1, sizeof *data);
    data->sy = sy;
    data->readonly = false;
    long long size = type_size(sy->type);
    data->data = malloc(size);
    memcpy(data->data, sy->data, size);
    data->addresses = vector_deep_copy(sy->addresses, (void* (*)(void*)) init_address_copy);
    vector_add(AIRINIZING_TRAVERSER->air->data, data);
}

static void linearize_compound_literal_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    symbol_t* sy = symbol_table_get_syn_id(SYMBOL_TABLE, syn);
    if (!sy) report_return;
    storage_duration_t sd = symbol_get_storage_duration(sy);
    if (sd == SD_STATIC)
        linearize_static_compound_literal_after(trav, syn, sy);
    else
        linearize_automatic_compound_literal_after(trav, syn, sy);
}

static void linearize_type_name_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->tn_declarator);
    VECTOR_FOR(syntax_component_t*, spec, syn->tn_specifier_qualifier_list)
        COPY_CODE(spec);
    FINALIZE_LINEARIZE;
}

static void linearize_reference_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->uexpr_operand);
    syn->expr_reg = syn->uexpr_operand->expr_reg;
    FINALIZE_LINEARIZE;
}

static void linearize_unary_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->uexpr_operand);
    air_insn_type_t type = AIR_NOP;
    switch (syn->type)
    {
        case SC_DEREFERENCE_EXPRESSION: type = AIR_LOAD; break;
        case SC_MINUS_EXPRESSION: type = AIR_NEGATE; break;
        case SC_PLUS_EXPRESSION: type = AIR_POSATE; break;
        case SC_COMPLEMENT_EXPRESSION: type = AIR_COMPLEMENT; break;
        case SC_NOT_EXPRESSION: type = AIR_NOT; break;
        default: report_return;
    }
    air_insn_t* insn = air_insn_init(type, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    regid_t srcreg = syn->uexpr_operand->expr_reg;
    c_type_t* ut = NULL;
    if (syn->type == SC_NOT_EXPRESSION)
    {
        c_type_t* int_type = make_basic_type(CTC_INT);
        ut = usual_arithmetic_conversions_result_type(syn->uexpr_operand->ctype, int_type);
        srcreg = convert(trav, syn->uexpr_operand->ctype, ut, srcreg, &code);
        type_delete(int_type);
    }
    if (syn->type == SC_DEREFERENCE_EXPRESSION)
    {
        c_type_t* dt = syn->uexpr_operand->ctype->derived_from;
        if (dt->class == CTC_STRUCTURE || dt->class == CTC_UNION)
        {
            type_delete(insn->ct);
            insn->ct = make_reference_type(dt);
        }
        if (!type_is_sua(dt))
            insn->ops[1] = air_insn_indirect_register_operand_init(srcreg, 0, INVALID_VREGID, 1);
        else
            insn->ops[1] = air_insn_register_operand_init(srcreg);
    }
    else
        insn->ops[1] = air_insn_register_operand_init(srcreg);
    if (syn->type == SC_NOT_EXPRESSION)
        insn->ops[1]->ct = ut;
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_sizeof_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    long long size = type_size(syn->uexpr_operand->ctype);
    if (size == -1)
        // TODO: VLA garbage
        report_return;
    air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
    insn->ct = make_basic_type(C_TYPE_SIZE_T);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_integer_constant_operand_init(size);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_sizeof_type_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    c_type_t* ct = create_type(syn->uexpr_operand, syn->uexpr_operand->tn_declarator);
    long long size = type_size(ct);
    type_delete(ct);
    if (size == -1)
        report_return;
    air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
    insn->ct = make_basic_type(C_TYPE_SIZE_T);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_integer_constant_operand_init(size);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_cast_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->caexpr_operand);
    syn->expr_reg = convert(trav, syn->caexpr_operand->ctype, syn->ctype, syn->caexpr_operand->expr_reg, &code);
    FINALIZE_LINEARIZE;
}

static void linearize_assignment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    air_insn_type_t type = AIR_NOP;
    switch (syn->type)
    {
        case SC_ASSIGNMENT_EXPRESSION:
            type = AIR_ASSIGN;
            break;
        case SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_MULTIPLY;
            break;
        case SC_DIVISION_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_DIVIDE;
            break;
        case SC_MODULAR_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_MODULO;
            break;
        case SC_ADDITION_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_ADD;
            break;
        case SC_SUBTRACTION_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_SUBTRACT;
            break;
        case SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_SHIFT_LEFT;
            break;
        case SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION:
            type = type_is_signed_integer(syn->bexpr_lhs->ctype) ? AIR_DIRECT_SIGNED_SHIFT_RIGHT : AIR_DIRECT_SHIFT_RIGHT;
            break;
        case SC_BITWISE_OR_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_OR;
            break;
        case SC_BITWISE_AND_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_AND;
            break;
        case SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION:
            type = AIR_DIRECT_XOR;
            break;
        default:
            report_return;
    }
    regid_t rhs_reg = syn->bexpr_rhs->expr_reg;
    long long lhs_deref_size = 0;
    if (syn->bexpr_lhs->ctype->class == CTC_POINTER &&
        syn->bexpr_rhs->ctype->class != CTC_POINTER &&
        (lhs_deref_size = type_size(syn->bexpr_lhs->ctype->derived_from)) != 1)
    {
        air_insn_t* mul = air_insn_init(AIR_MULTIPLY, 3);
        mul->ct = type_copy(syn->bexpr_rhs->ctype);
        regid_t multiplied_reg = NEXT_VIRTUAL_REGISTER;
        mul->ops[0] = air_insn_register_operand_init(multiplied_reg);
        mul->ops[1] = air_insn_register_operand_init(rhs_reg);
        mul->ops[2] = air_insn_integer_constant_operand_init(lhs_deref_size);
        ADD_CODE(mul);
        rhs_reg = multiplied_reg;
    }
    if (type_is_scalar(syn->ctype))
    {
        syn->expr_reg = convert(trav, syn->bexpr_rhs->ctype, syn->ctype, rhs_reg, &code);
        air_insn_t* insn = air_insn_init(type, 2);
        insn->ct = type_copy(syn->ctype);
        insn->ops[0] = air_insn_indirect_register_operand_init(syn->bexpr_lhs->expr_reg, 0, INVALID_VREGID, 1);
        insn->ops[1] = air_insn_register_operand_init(syn->expr_reg);
        ADD_CODE(insn);
    }
    else
        report_return;
    if (syn->type != SC_ASSIGNMENT_EXPRESSION)
    {
        air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
        insn->ct = type_copy(syn->ctype);
        insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
        insn->ops[1] = air_insn_indirect_register_operand_init(syn->bexpr_lhs->expr_reg, 0, INVALID_VREGID, 1);
        ADD_CODE(insn);
    }
    FINALIZE_LINEARIZE;
}

static void linearize_binary_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    air_insn_type_t type = AIR_NOP;
    switch (syn->type)
    {
        case SC_MULTIPLICATION_EXPRESSION:
            type = AIR_MULTIPLY;
            break;
        case SC_DIVISION_EXPRESSION:
            type = AIR_DIVIDE;
            break;
        case SC_MODULAR_EXPRESSION:
            type = AIR_MODULO;
            break;
        case SC_BITWISE_LEFT_EXPRESSION:
            type = AIR_SHIFT_LEFT;
            break;
        case SC_BITWISE_RIGHT_EXPRESSION:
            type = type_is_signed_integer(syn->bexpr_lhs->ctype) ? AIR_SIGNED_SHIFT_RIGHT : AIR_SHIFT_RIGHT;
            break;
        case SC_LESS_EQUAL_EXPRESSION:
            type = AIR_LESS_EQUAL;
            break;
        case SC_LESS_EXPRESSION:
            type = AIR_LESS;
            break;
        case SC_GREATER_EQUAL_EXPRESSION:
            type = AIR_GREATER_EQUAL;
            break;
        case SC_GREATER_EXPRESSION:
            type = AIR_GREATER;
            break;
        case SC_EQUALITY_EXPRESSION:
            type = AIR_EQUAL;
            break;
        case SC_INEQUALITY_EXPRESSION:
            type = AIR_INEQUAL;
            break;
        case SC_BITWISE_AND_EXPRESSION:
            type = AIR_AND;
            break;
        case SC_BITWISE_XOR_EXPRESSION:
            type = AIR_XOR;
            break;
        case SC_BITWISE_OR_EXPRESSION:
            type = AIR_OR;
            break;
        default:
            report_return;
    }
    regid_t lreg = syn->bexpr_lhs->expr_reg;
    regid_t rreg = syn->bexpr_rhs->expr_reg;
    c_type_t* opt = NULL;
    if ((syntax_is_relational_expression_type(syn->type) || syntax_is_equality_expression_type(syn->type)) &&
        type_is_arithmetic(syn->bexpr_lhs->ctype) &&
        type_is_arithmetic(syn->bexpr_rhs->ctype))
        opt = usual_arithmetic_conversions_result_type(syn->bexpr_lhs->ctype, syn->bexpr_rhs->ctype);
    else
        opt = type_copy(syn->ctype);
    lreg = convert(trav, syn->bexpr_lhs->ctype, opt, lreg, &code);
    rreg = convert(trav, syn->bexpr_rhs->ctype, opt, rreg, &code);
    air_insn_t* insn = air_insn_init(type, 3);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_register_operand_init(lreg);
    insn->ops[1]->ct = type_copy(opt);
    insn->ops[2] = air_insn_register_operand_init(rreg);
    insn->ops[2]->ct = type_copy(opt);
    ADD_CODE(insn);
    type_delete(opt);
    FINALIZE_LINEARIZE;
}

static void linearize_ptr_offset_expression_after(syntax_traverser_t* trav, syntax_component_t* syn, air_insn_type_t type)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    bool scale_on_left = syn->bexpr_rhs->ctype->class == CTC_POINTER;
    regid_t reg = scale_on_left ? syn->bexpr_lhs->expr_reg : syn->bexpr_rhs->expr_reg;
    long long size = type_size(scale_on_left ? syn->bexpr_rhs->ctype->derived_from : syn->bexpr_lhs->ctype->derived_from);
    air_insn_t* mul = NULL;
    if (size != 1)
    {
        mul = air_insn_init(AIR_MULTIPLY, 3);
        mul->ct = type_copy(scale_on_left ? syn->bexpr_lhs->ctype : syn->bexpr_rhs->ctype);
        mul->ops[0] = air_insn_register_operand_init(reg = NEXT_VIRTUAL_REGISTER);
        mul->ops[1] = air_insn_register_operand_init(scale_on_left ? syn->bexpr_lhs->expr_reg : syn->bexpr_rhs->expr_reg);
        mul->ops[2] = air_insn_integer_constant_operand_init(size);
    }
    if (mul && scale_on_left)
        ADD_CODE(mul);
    COPY_CODE(syn->bexpr_rhs);
    if (mul && !scale_on_left)
        ADD_CODE(mul);
    air_insn_t* insn = air_insn_init(type, 3);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_register_operand_init(scale_on_left ? reg : syn->bexpr_lhs->expr_reg);
    insn->ops[2] = air_insn_register_operand_init(scale_on_left ? syn->bexpr_rhs->expr_reg : reg);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_addition_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (syn->bexpr_lhs->ctype->class == CTC_POINTER ||
        syn->bexpr_rhs->ctype->class == CTC_POINTER)
    {
        linearize_ptr_offset_expression_after(trav, syn, AIR_ADD);
        return;
    }
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    regid_t lreg = syn->bexpr_lhs->expr_reg;
    regid_t rreg = syn->bexpr_rhs->expr_reg;
    lreg = convert(trav, syn->bexpr_lhs->ctype, syn->ctype, lreg, &code);
    rreg = convert(trav, syn->bexpr_rhs->ctype, syn->ctype, rreg, &code);
    air_insn_t* insn = air_insn_init(AIR_ADD, 3);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_register_operand_init(lreg);
    insn->ops[2] = air_insn_register_operand_init(rreg);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_ptrdiff_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    air_insn_t* insn = air_insn_init(AIR_SUBTRACT, 3);
    insn->ct = make_basic_type(C_TYPE_PTRSIZE_T);
    regid_t sub_reg = NEXT_VIRTUAL_REGISTER;
    insn->ops[0] = air_insn_register_operand_init(sub_reg);
    insn->ops[1] = air_insn_register_operand_init(syn->bexpr_lhs->expr_reg);
    insn->ops[2] = air_insn_register_operand_init(syn->bexpr_rhs->expr_reg);
    ADD_CODE(insn);
    long long size = type_size(syn->bexpr_lhs->ctype->derived_from);
    air_insn_t* div = air_insn_init(AIR_DIVIDE, 3);
    div->ct = make_basic_type(C_TYPE_PTRSIZE_T);
    div->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    div->ops[1] = air_insn_register_operand_init(sub_reg);
    div->ops[2] = air_insn_integer_constant_operand_init(size);
    ADD_CODE(div);
    FINALIZE_LINEARIZE;
}

static void linearize_subtraction_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    bool lhs_ptr = syn->bexpr_lhs->ctype->class == CTC_POINTER;
    bool rhs_ptr = syn->bexpr_rhs->ctype->class == CTC_POINTER;
    if (lhs_ptr && rhs_ptr)
    {
        linearize_ptrdiff_expression_after(trav, syn);
        return;
    }
    if (lhs_ptr || rhs_ptr)
    {
        linearize_ptr_offset_expression_after(trav, syn, AIR_SUBTRACT);
        return;
    }
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    regid_t lreg = syn->bexpr_lhs->expr_reg;
    regid_t rreg = syn->bexpr_rhs->expr_reg;
    lreg = convert(trav, syn->bexpr_lhs->ctype, syn->ctype, lreg, &code);
    rreg = convert(trav, syn->bexpr_rhs->ctype, syn->ctype, rreg, &code);
    air_insn_t* insn = air_insn_init(AIR_SUBTRACT, 3);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_register_operand_init(lreg);
    insn->ops[2] = air_insn_register_operand_init(rreg);
    ADD_CODE(insn);
    FINALIZE_LINEARIZE;
}

static void linearize_logical_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    bool or = syn->type == SC_LOGICAL_OR_EXPRESSION;

    unsigned long long first_label_no = NEXT_LABEL;
    unsigned long long last_label_no = NEXT_LABEL;

    COPY_CODE(syn->bexpr_lhs);

    ADD_SEQUENCE_POINT;

    air_insn_t* jzl = air_insn_init(or ? AIR_JNZ : AIR_JZ, 2);
    jzl->ct = type_copy(syn->bexpr_lhs->ctype);
    jzl->ops[0] = air_insn_label_operand_init(first_label_no, 'E');
    jzl->ops[1] = air_insn_register_operand_init(syn->bexpr_lhs->expr_reg);
    ADD_CODE(jzl);

    COPY_CODE(syn->bexpr_rhs);

    air_insn_t* jzr = air_insn_init(or ? AIR_JNZ : AIR_JZ, 2);
    jzr->ct = type_copy(syn->bexpr_rhs->ctype);
    jzr->ops[0] = air_insn_label_operand_init(first_label_no, 'E');
    jzr->ops[1] = air_insn_register_operand_init(syn->bexpr_rhs->expr_reg);
    ADD_CODE(jzr);

    regid_t lastreg = NEXT_VIRTUAL_REGISTER;
    air_insn_t* last = air_insn_init(AIR_LOAD, 2);
    last->ct = make_basic_type(CTC_INT);
    last->ops[0] = air_insn_register_operand_init(lastreg);
    last->ops[1] = air_insn_integer_constant_operand_init(or ? 0 : 1);
    ADD_CODE(last);

    air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
    jmp->ops[0] = air_insn_label_operand_init(last_label_no, 'E');
    ADD_CODE(jmp);

    air_insn_t* fail_label = air_insn_init(AIR_LABEL, 1);
    fail_label->ops[0] = air_insn_label_operand_init(first_label_no, 'E');
    ADD_CODE(fail_label);

    regid_t firstreg = NEXT_VIRTUAL_REGISTER;
    air_insn_t* first = air_insn_init(AIR_LOAD, 2);
    first->ct = make_basic_type(CTC_INT);
    first->ops[0] = air_insn_register_operand_init(firstreg);
    first->ops[1] = air_insn_integer_constant_operand_init(or ? 1 : 0);
    ADD_CODE(first);

    air_insn_t* pass_label = air_insn_init(AIR_LABEL, 1);
    pass_label->ops[0] = air_insn_label_operand_init(last_label_no, 'E');
    ADD_CODE(pass_label);

    air_insn_t* phi = air_insn_init(AIR_PHI, 3);
    phi->ct = make_basic_type(CTC_INT);
    phi->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    phi->ops[1] = air_insn_register_operand_init(lastreg);
    phi->ops[2] = air_insn_register_operand_init(firstreg);
    ADD_CODE(phi);

    FINALIZE_LINEARIZE;
}

static void linearize_conditional_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    COPY_CODE(syn->cexpr_condition);

    ADD_SEQUENCE_POINT;

    unsigned long long else_label_no = NEXT_LABEL;
    unsigned long long end_label_no = NEXT_LABEL;

    regid_t ifreg = syn->cexpr_if->expr_reg;
    regid_t elsereg = syn->cexpr_else->expr_reg;

    air_insn_t* jz = air_insn_init(AIR_JZ, 2);
    jz->ct = type_copy(syn->cexpr_condition->ctype);
    jz->ops[0] = air_insn_label_operand_init(else_label_no, 'E');
    jz->ops[1] = air_insn_register_operand_init(syn->cexpr_condition->expr_reg);
    ADD_CODE(jz);

    COPY_CODE(syn->cexpr_if);
    ifreg = convert(trav, syn->cexpr_if->ctype, syn->ctype, ifreg, &code);

    air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
    jmp->ops[0] = air_insn_label_operand_init(end_label_no, 'E');
    ADD_CODE(jmp);

    air_insn_t* else_label = air_insn_init(AIR_LABEL, 1);
    else_label->ops[0] = air_insn_label_operand_init(else_label_no, 'E');
    ADD_CODE(else_label);

    COPY_CODE(syn->cexpr_else);
    elsereg = convert(trav, syn->cexpr_else->ctype, syn->ctype, elsereg, &code);

    air_insn_t* end_label = air_insn_init(AIR_LABEL, 1);
    end_label->ops[0] = air_insn_label_operand_init(end_label_no, 'E');
    ADD_CODE(end_label);

    air_insn_t* phi = air_insn_init(AIR_PHI, 3);
    phi->ct = type_copy(syn->ctype);
    phi->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    phi->ops[1] = air_insn_register_operand_init(ifreg);
    phi->ops[2] = air_insn_register_operand_init(elsereg);
    ADD_CODE(phi);

    FINALIZE_LINEARIZE;
}

static void linearize_labeled_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    air_insn_t* insn = air_insn_init(AIR_LABEL, 1);
    insn->ops[0] = air_insn_label_operand_init(syn->lstmt_uid, 'L');
    ADD_CODE(insn);
    COPY_CODE(syn->lstmt_stmt);
    FINALIZE_LINEARIZE;
}

static void linearize_goto_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    c_namespace_t* ns = syntax_get_namespace(syn->gtstmt_label_id);
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, syn->gtstmt_label_id, ns);
    namespace_delete(ns);
    if (!sy) report_return;

    air_insn_t* insn = air_insn_init(AIR_JMP, 1);
    insn->ops[0] = air_insn_label_operand_init(sy->declarer->parent->lstmt_uid, 'L');
    ADD_CODE(insn);

    FINALIZE_LINEARIZE;
}

static void linearize_if_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    bool has_else = syn->ifstmt_else != NULL;

    unsigned long long else_label_no = has_else ? NEXT_LABEL : 0;
    unsigned long long end_label_no = NEXT_LABEL;

    COPY_CODE(syn->ifstmt_condition);
    ADD_SEQUENCE_POINT;

    air_insn_t* jz = air_insn_init(AIR_JZ, 2);
    jz->ct = type_copy(syn->ifstmt_condition->ctype);
    jz->ops[0] = air_insn_label_operand_init(has_else ? else_label_no : end_label_no, 'S');
    jz->ops[1] = air_insn_register_operand_init(syn->ifstmt_condition->expr_reg);
    ADD_CODE(jz);

    COPY_CODE(syn->ifstmt_body);

    if (has_else)
    {
        air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
        jmp->ops[0] = air_insn_label_operand_init(end_label_no, 'S');
        ADD_CODE(jmp);

        air_insn_t* else_label = air_insn_init(AIR_LABEL, 1);
        else_label->ops[0] = air_insn_label_operand_init(else_label_no, 'S');
        ADD_CODE(else_label);

        COPY_CODE(syn->ifstmt_else);
    }

    air_insn_t* end_label = air_insn_init(AIR_LABEL, 1);
    end_label->ops[0] = air_insn_label_operand_init(end_label_no, 'S');
    ADD_CODE(end_label);

    FINALIZE_LINEARIZE;
}

static void linearize_switch_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    COPY_CODE(syn->swstmt_condition);
    ADD_SEQUENCE_POINT;

    c_type_t* pt = integer_promotions(syn->swstmt_condition->ctype);

    regid_t reg = convert(trav, syn->swstmt_condition->ctype, pt, syn->swstmt_condition->expr_reg, &code);

    VECTOR_FOR(syntax_component_t*, cstmt, syn->swstmt_cases)
    {
        regid_t cvreg = NEXT_VIRTUAL_REGISTER;
        air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
        ld->ct = type_copy(pt);
        ld->ops[0] = air_insn_register_operand_init(cvreg);
        ld->ops[1] = air_insn_integer_constant_operand_init(cstmt->lstmt_value);
        ADD_CODE(ld);

        regid_t eqreg = NEXT_VIRTUAL_REGISTER;
        air_insn_t* eq = air_insn_init(AIR_EQUAL, 3);
        eq->ct = make_basic_type(CTC_INT);
        eq->ops[0] = air_insn_register_operand_init(eqreg);
        eq->ops[1] = air_insn_register_operand_init(reg);
        eq->ops[2] = air_insn_register_operand_init(cvreg);
        ADD_CODE(eq);

        air_insn_t* jnz = air_insn_init(AIR_JNZ, 2);
        jnz->ct = make_basic_type(CTC_INT);
        jnz->ops[0] = air_insn_label_operand_init(cstmt->lstmt_uid, 'L');
        jnz->ops[1] = air_insn_register_operand_init(eqreg);
        ADD_CODE(jnz);
    }

    uint64_t after_label_no = syn->break_label_no ? syn->break_label_no : 0;

    if (syn->swstmt_default)
    {
        air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
        jmp->ops[0] = air_insn_label_operand_init(syn->swstmt_default->lstmt_uid, 'L');
        ADD_CODE(jmp);
    }
    else
    {
        after_label_no = syn->break_label_no ? syn->break_label_no : NEXT_LABEL;
        air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
        jmp->ops[0] = air_insn_label_operand_init(after_label_no, 'S');
        ADD_CODE(jmp);
    }

    type_delete(pt);

    COPY_CODE(syn->swstmt_body);

    if (after_label_no)
    {
        air_insn_t* label = air_insn_init(AIR_LABEL, 1);
        label->ops[0] = air_insn_label_operand_init(after_label_no, 'S');
        ADD_CODE(label);
    }

    FINALIZE_LINEARIZE;
}

/*

for
    init
    jmp .L2
.L1:
    body
    post
.L2:
    condition
    jnz .L1

while

    jmp .L2
.L1:
    body
.L2:
    condition
    jnz .L1

do while

.L1:
    body
    condition
    jnz .L1

*/

static void linearize_for_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    unsigned long long body_label_no = NEXT_LABEL;
    unsigned long long condition_label_no = syn->forstmt_condition ? NEXT_LABEL : 0;

    COPY_CODE(syn->forstmt_init);
    ADD_SEQUENCE_POINT;

    if (syn->forstmt_condition)
    {
        air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
        jmp->ops[0] = air_insn_label_operand_init(condition_label_no, 'S');
        ADD_CODE(jmp);
    }

    air_insn_t* body_label = air_insn_init(AIR_LABEL, 1);
    body_label->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
    ADD_CODE(body_label);

    COPY_CODE(syn->forstmt_body);

    if (syn->continue_label_no)
    {
        air_insn_t* continue_label = air_insn_init(AIR_LABEL, 1);
        continue_label->ops[0] = air_insn_label_operand_init(syn->continue_label_no, 'S');
        ADD_CODE(continue_label);
    }

    COPY_CODE(syn->forstmt_post);
    ADD_SEQUENCE_POINT;

    if (syn->forstmt_condition)
    {
        air_insn_t* condition_label = air_insn_init(AIR_LABEL, 1);
        condition_label->ops[0] = air_insn_label_operand_init(condition_label_no, 'S');
        ADD_CODE(condition_label);
    }

    COPY_CODE(syn->forstmt_condition);
    ADD_SEQUENCE_POINT;

    if (syn->forstmt_condition)
    {
        air_insn_t* jnz = air_insn_init(AIR_JNZ, 2);
        jnz->ct = type_copy(syn->forstmt_condition->ctype);
        jnz->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
        jnz->ops[1] = air_insn_register_operand_init(syn->forstmt_condition->expr_reg);
        ADD_CODE(jnz);
    }
    else
    {
        air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
        jmp->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
        ADD_CODE(jmp);
    }

    if (syn->break_label_no)
    {
        air_insn_t* break_label = air_insn_init(AIR_LABEL, 1);
        break_label->ops[0] = air_insn_label_operand_init(syn->break_label_no, 'S');
        ADD_CODE(break_label);
    }

    FINALIZE_LINEARIZE;
}

static void linearize_while_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    unsigned long long body_label_no = NEXT_LABEL;
    unsigned long long condition_label_no = syn->continue_label_no ? syn->continue_label_no : NEXT_LABEL;

    air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
    jmp->ops[0] = air_insn_label_operand_init(condition_label_no, 'S');
    ADD_CODE(jmp);

    air_insn_t* body_label = air_insn_init(AIR_LABEL, 1);
    body_label->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
    ADD_CODE(body_label);

    COPY_CODE(syn->whstmt_body);

    air_insn_t* condition_label = air_insn_init(AIR_LABEL, 1);
    condition_label->ops[0] = air_insn_label_operand_init(condition_label_no, 'S');
    ADD_CODE(condition_label);

    COPY_CODE(syn->whstmt_condition);
    ADD_SEQUENCE_POINT;

    air_insn_t* jnz = air_insn_init(AIR_JNZ, 2);
    jnz->ct = type_copy(syn->whstmt_condition->ctype);
    jnz->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
    jnz->ops[1] = air_insn_register_operand_init(syn->whstmt_condition->expr_reg);
    ADD_CODE(jnz);

    if (syn->break_label_no)
    {
        air_insn_t* break_label = air_insn_init(AIR_LABEL, 1);
        break_label->ops[0] = air_insn_label_operand_init(syn->break_label_no, 'S');
        ADD_CODE(break_label);
    }

    FINALIZE_LINEARIZE;
}

static void linearize_do_while_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;

    unsigned long long body_label_no = NEXT_LABEL;

    air_insn_t* body_label = air_insn_init(AIR_LABEL, 1);
    body_label->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
    ADD_CODE(body_label);

    COPY_CODE(syn->dostmt_body);

    if (syn->continue_label_no)
    {
        air_insn_t* continue_label = air_insn_init(AIR_LABEL, 1);
        continue_label->ops[0] = air_insn_label_operand_init(syn->continue_label_no, 'S');
        ADD_CODE(continue_label);
    }

    COPY_CODE(syn->dostmt_condition);
    ADD_SEQUENCE_POINT;

    air_insn_t* jnz = air_insn_init(AIR_JNZ, 2);
    jnz->ct = type_copy(syn->dostmt_condition->ctype);
    jnz->ops[0] = air_insn_label_operand_init(body_label_no, 'S');
    jnz->ops[1] = air_insn_register_operand_init(syn->dostmt_condition->expr_reg);
    ADD_CODE(jnz);

    if (syn->break_label_no)
    {
        air_insn_t* break_label = air_insn_init(AIR_LABEL, 1);
        break_label->ops[0] = air_insn_label_operand_init(syn->break_label_no, 'S');
        ADD_CODE(break_label);
    }

    FINALIZE_LINEARIZE;
}

static void linearize_va_intrinsic_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn, air_insn_type_t type)
{
    SETUP_LINEARIZE;

    if (type == AIR_VA_START)
        AIRINIZING_TRAVERSER->croutine->uses_varargs = true;

    syntax_component_t* arg_ap = vector_get(syn->icallexpr_args, 0);
    
    COPY_CODE(arg_ap);

    air_insn_t* insn = air_insn_init(type, 2);
    insn->ct = type_copy(syn->ctype);
    insn->ops[0] = air_insn_register_operand_init(syn->expr_reg = NEXT_VIRTUAL_REGISTER);
    insn->ops[1] = air_insn_register_operand_init(arg_ap->expr_reg);
    ADD_CODE(insn);

    FINALIZE_LINEARIZE;
}

static void linearize_intrinsic_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    if (streq(syn->icallexpr_name, "__ecc_va_arg"))
        linearize_va_intrinsic_call_expression_after(trav, syn, AIR_VA_ARG);
    else if (streq(syn->icallexpr_name, "__ecc_va_start"))
        linearize_va_intrinsic_call_expression_after(trav, syn, AIR_VA_START);
    else if (streq(syn->icallexpr_name, "__ecc_va_end"))
        linearize_va_intrinsic_call_expression_after(trav, syn, AIR_VA_END);
    else
        report_return;
}

static void linearize_break_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* parent = syn;
    for (; parent &&
        parent->type != SC_FOR_STATEMENT &&
        parent->type != SC_WHILE_STATEMENT &&
        parent->type != SC_DO_STATEMENT &&
        parent->type != SC_SWITCH_STATEMENT; parent = parent->parent);
    if (!parent) report_return;
    if (!parent->break_label_no)
        parent->break_label_no = NEXT_LABEL;
    
    SETUP_LINEARIZE;
    air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
    jmp->ops[0] = air_insn_label_operand_init(parent->break_label_no, 'S');
    ADD_CODE(jmp);
    FINALIZE_LINEARIZE;
}

static void linearize_continue_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    syntax_component_t* loop = syn;
    for (; loop &&
        loop->type != SC_FOR_STATEMENT &&
        loop->type != SC_WHILE_STATEMENT &&
        loop->type != SC_DO_STATEMENT;
        loop = loop->parent);
    if (!loop) report_return;
    if (!loop->continue_label_no)
        loop->continue_label_no = NEXT_LABEL;

    SETUP_LINEARIZE;
    air_insn_t* jmp = air_insn_init(AIR_JMP, 1);
    jmp->ops[0] = air_insn_label_operand_init(loop->continue_label_no, 'S');
    ADD_CODE(jmp);
    FINALIZE_LINEARIZE;
}

static void linearize_no_action_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    // does nothing. for syntax elements which need not be linearized
    // (assuredly, that's why the warning below is set up)
}

static void linearize_default_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    warnf("no linearization procedure built for syntax elements of type %s\n", SYNTAX_COMPONENT_NAMES[syn->type]);
}

air_t* airinize(syntax_component_t* tlu)
{
    if (!tlu) return NULL;
    syntax_traverser_t* trav = traverse_init(tlu, sizeof(airinizing_syntax_traverser_t));
    air_t* air = AIRINIZING_TRAVERSER->air = calloc(1, sizeof(air_t));
    air->next_available_temporary = NO_PHYSICAL_REGISTERS + 1;
    AIRINIZING_TRAVERSER->next_label = 1;
    air->data = vector_init();
    air->rodata = vector_init();
    air->routines = vector_init();
    air->st = tlu->tlu_st;

    trav->after[SC_DECLARATOR_IDENTIFIER] = linearize_declarator_identifier_after;
    trav->after[SC_FUNCTION_DECLARATOR] = linearize_function_declarator_after;
    trav->after[SC_INIT_DECLARATOR] = linearize_init_declarator_after;
    trav->after[SC_ARRAY_DECLARATOR] = linearize_array_declarator_after;
    trav->after[SC_DECLARATOR] = linearize_declarator_after;
    trav->after[SC_STRUCT_DECLARATOR] = linearize_no_action_after;
    trav->after[SC_ABSTRACT_DECLARATOR] = linearize_no_action_after;

    trav->before[SC_FUNCTION_DEFINITION] = linearize_function_definition_before;
    trav->after[SC_FUNCTION_DEFINITION] = linearize_function_definition_after;
    trav->after[SC_PRIMARY_EXPRESSION_IDENTIFIER] = linearize_primary_expression_identifier_after;
    trav->after[SC_PRIMARY_EXPRESSION_ENUMERATION_CONSTANT] = linearize_primary_expression_enumeration_constant_after;
    trav->after[SC_INTEGER_CONSTANT] = linearize_integer_constant_after;
    trav->after[SC_CHARACTER_CONSTANT] = linearize_character_constant_after;
    trav->after[SC_FLOATING_CONSTANT] = linearize_floating_constant_after;
    trav->after[SC_ADDITION_EXPRESSION] = linearize_addition_expression_after;
    trav->after[SC_SUBTRACTION_EXPRESSION] = linearize_subtraction_expression_after;
    trav->after[SC_EXPRESSION] = linearize_expression_after;
    trav->after[SC_EXPRESSION_STATEMENT] = linearize_expression_statement_after;
    trav->after[SC_COMPOUND_STATEMENT] = linearize_compound_statement_after;
    trav->after[SC_RETURN_STATEMENT] = linearize_return_statement_after;
    trav->after[SC_DECLARATION] = linearize_declaration_after;
    trav->after[SC_INITIALIZER_LIST] = linearize_initializer_list_after;
    trav->after[SC_SUBSCRIPT_EXPRESSION] = linearize_subscript_expression_after;
    trav->after[SC_STRING_LITERAL] = linearize_string_literal_after;
    trav->after[SC_FUNCTION_CALL_EXPRESSION] = linearize_function_call_expression_after;
    trav->after[SC_MEMBER_EXPRESSION] = linearize_member_expression_after;
    trav->after[SC_DEREFERENCE_MEMBER_EXPRESSION] = linearize_member_expression_after;
    trav->after[SC_PREFIX_INCREMENT_EXPRESSION] = linearize_increment_decrement_expression_after;
    trav->after[SC_POSTFIX_INCREMENT_EXPRESSION] = linearize_increment_decrement_expression_after;
    trav->after[SC_PREFIX_DECREMENT_EXPRESSION] = linearize_increment_decrement_expression_after;
    trav->after[SC_POSTFIX_DECREMENT_EXPRESSION] = linearize_increment_decrement_expression_after;
    trav->after[SC_COMPOUND_LITERAL] = linearize_compound_literal_after;
    trav->after[SC_TYPE_NAME] = linearize_type_name_after;
    trav->after[SC_REFERENCE_EXPRESSION] = linearize_reference_expression_after;
    trav->after[SC_DEREFERENCE_EXPRESSION] = linearize_unary_expression_after;
    trav->after[SC_PLUS_EXPRESSION] = linearize_unary_expression_after;
    trav->after[SC_MINUS_EXPRESSION] = linearize_unary_expression_after;
    trav->after[SC_COMPLEMENT_EXPRESSION] = linearize_unary_expression_after;
    trav->after[SC_NOT_EXPRESSION] = linearize_unary_expression_after;
    trav->after[SC_SIZEOF_EXPRESSION] = linearize_sizeof_expression_after;
    trav->after[SC_SIZEOF_TYPE_EXPRESSION] = linearize_sizeof_type_expression_after;
    trav->after[SC_CAST_EXPRESSION] = linearize_cast_expression_after;
    trav->after[SC_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_MULTIPLICATION_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_DIVISION_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_MODULAR_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_ADDITION_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_SUBTRACTION_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_BITWISE_LEFT_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_BITWISE_RIGHT_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_BITWISE_AND_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_BITWISE_XOR_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_BITWISE_OR_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_MULTIPLICATION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_DIVISION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_MODULAR_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_BITWISE_LEFT_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_BITWISE_RIGHT_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_LESS_EQUAL_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_LESS_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_GREATER_EQUAL_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_GREATER_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_EQUALITY_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_INEQUALITY_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_BITWISE_AND_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_BITWISE_XOR_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_BITWISE_OR_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_LOGICAL_AND_EXPRESSION] = linearize_logical_expression_after;
    trav->after[SC_LOGICAL_OR_EXPRESSION] = linearize_logical_expression_after;
    trav->after[SC_CONDITIONAL_EXPRESSION] = linearize_conditional_expression_after;
    trav->after[SC_LABELED_STATEMENT] = linearize_labeled_statement_after;
    trav->after[SC_GOTO_STATEMENT] = linearize_goto_statement_after;
    trav->after[SC_IF_STATEMENT] = linearize_if_statement_after;
    trav->after[SC_FOR_STATEMENT] = linearize_for_statement_after;
    trav->after[SC_WHILE_STATEMENT] = linearize_while_statement_after;
    trav->after[SC_DO_STATEMENT] = linearize_do_while_statement_after;
    trav->after[SC_INTRINSIC_CALL_EXPRESSION] = linearize_intrinsic_call_expression_after;
    trav->after[SC_CONTINUE_STATEMENT] = linearize_continue_statement_after;
    trav->after[SC_BREAK_STATEMENT] = linearize_break_statement_after;
    trav->after[SC_SWITCH_STATEMENT] = linearize_switch_statement_after;

    trav->after[SC_TRANSLATION_UNIT] = linearize_no_action_after;
    trav->after[SC_BASIC_TYPE_SPECIFIER] = linearize_no_action_after;
    trav->after[SC_STORAGE_CLASS_SPECIFIER] = linearize_no_action_after;
    trav->after[SC_TYPEDEF_NAME] = linearize_no_action_after;
    trav->after[SC_PARAMETER_DECLARATION] = linearize_no_action_after;
    trav->after[SC_STRUCT_UNION_SPECIFIER] = linearize_no_action_after;
    trav->after[SC_STRUCT_DECLARATION] = linearize_no_action_after;
    trav->after[SC_IDENTIFIER] = linearize_no_action_after;
    trav->after[SC_DESIGNATION] = linearize_no_action_after;
    trav->after[SC_POINTER] = linearize_no_action_after;
    trav->after[SC_TYPE_QUALIFIER] = linearize_no_action_after;
    trav->after[SC_ABSTRACT_DECLARATOR] = linearize_no_action_after;
    trav->after[SC_ABSTRACT_ARRAY_DECLARATOR] = linearize_no_action_after;
    trav->after[SC_ABSTRACT_FUNCTION_DECLARATOR] = linearize_no_action_after;
    trav->after[SC_ENUMERATOR] = linearize_no_action_after;
    trav->after[SC_ENUMERATION_CONSTANT] = linearize_no_action_after;
    trav->after[SC_ENUM_SPECIFIER] = linearize_no_action_after;

    trav->default_after = linearize_default_after;

    traverse(trav);
    traverse_delete(trav);
    return air;
}
