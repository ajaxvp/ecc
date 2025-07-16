#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "ecc.h"

#define MAX_CONSTANT_LOCAL_LABEL_LENGTH MAX_STRINGIFIED_INTEGER_LENGTH + 5

bool x86_64_is_integer_register(regid_t reg)
{
    return reg >= X86R_RAX && reg <= X86R_R15;
}

bool x86_64_is_sse_register(regid_t reg)
{
    return reg >= X86R_XMM0 && reg <= X86R_XMM7;
}

const char* register_name(regid_t reg, x86_insn_size_t size)
{
    if (x86_64_is_sse_register(reg))
        return X86_64_SSE_REGISTERS[reg - X86R_XMM0];
    switch (size)
    {
        case X86SZ_BYTE: return X86_64_BYTE_REGISTERS[reg - X86R_RAX];
        case X86SZ_WORD: return X86_64_WORD_REGISTERS[reg - X86R_RAX];
        case X86SZ_DWORD: return X86_64_DOUBLE_REGISTERS[reg - X86R_RAX];
        case X86SZ_QWORD: return X86_64_QUAD_REGISTERS[reg - X86R_RAX];
        default: return "(invalid register)";
    }
}

bool x86_operand_equals(x86_operand_t* op1, x86_operand_t* op2)
{
    if (!op1 && !op2) return true;
    if (!op1 || !op2) return false;
    #define check(condition) if (!(condition)) return false;
    check(op1->type == op2->type);
    check(op1->size == op2->size);
    switch (op1->type)
    {
        case X86OP_REGISTER:
        case X86OP_PTR_REGISTER:
            check(op1->reg == op2->reg);
            break;
        case X86OP_DEREF_REGISTER:
            check(op1->deref_reg.offset == op2->deref_reg.offset);
            check(op1->deref_reg.reg_addr == op2->deref_reg.reg_addr);
            break;
        case X86OP_ARRAY:
            check(op1->array.reg_base == op2->array.reg_base);
            check(op1->array.reg_offset == op2->array.reg_offset);
            check(op1->array.scale == op2->array.scale);
            check(op1->array.offset == op2->array.offset);
            break;
        case X86OP_LABEL:
            check(streq(op1->label, op2->label));
            break;
        case X86OP_LABEL_REF:
            check(streq(op1->label_ref.label, op2->label_ref.label));
            check(op1->label_ref.offset == op2->label_ref.offset);
            break;
        case X86OP_TEXT:
            check(streq(op1->text, op2->text));
            break;
        case X86OP_STRING:
            check(streq(op1->string, op2->string));
            break;
        case X86OP_IMMEDIATE:
            check(op1->immediate == op2->immediate);
            break;
    }
    return true;
    #undef check
}

void x86_operand_delete(x86_operand_t* op)
{
    if (!op) return;
    switch (op->type)
    {
        case X86OP_LABEL:
            free(op->label);
            break;
        case X86OP_LABEL_REF:
            free(op->label_ref.label);
            break;
        case X86OP_TEXT:
            free(op->text);
            break;
        case X86OP_STRING:
            free(op->string);
            break;
        default:
            break;
    }
    free(op);
}

void x86_insn_delete(x86_insn_t* insn)
{
    if (!insn) return;
    x86_operand_delete(insn->op1);
    x86_operand_delete(insn->op2);
    x86_operand_delete(insn->op3);
    free(insn);
}

void x86_insn_delete_all(x86_insn_t* insns)
{
    if (!insns) return;
    x86_insn_delete_all(insns->next);
    x86_insn_delete(insns);
}

void x86_asm_data_delete(x86_asm_data_t* data)
{
    if (!data) return;
    free(data->data);
    free(data->label);
    free(data);
}

void x86_asm_routine_delete(x86_asm_routine_t* routine)
{
    if (!routine) return;
    free(routine->label);
    x86_insn_delete_all(routine->insns);
    free(routine);
}

void x86_asm_file_delete(x86_asm_file_t* file)
{
    if (!file) return;
    vector_deep_delete(file->data, (deleter_t) x86_asm_data_delete);
    vector_deep_delete(file->rodata, (deleter_t) x86_asm_data_delete);
    vector_deep_delete(file->routines, (deleter_t) x86_asm_routine_delete);
    free(file);
}

static char* x86_asm_file_create_next_label(x86_asm_file_t* file)
{
    if (!file) return NULL;
    // .LGEN(num)\0
    size_t length = 5 + 1 + MAX_STRINGIFIED_INTEGER_LENGTH;
    char* str = malloc(length);
    snprintf(str, length, ".LGEN%lu", ++(file->next_constant_local_label));
    return str;
}

bool x86_64_c_type_registers_compatible(c_type_t* t1, c_type_t* t2)
{
    if (!t1 && !t2) return false;
    if (!t1 || !t2) return false;
    bool t1_intreg = type_is_integer(t1) ||
        t1->class == CTC_POINTER;
    bool t2_intreg = type_is_integer(t2) ||
        t2->class == CTC_POINTER;
    if (t1_intreg && t2_intreg)
        return true;
    if (type_is_real_floating(t1) && type_is_real_floating(t2) &&
        t1->class != CTC_LONG_DOUBLE && t2->class != CTC_LONG_DOUBLE)
        return true;
    if (t1->class == CTC_LONG_DOUBLE && t2->class == CTC_LONG_DOUBLE)
        return true;
    // TODO
    return false;
}

// location next
// location insn next
void insert_x86_insn_after(x86_insn_t* insn, x86_insn_t* location)
{
    if (!insn || !location) return;
    insn->next = location->next;
    location->next = insn;
}

char x86_operand_size_character(x86_insn_size_t size)
{
    switch (size)
    {
        case X86SZ_BYTE: return 'b';
        case X86SZ_WORD: return 'w';
        case X86SZ_DWORD: return 'l';
        case X86SZ_QWORD: return 'q';
        default: return '?';
    }
}

bool x86_insn_has_sse_operands(x86_insn_t* insn)
{
    if (!insn) return false;
    switch (insn->type)
    {
        case X86I_UNKNOWN:
        case X86I_NO_ELEMENTS:
        case X86I_LABEL:
        case X86I_LEA:
        case X86I_CALL:
        case X86I_PUSH:
        case X86I_POP:
        case X86I_LEAVE:
        case X86I_RET:
        case X86I_JMP:
        case X86I_JE:
        case X86I_JNE:
        case X86I_CMP:
        case X86I_SETE:
        case X86I_SETNE:
        case X86I_SETLE:
        case X86I_SETL:
        case X86I_SETGE:
        case X86I_SETG:
        case X86I_AND:
        case X86I_OR:
        case X86I_XOR:
        case X86I_NOT:
        case X86I_NOP:
        case X86I_NEG:
        case X86I_MOV:
        case X86I_MOVZX:
        case X86I_MOVSX:
        case X86I_ADD:
        case X86I_SUB:
        case X86I_MUL:
        case X86I_IMUL:
        case X86I_DIV:
        case X86I_IDIV:
        case X86I_SHL:
        case X86I_SHR:
        case X86I_SAR:
        case X86I_SKIP:
        case X86I_SETA:
        case X86I_SETNB:
        case X86I_SETP:
        case X86I_SETNP:
            return false;
        case X86I_MOVSS:
        case X86I_MOVSD:
        case X86I_ADDSS:
        case X86I_ADDSD:
        case X86I_SUBSS:
        case X86I_SUBSD:
        case X86I_MULSS:
        case X86I_MULSD:
        case X86I_DIVSS:
        case X86I_DIVSD:
        case X86I_CVTSD2SS:
        case X86I_CVTSS2SD:
        case X86I_COMISS:
        case X86I_COMISD:
        case X86I_XORPD:
        case X86I_XORPS:
        case X86I_UCOMISS:
        case X86I_UCOMISD:
            return true;
    }
    return false;
}

#define X86_INSN_WRITES_OP1 (uint8_t) 0x01
#define X86_INSN_WRITES_OP2 (uint8_t) 0x02
#define X86_INSN_WRITES_OP3 (uint8_t) 0x04

uint8_t x86_insn_writes(x86_insn_t* insn)
{
    if (!insn)
        return 0;
    switch (insn->type)
    {
        case X86I_UNKNOWN:
        case X86I_NO_ELEMENTS:
        case X86I_LABEL:
        case X86I_CALL:
        case X86I_PUSH:
        case X86I_LEAVE:
        case X86I_RET:
        case X86I_JMP:
        case X86I_JE:
        case X86I_JNE:
        case X86I_CMP:
        case X86I_COMISS:
        case X86I_COMISD:
        case X86I_UCOMISS:
        case X86I_UCOMISD:
        case X86I_NOP:
        case X86I_SKIP:
            return 0;
        case X86I_POP:
        case X86I_SETE:
        case X86I_SETNE:
        case X86I_SETLE:
        case X86I_SETL:
        case X86I_SETGE:
        case X86I_SETG:
        case X86I_SETA:
        case X86I_SETNB:
        case X86I_SETP:
        case X86I_SETNP:
        case X86I_NOT:
        case X86I_NEG:
        case X86I_MUL:
            return X86_INSN_WRITES_OP1;
        case X86I_LEA:
        case X86I_AND:
        case X86I_OR:
        case X86I_XOR:
        case X86I_MOV:
        case X86I_MOVZX:
        case X86I_MOVSX:
        case X86I_ADD:
        case X86I_SUB:
        case X86I_IMUL:
        case X86I_DIV:
        case X86I_IDIV:
        case X86I_SHL:
        case X86I_SHR:
        case X86I_SAR:
        case X86I_MOVSS:
        case X86I_MOVSD:
        case X86I_ADDSS:
        case X86I_ADDSD:
        case X86I_SUBSS:
        case X86I_SUBSD:
        case X86I_MULSS:
        case X86I_MULSD:
        case X86I_DIVSS:
        case X86I_DIVSD:
        case X86I_XORPS:
        case X86I_XORPD:
        case X86I_CVTSD2SS:
        case X86I_CVTSS2SD:
            return X86_INSN_WRITES_OP2;
    }
    return 0;
}

x86_insn_size_t c_type_to_x86_operand_size(c_type_t* ct)
{
    long long size = type_size(ct);
    switch (size)
    {
        case 1: return X86SZ_BYTE;
        case 2: return X86SZ_WORD;
        case 4: return X86SZ_DWORD;
        case 8: return X86SZ_QWORD;
        default: return X86SZ_QWORD;
    }
}

void x86_write_register(regid_t reg, x86_insn_size_t size, FILE* file)
{
    fprintf(file, "%%%s", register_name(reg, size));
}

void x86_write_operand(x86_operand_t* op, x86_insn_size_t size, FILE* file)
{
    if (!op) return;
    switch (op->type)
    {
        case X86OP_REGISTER:
            x86_write_register(op->reg, op->size ? op->size : size, file);
            break;
        case X86OP_PTR_REGISTER:
            fprintf(file, "*");
            x86_write_register(op->reg, op->size ? op->size : size, file);
            break;
        case X86OP_DEREF_REGISTER:
            if (op->deref_reg.offset != 0)
                fprintf(file, "%lld", op->deref_reg.offset);
            fprintf(file, "(");
            x86_write_register(op->deref_reg.reg_addr, X86SZ_QWORD, file);
            fprintf(file, ")");
            break;
        case X86OP_ARRAY:
            if (op->array.offset != 0)
                fprintf(file, "%lld", op->array.offset);
            fprintf(file, "(");
            if (op->array.reg_base != INVALID_VREGID)
                x86_write_register(op->array.reg_base, X86SZ_QWORD, file);
            fprintf(file, ", ");
            if (op->array.reg_offset != INVALID_VREGID)
                x86_write_register(op->array.reg_offset, X86SZ_QWORD, file);
            fprintf(file, ", %lld)", op->array.scale);
            break;
        case X86OP_LABEL:
            fprintf(file, "%s", op->label);
            break;
        case X86OP_TEXT:
            fprintf(file, "%s", op->text);
            break;
        case X86OP_STRING:
            fprintf(file, "\"%s\"", op->string);
            break;
        case X86OP_LABEL_REF:
            if (op->label_ref.offset > 0)
                fprintf(file, "%s+%lld(%%rip)", op->label_ref.label, op->label_ref.offset);
            else if (op->label_ref.offset < 0)
                fprintf(file, "%s-%lld(%%rip)", op->label_ref.label, llabs(op->label_ref.offset));
            else
                fprintf(file, "%s(%%rip)", op->label_ref.label);
            break;
        case X86OP_IMMEDIATE:
            fprintf(file, "$%llu", op->immediate);
            break;
        default:
            break;
    }
}

void x86_write_insn(x86_insn_t* insn, FILE* file)
{
    if (!insn) return;
    #define INDENT "    "
    char suffix[32];
    if (x86_insn_has_sse_operands(insn) ||
        insn->type == X86I_MOVSX ||
        insn->type == X86I_MOVZX)
        suffix[0] = '\0';
    else
        snprintf(suffix, 32, "%c", x86_operand_size_character(insn->size));
    #define USUAL_START(name) fprintf(file, INDENT name "%s ", suffix)
    #define USUAL_1OP(name) USUAL_START(name); goto op1;
    #define USUAL_2OP(name) USUAL_START(name); goto op2;
    switch (insn->type)
    {
        case X86I_LABEL:
            fprintf(file, "%s:", insn->op1->label);
            break;
        
        case X86I_LEAVE:
            fprintf(file, INDENT "leave");
            break;

        case X86I_RET:
            fprintf(file, INDENT "ret");
            break;

        case X86I_NOP:
            fprintf(file, INDENT "nop");
            break;
        
        case X86I_CALL:
            fprintf(file, INDENT "call ");
            x86_write_operand(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_JMP:
            fprintf(file, INDENT "jmp ");
            x86_write_operand(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_JE:
            fprintf(file, INDENT "je ");
            x86_write_operand(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_JNE:
            fprintf(file, INDENT "jne ");
            x86_write_operand(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_SETE:
            fprintf(file, INDENT "sete ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETNE:
            fprintf(file, INDENT "setne ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETLE:
            fprintf(file, INDENT "setle ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETL:
            fprintf(file, INDENT "setl ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETGE:
            fprintf(file, INDENT "setge ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETG:
            fprintf(file, INDENT "setg ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;
        
        case X86I_SETA:
            fprintf(file, INDENT "seta ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETNB:
            fprintf(file, INDENT "setnb ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETP:
            fprintf(file, INDENT "setp ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETNP:
            fprintf(file, INDENT "setnp ");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            break;
        
        case X86I_PUSH: USUAL_1OP("push")
        case X86I_NEG: USUAL_1OP("neg")

        case X86I_MOV: USUAL_2OP("mov")
        case X86I_MOVSS: USUAL_2OP("movss")
        case X86I_MOVSD: USUAL_2OP("movsd")
        case X86I_MOVSX: USUAL_2OP("movsx")
        case X86I_MOVZX: USUAL_2OP("movzx")
        case X86I_LEA: USUAL_2OP("lea")
        case X86I_AND: USUAL_2OP("and")
        case X86I_OR: USUAL_2OP("or")
        case X86I_CMP: USUAL_2OP("cmp")
        case X86I_NOT: USUAL_2OP("not")
    
        case X86I_ADD: USUAL_2OP("add")
        case X86I_ADDSS: USUAL_2OP("addss")
        case X86I_ADDSD: USUAL_2OP("addsd")

        case X86I_SUB: USUAL_2OP("sub")
        case X86I_SUBSS: USUAL_2OP("subss")
        case X86I_SUBSD: USUAL_2OP("subsd")

        case X86I_MUL: USUAL_1OP("mul")
        case X86I_IMUL: USUAL_2OP("imul")
        case X86I_MULSS: USUAL_2OP("mulss")
        case X86I_MULSD: USUAL_2OP("mulsd")

        case X86I_DIV: USUAL_1OP("div")
        case X86I_IDIV: USUAL_1OP("idiv")
        case X86I_DIVSS: USUAL_2OP("divss")
        case X86I_DIVSD: USUAL_2OP("divsd")

        case X86I_XOR: USUAL_2OP("xor")
        case X86I_XORPS: USUAL_2OP("xorps")
        case X86I_XORPD: USUAL_2OP("xorpd")

        case X86I_CVTSD2SS: USUAL_2OP("cvtsd2ss")
        case X86I_CVTSS2SD: USUAL_2OP("cvtss2sd")
    
        case X86I_COMISS: USUAL_2OP("comiss")
        case X86I_COMISD: USUAL_2OP("comisd")

        case X86I_UCOMISS: USUAL_2OP("ucomiss")
        case X86I_UCOMISD: USUAL_2OP("ucomisd")

        case X86I_SHL:
            USUAL_START("shl");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            fprintf(file, ", ");
            x86_write_operand(insn->op2, insn->size, file);
            break;
        
        case X86I_SHR:
            USUAL_START("shr");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            fprintf(file, ", ");
            x86_write_operand(insn->op2, insn->size, file);
            break;

        case X86I_SAR:
            USUAL_START("sar");
            x86_write_operand(insn->op1, X86SZ_BYTE, file);
            fprintf(file, ", ");
            x86_write_operand(insn->op2, insn->size, file);
            break;

        op1:
            x86_write_operand(insn->op1, insn->size, file);
            break;
        
        op2:
            x86_write_operand(insn->op1, insn->size, file);
            fprintf(file, ", ");
            x86_write_operand(insn->op2, insn->size, file);
            break;

        case X86I_SKIP: return;

        default:
            break;
    }
    #undef INDENT
    #undef USUAL_START
    #undef USUAL_1OP
    #undef USUAL_2OP
    fprintf(file, "\n");
}

void x86_write_data(x86_asm_data_t* data, FILE* out)
{
    fprintf(out, "    .align %lu\n", data->alignment);
    fprintf(out, "%s:\n", data->label);
    for (size_t i = 0; i < data->length;)
    {
        if (i + UNSIGNED_LONG_LONG_INT_WIDTH <= data->length)
            fprintf(out, "    .quad 0x%llX\n", *((unsigned long long*) (data->data + i))), i += UNSIGNED_LONG_LONG_INT_WIDTH;
        else if (i + UNSIGNED_INT_WIDTH <= data->length)
            fprintf(out, "    .long 0x%X\n", *((unsigned*) (data->data + i))), i += UNSIGNED_INT_WIDTH;
        else if (i + UNSIGNED_SHORT_INT_WIDTH <= data->length)
            fprintf(out, "    .word 0x%X\n", *((unsigned short*) (data->data + i))), i += UNSIGNED_SHORT_INT_WIDTH;
        else
            fprintf(out, "    .byte 0x%X\n", *((unsigned char*) (data->data + i))), i += UNSIGNED_CHAR_WIDTH;
    }
}

static void x86_write_varargs_setup(x86_asm_routine_t* routine, FILE* out)
{
    fprintf(out, "    movq %%r9, -8(%%rbp)\n");
    fprintf(out, "    movq %%r8, -16(%%rbp)\n");
    fprintf(out, "    movq %%rcx, -16(%%rbp)\n");
    fprintf(out, "    movq %%rdx, -24(%%rbp)\n");
    fprintf(out, "    movq %%rdx, -32(%%rbp)\n");
    fprintf(out, "    movq %%rsi, -40(%%rbp)\n");
    fprintf(out, "    movq %%rdi, -48(%%rbp)\n");
    fprintf(out, "    movaps %%xmm7, -64(%%rbp)\n");
    fprintf(out, "    movaps %%xmm6, -80(%%rbp)\n");
    fprintf(out, "    movaps %%xmm5, -96(%%rbp)\n");
    fprintf(out, "    movaps %%xmm4, -112(%%rbp)\n");
    fprintf(out, "    movaps %%xmm3, -128(%%rbp)\n");
    fprintf(out, "    movaps %%xmm2, -144(%%rbp)\n");
    fprintf(out, "    movaps %%xmm1, -160(%%rbp)\n");
    fprintf(out, "    movaps %%xmm0, -176(%%rbp)\n");
}

static void x86_find_used_nonvolatiles(x86_asm_routine_t* routine)
{
    for (x86_insn_t* insn = routine->insns; insn; insn = insn->next)
    {
        uint8_t writes = x86_insn_writes(insn);
        x86_operand_t* ops[] = { insn->op1, insn->op2, insn->op3 };
        static uint8_t write_states[] = { X86_INSN_WRITES_OP1, X86_INSN_WRITES_OP2, X86_INSN_WRITES_OP3 };
        for (size_t i = 0; i < 3; ++i)
        {
            if (!ops[i]) continue;
            if ((writes & write_states[i]) && ops[i]->type == X86OP_REGISTER)
            {
                switch (insn->op1->reg)
                {
                    case X86R_RBX: routine->used_nonvolatiles |= USED_NONVOLATILES_RBX; break;
                    case X86R_R12: routine->used_nonvolatiles |= USED_NONVOLATILES_R12; break;
                    case X86R_R13: routine->used_nonvolatiles |= USED_NONVOLATILES_R13; break;
                    case X86R_R14: routine->used_nonvolatiles |= USED_NONVOLATILES_R14; break;
                    case X86R_R15: routine->used_nonvolatiles |= USED_NONVOLATILES_R15; break;
                    default: break;
                }
            }
        }
    }
}

static uint16_t NONVOLATILE_FLAGS[] = {
    USED_NONVOLATILES_RBX,
    USED_NONVOLATILES_R12,
    USED_NONVOLATILES_R13,
    USED_NONVOLATILES_R14,
    USED_NONVOLATILES_R15
};

static const char* NONVOLATILE_REGISTER_NAMES[] = {
    "rbx",
    "r12",
    "r13",
    "r14",
    "r15"
};

static void x86_write_routine_push_nonvolatiles(x86_asm_routine_t* routine, FILE* out)
{
    for (int i = 0; i < sizeof(NONVOLATILE_FLAGS) / sizeof(NONVOLATILE_FLAGS[0]); ++i)
    {
        if (routine->used_nonvolatiles & NONVOLATILE_FLAGS[i])
            fprintf(out, "    pushq %%%s\n", NONVOLATILE_REGISTER_NAMES[i]);
    }
}

static void x86_write_routine_pop_nonvolatiles(x86_asm_routine_t* routine, FILE* out)
{
    for (int i = sizeof(NONVOLATILE_FLAGS) / sizeof(NONVOLATILE_FLAGS[0]) - 1; i >= 0; --i)
    {
        if (routine->used_nonvolatiles & NONVOLATILE_FLAGS[i])
            fprintf(out, "    popq %%%s\n", NONVOLATILE_REGISTER_NAMES[i]);
    }
}

void x86_write_routine(x86_asm_routine_t* routine, FILE* out)
{
    x86_find_used_nonvolatiles(routine);
    if (routine->global)
        fprintf(out, "    .globl %s\n", routine->label);
    fprintf(out, "%s:\n", routine->label);
    fprintf(out, "    pushq %%rbp\n");
    fprintf(out, "    movq %%rsp, %%rbp\n");
    if (routine->stackalloc)
    {
        long long v = llabs(routine->stackalloc);
        fprintf(out, "    subq $%lld, %%rsp\n", v + (16 - (v % 16)) % 16);
    }
    x86_write_routine_push_nonvolatiles(routine, out);
    if (routine->uses_varargs)
        x86_write_varargs_setup(routine, out);
    size_t lr_jumps = 0;
    for (x86_insn_t* insn = routine->insns; insn; insn = insn->next)
    {
        if (insn->type == X86I_JMP && insn->op1->type == X86OP_LABEL && starts_with_ignore_case(insn->op1->label, ".LR"))
        {
            if (!insn->next)
                continue;
            ++lr_jumps;
        }
        x86_write_insn(insn, out);
    }
    if (lr_jumps > 0)
    {
        char buffer[6 + MAX_STRINGIFIED_INTEGER_LENGTH];
        snprintf(buffer, sizeof(buffer), ".LR%lu:\n", routine->id);
        fprintf(out, "%s", buffer);
    }
    x86_write_routine_pop_nonvolatiles(routine, out);
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
}

void x86_asm_file_write(x86_asm_file_t* file, FILE* out)
{
    if (file->data->size)
        fprintf(out, "    .data\n");
    VECTOR_FOR(x86_asm_data_t*, data, file->data)
        x86_write_data(data, out);
    if (file->rodata->size)
        fprintf(out, "    .section .rodata\n");
    VECTOR_FOR(x86_asm_data_t*, rodata, file->rodata)
        x86_write_data(rodata, out);
    if (file->routines->size)
        fprintf(out, "    .text\n");
    VECTOR_FOR(x86_asm_routine_t*, routine, file->routines)
        x86_write_routine(routine, out);
}

x86_operand_t* make_basic_x86_operand(x86_operand_type_t type)
{
    x86_operand_t* op = calloc(1, sizeof *op);
    op->type = type;
    return op;
}

x86_operand_t* make_operand_label(char* label)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_LABEL);
    op->label = strdup(label);
    return op;
}

x86_operand_t* make_operand_label_ref(char* label, long long offset)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_LABEL_REF);
    op->label_ref.label = strdup(label);
    op->label_ref.offset = offset;
    return op;
}

x86_operand_t* make_operand_string(char* string)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_STRING);
    op->string = strdup(string);
    return op;
}

x86_operand_t* make_operand_register(regid_t reg)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_REGISTER);
    op->reg = reg;
    return op;
}

x86_operand_t* make_operand_ptr_register(regid_t reg)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_PTR_REGISTER);
    op->reg = reg;
    return op;
}

x86_operand_t* make_operand_deref_register(regid_t reg, long long offset)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_DEREF_REGISTER);
    op->deref_reg.offset = offset;
    op->deref_reg.reg_addr = reg;
    return op;
}

x86_operand_t* make_operand_immediate(unsigned long long immediate)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_IMMEDIATE);
    op->immediate = immediate;
    return op;
}

x86_operand_t* make_operand_array(regid_t reg_base, regid_t reg_offset, long long scale, long long offset)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_ARRAY);
    op->array.reg_base = reg_base;
    op->array.reg_offset = reg_offset;
    op->array.scale = scale;
    op->array.offset = offset;
    return op;
}

x86_operand_t* air_operand_to_x86_operand(air_insn_operand_t* aop, x86_asm_routine_t* routine)
{
    switch (aop->type)
    {
        case AOP_INDIRECT_REGISTER:
        {
            if (aop->content.inreg.roffset != INVALID_VREGID || aop->content.inreg.factor != 1)
                return make_operand_array(aop->content.inreg.id, aop->content.inreg.roffset, aop->content.inreg.factor, aop->content.inreg.offset);
            return make_operand_deref_register(aop->content.inreg.id, aop->content.inreg.offset);
        }
        case AOP_REGISTER:
            return make_operand_register(aop->content.reg);
        case AOP_INTEGER_CONSTANT:
            return make_operand_immediate(aop->content.ic);
        case AOP_SYMBOL:
        case AOP_INDIRECT_SYMBOL:
        {
            symbol_t* sy = aop->type == AOP_SYMBOL ? aop->content.sy : aop->content.insy.sy;
            long long offset = aop->type == AOP_INDIRECT_SYMBOL ? aop->content.insy.offset : 0;

            // if it has static storage duration, we use a label
            if (symbol_get_storage_duration(sy) == SD_STATIC)
                return make_operand_label_ref(symbol_get_name(sy), offset);
            
            // if it has a stack offset already, use that
            if (sy->stack_offset)
                return make_operand_deref_register(X86R_RBP, sy->stack_offset + offset);
            
            // otherwise, give it a stack offset
            long long syoffset = routine->stackalloc;
            long long size = type_size(sy->type);
            long long alignment = type_alignment(sy->type);
            syoffset -= size;
            syoffset -= abs(syoffset % alignment);
            return make_operand_deref_register(X86R_RBP, (routine->stackalloc = sy->stack_offset = syoffset) + offset);
        }
        case AOP_LABEL:
            char label[MAX_CONSTANT_LOCAL_LABEL_LENGTH];
            snprintf(label, MAX_CONSTANT_LOCAL_LABEL_LENGTH, ".L%c%llu", aop->content.label.disambiguator, aop->content.label.id);
            return make_operand_label(label);
        case AOP_FLOATING_CONSTANT:
        case AOP_TYPE:
            report_return_value(NULL);
    }
    report_return_value(NULL);
}

x86_insn_t* make_basic_x86_insn(x86_insn_type_t type)
{
    x86_insn_t* insn = calloc(1, sizeof *insn);
    insn->type = type;
    return insn;
}

x86_insn_t* make_x86_insn_clear_register(regid_t reg)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_XOR);
    insn->size = X86SZ_QWORD;
    insn->op1 = make_operand_register(reg);
    insn->op2 = make_operand_register(reg);
    return insn;
}

x86_insn_t* x86_generate_load(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_type_t type = X86I_UNKNOWN;
    if (type_is_integer(ainsn->ct) || ainsn->ct->class == CTC_POINTER)
        type = X86I_MOV;
    else if (ainsn->ct->class == CTC_FLOAT)
        type = X86I_MOVSS;
    else if (ainsn->ct->class == CTC_DOUBLE)
        type = X86I_MOVSD;
    else
        report_return_value(NULL);
    x86_insn_t* insn = make_basic_x86_insn(type);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    return insn;
}

x86_insn_t* x86_generate_load_addr(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_LEA);
    insn->size = X86SZ_QWORD;
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    return insn;
}

x86_insn_t* x86_generate_func_call(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_CALL);
    insn->size = X86SZ_QWORD;
    air_insn_operand_t* aop = ainsn->ops[1];
    switch (aop->type)
    {
        case AOP_REGISTER:
            insn->op1 = make_operand_ptr_register(aop->content.reg);
            break;
        case AOP_SYMBOL:
            insn->op1 = make_operand_label(symbol_get_name(aop->content.sy));
            break;
        default:
            report_return_value(NULL);
    }
    return insn;
}

x86_insn_t* x86_generate_nop(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    return make_basic_x86_insn(X86I_NOP);
}

// just letting the code generator know that the variable exists
x86_insn_t* x86_generate_declare(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_operand_delete(air_operand_to_x86_operand(ainsn->ops[0], routine));
    return NULL;
}

x86_insn_t* x86_generate_return(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* jmp = make_basic_x86_insn(X86I_JMP);
    char buffer[4 + MAX_STRINGIFIED_INTEGER_LENGTH];
    snprintf(buffer, sizeof(buffer), ".LR%lu", routine->id);
    jmp->op1 = make_operand_label(buffer);
    return jmp;
}

x86_insn_t* x86_generate_binary_operator(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_type_t type = X86I_UNKNOWN;
    x86_insn_type_t movtype = X86I_UNKNOWN;
    if (ainsn->ct->class == CTC_FLOAT)
    {
        movtype = X86I_MOVSS;
        switch (ainsn->type)
        {
            case AIR_ADD: type = X86I_ADDSS; break;
            case AIR_SUBTRACT: type = X86I_SUBSS; break;
            case AIR_MULTIPLY: type = X86I_MULSS; break;
            case AIR_XOR: type = X86I_XORPS; break;
            default: report_return_value(NULL);
        }
    }
    else if (ainsn->ct->class == CTC_DOUBLE)
    {
        movtype = X86I_MOVSD;
        switch (ainsn->type)
        {
            case AIR_ADD: type = X86I_ADDSD; break;
            case AIR_SUBTRACT: type = X86I_SUBSD; break;
            case AIR_MULTIPLY: type = X86I_MULSD; break;
            case AIR_XOR: type = X86I_XORPD; break;
            default: report_return_value(NULL);
        }
    }
    else if (type_is_signed_integer(ainsn->ct) || ainsn->ct->class == CTC_CHAR || ainsn->ct->class == CTC_POINTER)
    {
        movtype = X86I_MOV;
        switch (ainsn->type)
        {
            case AIR_ADD: type = X86I_ADD; break;
            case AIR_SUBTRACT: type = X86I_SUB; break;
            case AIR_MULTIPLY: type = X86I_IMUL; break;
            case AIR_AND: type = X86I_AND; break;
            case AIR_XOR: type = X86I_XOR; break;
            case AIR_OR: type = X86I_OR; break;
            case AIR_SHIFT_LEFT: type = X86I_SHL; break;
            case AIR_SHIFT_RIGHT: type = X86I_SHR; break;
            case AIR_SIGNED_SHIFT_RIGHT: type = X86I_SAR; break;
            default: report_return_value(NULL);
        }
    }
    else
        // TODO: handle unsigned integers
        report_return_value(NULL);
    
    x86_insn_t* insn = make_basic_x86_insn(type);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[2], routine);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);

    x86_insn_t* mov = make_basic_x86_insn(movtype);
    mov->size = c_type_to_x86_operand_size(ainsn->ct);
    mov->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    mov->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    if (x86_operand_equals(mov->op1, mov->op2))
        x86_insn_delete(mov);
    else
        insn->next = mov;
    return insn;
}

x86_insn_t* x86_generate_direct_binary_operator(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_type_t type = X86I_UNKNOWN;
    if (ainsn->ct->class == CTC_FLOAT)
    {
        switch (ainsn->type)
        {
            case AIR_DIRECT_ADD: type = X86I_ADDSS; break;
            case AIR_DIRECT_SUBTRACT: type = X86I_SUBSS; break;
            case AIR_DIRECT_MULTIPLY: type = X86I_MULSS; break;
            default: report_return_value(NULL);
        }
    }
    else if (ainsn->ct->class == CTC_DOUBLE)
    {
        switch (ainsn->type)
        {
            case AIR_DIRECT_ADD: type = X86I_ADDSD; break;
            case AIR_DIRECT_SUBTRACT: type = X86I_SUBSD; break;
            case AIR_DIRECT_MULTIPLY: type = X86I_MULSD; break;
            default: report_return_value(NULL);
        }
    }
    else if (type_is_signed_integer(ainsn->ct))
    {
        switch (ainsn->type)
        {
            case AIR_DIRECT_ADD: type = X86I_ADD; break;
            case AIR_DIRECT_SUBTRACT: type = X86I_SUB; break;
            case AIR_DIRECT_MULTIPLY: type = X86I_IMUL; break;
            case AIR_DIRECT_AND: type = X86I_AND; break;
            case AIR_DIRECT_XOR: type = X86I_XOR; break;
            case AIR_DIRECT_OR: type = X86I_OR; break;
            case AIR_DIRECT_SHIFT_LEFT: type = X86I_SHL; break;
            case AIR_DIRECT_SHIFT_RIGHT: type = X86I_SHR; break;
            case AIR_DIRECT_SIGNED_SHIFT_RIGHT: type = X86I_SAR; break;
            default: report_return_value(NULL);
        }
    }
    else if (type_is_unsigned_integer(ainsn->ct))
    {
        // TODO: handle unsigned integers
        switch (ainsn->type)
        {
            case AIR_DIRECT_ADD: type = X86I_ADD; break;
            case AIR_DIRECT_SUBTRACT: type = X86I_SUB; break;
            case AIR_DIRECT_AND: type = X86I_AND; break;
            default: report_return_value(NULL);
        }
    }
    else if (ainsn->ct->class == CTC_POINTER)
    {
        switch (ainsn->type)
        {
            case AIR_DIRECT_ADD: type = X86I_ADD; break;
            case AIR_DIRECT_SUBTRACT: type = X86I_SUB; break;
            default: report_return_value(NULL);
        }
    }
    
    x86_insn_t* insn = make_basic_x86_insn(type);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    return insn;
}

x86_insn_t* x86_generate_not(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    if (type_is_integer(ainsn->ct))
    {
        x86_insn_t* cmp = make_basic_x86_insn(X86I_CMP);
        cmp->size = c_type_to_x86_operand_size(ainsn->ct);
        cmp->op1 = make_operand_immediate(0);
        cmp->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);

        x86_insn_t* sete = make_basic_x86_insn(X86I_SETE);
        sete->size = X86SZ_BYTE;
        sete->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);

        cmp->next = sete;
        return cmp;
    }
    else
        // TODO: support floats and doubles
        report_return_value(NULL);
}

x86_insn_t* x86_generate_negate(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    if (type_is_integer(ainsn->ct))
    {
        x86_insn_t* insn = make_basic_x86_insn(X86I_NEG);
        insn->size = c_type_to_x86_operand_size(ainsn->ct);
        insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);

        x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
        mov->size = c_type_to_x86_operand_size(ainsn->ct);
        mov->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
        mov->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

        if (x86_operand_equals(mov->op1, mov->op2))
            x86_insn_delete(mov);
        else
            insn->next = mov;

        return insn;
    }
    else if (type_is_sse_floating(ainsn->ct))
    {
        // negations for SSE operands get removed during localization
        report_return_value(NULL);
    }
    else
        // TODO: support long doubles and complex numbers
        report_return_value(NULL);
}

x86_insn_t* x86_generate_posate(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_type_t type = X86I_UNKNOWN;
    if (ainsn->ct->class == CTC_FLOAT)
        type = X86I_MOVSS;
    else if (ainsn->ct->class == CTC_DOUBLE)
        type = X86I_MOVSD;
    else if (type_is_integer(ainsn->ct))
        type = X86I_MOV;
    else
        report_return_value(NULL);

    x86_insn_t* insn = make_basic_x86_insn(type);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    return insn;
}

x86_insn_t* x86_generate_complement(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_NOT);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);

    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(ainsn->ct);
    mov->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    mov->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    if (x86_operand_equals(mov->op1, mov->op2))
        x86_insn_delete(mov);
    else
        insn->next = mov;

    return insn;
}

x86_insn_t* x86_generate_conditional_jump(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_type_t type = X86I_UNKNOWN;
    switch (ainsn->type)
    {
        case AIR_JZ: type = X86I_JE; break;
        case AIR_JNZ: type = X86I_JNE; break;
        default: report_return_value(NULL);
    }

    x86_insn_t* cmp = NULL;
    
    if (type_is_integer(ainsn->ct))
    {
        cmp = make_basic_x86_insn(X86I_CMP);
        cmp->size = c_type_to_x86_operand_size(ainsn->ct);
        cmp->op1 = make_operand_immediate(0);
        cmp->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    }
    else
        // TODO: support floats and doubles
        report_return_value(NULL);

    x86_insn_t* jmp = make_basic_x86_insn(type);
    jmp->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    cmp->next = jmp;

    return cmp;
}

x86_insn_t* x86_generate_jmp(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_JMP);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    return insn;
}

x86_insn_t* x86_generate_label(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_LABEL);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    return insn;
}

x86_insn_t* x86_generate_push(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_PUSH);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    return insn;
}

x86_insn_t* x86_generate_relational_equality_operator(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    // this type should also be equal to ainsn->ops[2]->ct
    c_type_t* opt = ainsn->ops[1]->ct;
    if (!opt) report_return_value(NULL);

    bool opt_sse = type_is_sse_floating(opt);

    x86_insn_type_t type = X86I_UNKNOWN;
    switch (ainsn->type)
    {
        case AIR_LESS_EQUAL:
            if (opt_sse)
            {
                type = X86I_SETNB;
                break;
            }
            type = X86I_SETLE;
            break;
        case AIR_LESS: 
            if (opt_sse)
            {
                type = X86I_SETA;
                break;
            }
            type = X86I_SETL;
            break;
        case AIR_GREATER_EQUAL:
            if (opt_sse)
            {
                type = X86I_SETNB;
                break;
            }
            type = X86I_SETGE;
            break;
        case AIR_GREATER:
            if (opt_sse)
            {
                type = X86I_SETA;
                break;
            }
            type = X86I_SETG;
            break;
        case AIR_EQUAL:
            type = X86I_SETE;
            break;
        case AIR_INEQUAL:
            type = X86I_SETNE;
            break;
        default:
            report_return_value(NULL);
    }

    x86_insn_t* cmp = NULL;
    if (type_is_integer(opt))
        cmp = make_basic_x86_insn(X86I_CMP);
    else if (opt_sse)
        cmp = make_basic_x86_insn(opt->class == CTC_FLOAT ? X86I_COMISS : X86I_COMISD);
    else // TODO: long doubles and complex numbers
        report_return_value(NULL);

    cmp->size = c_type_to_x86_operand_size(ainsn->ct);

    // flip operands for SSE <= and <
    if (opt_sse && (ainsn->type == AIR_LESS_EQUAL || ainsn->type == AIR_LESS))
    {
        cmp->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
        cmp->op2 = air_operand_to_x86_operand(ainsn->ops[2], routine);
    }
    else
    {
        cmp->op1 = air_operand_to_x86_operand(ainsn->ops[2], routine);
        cmp->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    }
    
    x86_insn_t* insn = make_basic_x86_insn(type);
    insn->size = X86SZ_BYTE;
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    cmp->next = insn;

    return cmp;
}

/*

SSE equals:

	movsd	-16(%rbp), %xmm0
	ucomisd	-8(%rbp), %xmm0
	setnp	%al
	andq	$1, %rax
	movsd	-16(%rbp), %xmm0
	ucomisd	-8(%rbp), %xmm0
	je		.L1
	movl	$0, %eax
.L1:

SSE not equals:

	movsd	-16(%rbp), %xmm0
	ucomisd	-8(%rbp), %xmm0
	setp	%al
	andq	$1, %rax
	movsd	-16(%rbp), %xmm0
	ucomisd	-8(%rbp), %xmm0
	je      .L1
	movl    $1, %eax
.L1:

*/

x86_insn_t* x86_generate_sse_equality_operator(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    bool eq = ainsn->type == AIR_EQUAL;
    c_type_t* opt = ainsn->ops[1]->ct;
    bool is_float = opt->class == CTC_FLOAT;

    x86_insn_t* cmp1 = make_basic_x86_insn(is_float ? X86I_UCOMISS : X86I_UCOMISD);
    cmp1->size = c_type_to_x86_operand_size(opt);
    cmp1->op1 = air_operand_to_x86_operand(ainsn->ops[2], routine);
    cmp1->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);

    x86_insn_t* parity = make_basic_x86_insn(eq ? X86I_SETNP : X86I_SETP);
    parity->size = X86SZ_BYTE;
    parity->op1 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    cmp1->next = parity;

    x86_insn_t* cmp2 = make_basic_x86_insn(is_float ? X86I_UCOMISS : X86I_UCOMISD);
    cmp2->size = c_type_to_x86_operand_size(opt);
    cmp2->op1 = air_operand_to_x86_operand(ainsn->ops[2], routine);
    cmp2->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    parity->next = cmp2;

    x86_insn_t* je = make_basic_x86_insn(X86I_JE);
    char* label_name = x86_asm_file_create_next_label(file);
    je->op1 = make_operand_label(label_name);
    cmp2->next = je;

    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(ainsn->ct);
    mov->op1 = make_operand_immediate(!eq);
    mov->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    je->next = mov;

    x86_insn_t* label = make_basic_x86_insn(X86I_LABEL);
    label->op1 = make_operand_label(label_name);
    mov->next = label;

    free(label_name);

    return cmp1;
}

x86_insn_t* x86_generate_equality_operator(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    c_type_t* opt = ainsn->ops[1]->ct;
    if (type_is_sse_floating(opt))
        return x86_generate_sse_equality_operator(ainsn, routine, file);
    else if (type_is_integer(opt))
        return x86_generate_relational_equality_operator(ainsn, routine, file);
    else
        // TODO: support long doubles and complex numbers
        report_return_value(NULL);
}

x86_insn_t* x86_generate_extension(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(ainsn->type == AIR_SEXT ? X86I_MOVSX : X86I_MOVZX);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    insn->op1->size = c_type_to_x86_operand_size(ainsn->ops[1]->ct);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);
    return insn;
}

x86_insn_t* x86_generate_divide(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* div = NULL;
    x86_insn_type_t type = X86I_UNKNOWN;
    if (ainsn->ct->class == CTC_FLOAT || ainsn->ct->class == CTC_DOUBLE)
    {
        type = ainsn->ct->class == CTC_FLOAT ? X86I_MOVSS : X86I_MOVSD;
        div = make_basic_x86_insn(ainsn->ct->class == CTC_FLOAT ? X86I_DIVSS : X86I_DIVSD);
        div->size = c_type_to_x86_operand_size(ainsn->ct);
        div->op1 = air_operand_to_x86_operand(ainsn->ops[2], routine);
        div->op2 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    }
    else if (type_is_integer(ainsn->ct))
    {
        bool sig = type_is_signed_integer(ainsn->ct);
        type = X86I_MOV;
        div = make_basic_x86_insn(sig ? X86I_IDIV : X86I_DIV);
        div->size = c_type_to_x86_operand_size(ainsn->ct);
        div->op1 = air_operand_to_x86_operand(ainsn->ops[2], routine);
        return div;
    }
    else
        report_return_value(NULL);
    
    x86_insn_t* insn = make_basic_x86_insn(type);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    if (x86_operand_equals(insn->op1, insn->op2))
        x86_insn_delete(insn);
    else
        div->next = insn;

    return div;
}

x86_insn_t* x86_generate_s2d(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_CVTSS2SD);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    return insn;
}

x86_insn_t* x86_generate_d2s(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_CVTSD2SS);
    insn->size = c_type_to_x86_operand_size(ainsn->ct);
    insn->op1 = air_operand_to_x86_operand(ainsn->ops[1], routine);
    insn->op2 = air_operand_to_x86_operand(ainsn->ops[0], routine);

    return insn;
}

x86_insn_t* x86_generate_insn(air_insn_t* ainsn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    if (!ainsn) return NULL;
    switch (ainsn->type)
    {
        case AIR_LOAD:
        case AIR_ASSIGN:
            return x86_generate_load(ainsn, routine, file);
        
        case AIR_LOAD_ADDR: return x86_generate_load_addr(ainsn, routine, file);
        case AIR_FUNC_CALL: return x86_generate_func_call(ainsn, routine, file);
        case AIR_NOP: return x86_generate_nop(ainsn, routine, file);
        case AIR_DECLARE: return x86_generate_declare(ainsn, routine, file);
        case AIR_RETURN: return x86_generate_return(ainsn, routine, file);

        case AIR_ADD:
        case AIR_SUBTRACT:
        case AIR_MULTIPLY:
        case AIR_AND:
        case AIR_XOR:
        case AIR_OR:
        case AIR_SHIFT_LEFT:
        case AIR_SHIFT_RIGHT:
        case AIR_SIGNED_SHIFT_RIGHT:
            return x86_generate_binary_operator(ainsn, routine, file);
        
        case AIR_DIVIDE:
            return x86_generate_divide(ainsn, routine, file);

        case AIR_JZ:
        case AIR_JNZ:
            return x86_generate_conditional_jump(ainsn, routine, file);

        case AIR_JMP: return x86_generate_jmp(ainsn, routine, file);
        case AIR_LABEL: return x86_generate_label(ainsn, routine, file);
        case AIR_PUSH: return x86_generate_push(ainsn, routine, file);

        case AIR_DIRECT_ADD: 
        case AIR_DIRECT_SUBTRACT: 
        case AIR_DIRECT_MULTIPLY: 
        case AIR_DIRECT_AND: 
        case AIR_DIRECT_XOR: 
        case AIR_DIRECT_OR:
        case AIR_DIRECT_SHIFT_LEFT:
        case AIR_DIRECT_SHIFT_RIGHT:
        case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
            return x86_generate_direct_binary_operator(ainsn, routine, file);
        
        case AIR_NEGATE: return x86_generate_negate(ainsn, routine, file);

        case AIR_NOT: return x86_generate_not(ainsn, routine, file);

        case AIR_POSATE: return x86_generate_posate(ainsn, routine, file);
        
        case AIR_COMPLEMENT: return x86_generate_complement(ainsn, routine, file);

        case AIR_LESS_EQUAL:
        case AIR_LESS:
        case AIR_GREATER_EQUAL:
        case AIR_GREATER:
            return x86_generate_relational_equality_operator(ainsn, routine, file);

        case AIR_EQUAL:
        case AIR_INEQUAL:
            return x86_generate_equality_operator(ainsn, routine, file);
        
        case AIR_SEXT:
        case AIR_ZEXT:
            return x86_generate_extension(ainsn, routine, file);

        case AIR_S2D:
            return x86_generate_s2d(ainsn, routine, file);

        case AIR_D2S:
            return x86_generate_d2s(ainsn, routine, file);
        
        case AIR_DIRECT_DIVIDE:

        case AIR_S2SI:
        case AIR_S2UI:
        case AIR_D2SI:
        case AIR_D2UI:
        case AIR_SI2S:
        case AIR_UI2S:
        case AIR_SI2D:
        case AIR_UI2D:
            warnf("no x86 code generator built for an AIR instruction: %d\n", ainsn->type);
            return NULL;
        
        // this instruction is symbolic for earlier stages
        case AIR_DECLARE_REGISTER:

        // modulo operations get converted to division operations during x86 localization
        case AIR_MODULO:
        case AIR_DIRECT_MODULO:

        // phi instructions get deleted in an earlier stage
        case AIR_PHI:

        // varargs instructions get deleted in an earlier stage
        case AIR_VA_ARG:
        case AIR_VA_START:
        case AIR_VA_END:

        // sequence points don't do anything, they are markers
        case AIR_SEQUENCE_POINT:

            break;
    }
    return NULL;
}

x86_asm_routine_t* x86_generate_routine(air_routine_t* aroutine, x86_asm_file_t* file)
{
    x86_asm_routine_t* routine = calloc(1, sizeof *routine);
    routine->id = ++file->next_routine_id;
    routine->global = symbol_get_linkage(aroutine->sy) == LK_EXTERNAL;
    routine->label = strdup(symbol_get_name(aroutine->sy));
    routine->stackalloc = 0;
    if (aroutine->uses_varargs)
    {
        routine->stackalloc -= 176;
        routine->uses_varargs = true;
    }
    x86_insn_t* last = NULL;
    for (air_insn_t* ainsn = aroutine->insns; ainsn; ainsn = ainsn->next)
    {
        // skip first nop
        if (ainsn == aroutine->insns && ainsn->type == AIR_NOP)
            continue;
        x86_insn_t* insn = x86_generate_insn(ainsn, routine, file);
        if (!insn) continue;
        if (!last)
            routine->insns = last = insn;
        else
            last = last->next = insn;
        for (; last->next; last = last->next);
    }
    return routine;
}

x86_asm_data_t* x86_generate_data(air_data_t* adata, x86_asm_file_t* file)
{
    x86_asm_data_t* data = calloc(1, sizeof *data);
    data->alignment = type_alignment(adata->sy->type);
    long long size = type_size(adata->sy->type);
    if (size == -1)
        report_return_value(NULL);
    data->data = malloc(size);
    memcpy(data->data, adata->data, size);
    data->length = size;
    // TODO: something w/ static fields maybe here or earlier idk
    data->label = strdup(symbol_get_name(adata->sy));
    data->readonly = adata->readonly;
    return data;
}

x86_asm_file_t* x86_generate(air_t* air, symbol_table_t* st)
{
    x86_asm_file_t* file = calloc(1, sizeof *file);
    file->st = st;
    file->air = air;
    file->data = vector_init();
    file->rodata = vector_init();
    file->routines = vector_init();

    VECTOR_FOR(air_data_t*, data, air->data)
        vector_add(file->data, x86_generate_data(data, file));

    VECTOR_FOR(air_data_t*, rodata, air->rodata)
        vector_add(file->rodata, x86_generate_data(rodata, file));

    VECTOR_FOR(air_routine_t*, routine, air->routines)
        vector_add(file->routines, x86_generate_routine(routine, file));

    return file;
}
