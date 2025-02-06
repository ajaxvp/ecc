#include <stdio.h>
#include <stdlib.h>

#include "cc.h"

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
    X86R_R12,
    X86R_R13,
    X86R_R14,
    X86R_R15,
    X86R_RSP,
    X86R_RBP
} x86_register_t;

const char* X86_REG64_MAP[] = {
    "rax",
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8",
    "r9",
    "r10",
    "r11",
    "rbx",
    "r12",
    "r13",
    "r14",
    "r15",
    "rsp",
    "rbp"
};

const char* X86_REG32_MAP[] = {
    "eax",
    "edi",
    "esi",
    "edx",
    "ecx",
    "r8d",
    "r9d",
    "r10d",
    "r11d",
    "ebx",
    "r12d",
    "r13d",
    "r14d",
    "r15d",
    "esp",
    "ebp"
};

const char* X86_REG16_MAP[] = {
    "ax",
    "di",
    "si",
    "dx",
    "cx",
    "r8w",
    "r9w",
    "r10w",
    "r11w",
    "bx",
    "r12w",
    "r13w",
    "r14w",
    "r15w",
    "sp",
    "bp"
};

const char* X86_REG8_MAP[] = {
    "al",
    "dil",
    "sil",
    "dl",
    "cl",
    "r8b",
    "r9b",
    "r10b",
    "r11b",
    "bl",
    "r12b",
    "r13b",
    "r14b",
    "r15b",
    "spl",
    "bpl"
};

const char* register_name(regid_t reg, x86_insn_size_t size)
{
    switch (size)
    {
        case X86SZ_BYTE: return X86_REG8_MAP[reg - 1];
        case X86SZ_WORD: return X86_REG16_MAP[reg - 1];
        case X86SZ_DWORD: return X86_REG32_MAP[reg - 1];
        case X86SZ_QWORD: return X86_REG64_MAP[reg - 1];
        default: return "badreg";
    }
}

// maps procedure argument indices to their corresponding physical registers
// for the x86-64 System V calling convention it is:
// 0 -> %rdi
// 1 -> %rsi
// 2 -> %rdx
// 3 -> %rcx
// 4 -> %r8
// 5 -> %r9
regid_t x86procregmap(long long index)
{
    if (index < 0 || index > 5)
        return INVALID_VREGID;
    return index + 2;
}

void x86_operand_delete(x86_operand_t* op)
{
    if (!op) return;
    switch (op->type)
    {
        case X86OP_LABEL:
            free(op->label);
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

void x86_operand_write(x86_operand_t* op, x86_insn_size_t size, FILE* file)
{
    if (!op) return;
    switch (op->type)
    {
        case X86OP_REGISTER:
            fprintf(file, "%%%s", register_name(op->reg, size));
            break;
        case X86OP_PTR_REGISTER:
            fprintf(file, "*%%%s", register_name(op->reg, size));
            break;
        case X86OP_DEREF_REGISTER:
            if (op->deref_reg.offset != 0)
                fprintf(file, "%lld", op->deref_reg.offset);
            fprintf(file, "(%%%s)", X86_REG64_MAP[op->deref_reg.reg - 1]);
            break;
        case X86OP_ARRAY:
            fprintf(file, "(%%%s, %%%s, %lld)", X86_REG64_MAP[op->array.reg_base - 1], X86_REG64_MAP[op->array.reg_offset - 1], op->array.scale);
            break;
        case X86OP_LABEL:
            fprintf(file, "%s", op->label);
            break;
        case X86OP_LABEL_REF:
            fprintf(file, "%s(%%rip)", op->label);
            break;
        case X86OP_IMMEDIATE:
            fprintf(file, "$%llu", op->immediate);
            break;
        default:
            break;
    }
}

void x86_write_all(x86_insn_t* insns, FILE* file)
{
    for (; insns; insns = insns->next)
        x86_write(insns, file);
}

void x86_write(x86_insn_t* insn, FILE* file)
{
    if (!insn) return;
    #define INDENT "    "
    #define USUAL_1OP(name) fprintf(file, INDENT name "%c ", x86_operand_size_character(insn->size)); goto op1;
    #define USUAL_2OP(name) fprintf(file, INDENT name "%c ", x86_operand_size_character(insn->size)); goto op2;
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
        
        case X86I_CALL:
            fprintf(file, INDENT "call ");
            x86_operand_write(insn->op1, X86SZ_QWORD, file);
            break;
        
        case X86I_PUSH: USUAL_1OP("push")

        case X86I_MOV: USUAL_2OP("mov")
        case X86I_LEA: USUAL_2OP("lea")
        case X86I_ADD: USUAL_2OP("add")
        case X86I_SUB: USUAL_2OP("sub")

        op1:
            x86_operand_write(insn->op1, insn->size, file);
            break;
        
        op2:
            x86_operand_write(insn->op1, insn->size, file);
            fprintf(file, ", ");
            x86_operand_write(insn->op2, insn->size, file);
            break;

        default:
            break;
    }
    #undef INDENT
    fprintf(file, "\n");
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

x86_operand_t* make_operand_label_ref(char* label)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_LABEL_REF);
    op->label = strdup(label);
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
    op->deref_reg.reg = reg;
    return op;
}

x86_operand_t* make_operand_immediate(long long immediate)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_IMMEDIATE);
    op->immediate = immediate;
    return op;
}

x86_operand_t* make_operand_array(regid_t reg_base, regid_t reg_offset, long long scale)
{
    x86_operand_t* op = make_basic_x86_operand(X86OP_ARRAY);
    op->array.reg_base = reg_base;
    op->array.reg_offset = reg_offset;
    op->array.scale = scale;
    return op;
}

x86_operand_t* locator_to_operand(locator_t* loc)
{
    if (!loc) return NULL;
    switch (loc->type)
    {
        case L_ARRAY:
            return make_operand_array(loc->array.base_reg, loc->array.offset_reg, loc->array.scale);
        case L_LABEL:
            return make_operand_label_ref(loc->label);
        case L_OFFSET:
            return make_operand_deref_register(X86R_RBP, loc->stack_offset);
        default:
            return NULL;
    }
}

x86_insn_t* make_basic_x86_insn(x86_insn_type_t type)
{
    x86_insn_t* insn = calloc(1, sizeof *insn);
    insn->type = type;
    return insn;
}

x86_insn_t* x86_generate_label(ir_insn_t* insn)
{
    x86_insn_t* x86_insn = make_basic_x86_insn(X86I_LABEL);
    x86_insn->op1 = make_operand_label(insn->label);
    return x86_insn;
}

x86_insn_t* x86_generate_enter(ir_insn_t* insn)
{
    x86_insn_t* push = make_basic_x86_insn(X86I_PUSH);
    push->size = X86SZ_QWORD;
    push->op1 = make_operand_register(X86R_RBP);
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = X86SZ_QWORD;
    mov->op1 = make_operand_register(X86R_RSP);
    mov->op2 = make_operand_register(X86R_RBP);
    x86_insn_t* sub = make_basic_x86_insn(X86I_SUB);
    sub->size = X86SZ_QWORD;
    sub->op1 = make_operand_immediate(insn->enter_stackalloc);
    sub->op2 = make_operand_register(X86R_RSP);
    push->next = mov;
    mov->next = sub;
    return push;
}

x86_insn_t* x86_generate_leave(ir_insn_t* insn)
{
    x86_insn_t* lv = make_basic_x86_insn(X86I_LEAVE);
    x86_insn_t* ret = make_basic_x86_insn(X86I_RET);
    lv->next = ret;
    return lv;
}

x86_insn_t* x86_generate_load_object(ir_insn_t* insn)
{
    x86_insn_t* ld = make_basic_x86_insn(insn->ctype->class == CTC_POINTER && insn->ctype->derived_from->class == CTC_FUNCTION ? X86I_LEA : X86I_MOV);
    ld->size = c_type_to_x86_operand_size(insn->ctype);
    ld->op1 = locator_to_operand(insn->loc);
    ld->op2 = make_operand_register(insn->result);
    return ld;
}

x86_insn_t* x86_generate_load_iconst(ir_insn_t* insn)
{
    x86_insn_t* ld = make_basic_x86_insn(X86I_MOV);
    ld->size = c_type_to_x86_operand_size(insn->ctype);
    ld->op1 = make_operand_immediate(insn->load_iconst_value);
    ld->op2 = make_operand_register(insn->result);
    return ld;
}

x86_insn_t* x86_generate_function_call(ir_insn_t* insn)
{
    x86_insn_t* cl = make_basic_x86_insn(X86I_CALL);
    cl->op1 = make_operand_ptr_register(insn->function_call.calling);
    if (insn->result != X86R_RAX)
    {
        x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
        mov->size = c_type_to_x86_operand_size(insn->ctype);
        mov->op1 = make_operand_register(X86R_RAX);
        mov->op2 = make_operand_register(insn->result);
        cl->next = mov;
    }
    return cl;
}

x86_insn_t* x86_generate_return(ir_insn_t* insn)
{
    if (insn->return_reg == X86R_RAX)
        return NULL;
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(insn->ctype);
    mov->op1 = make_operand_register(insn->return_reg);
    mov->op2 = make_operand_register(X86R_RAX);
    return mov;
}

x86_insn_t* x86_generate_addition(ir_insn_t* insn)
{
    x86_insn_t* add = make_basic_x86_insn(X86I_ADD);
    add->size = c_type_to_x86_operand_size(insn->ctype);
    add->op2 = make_operand_register(insn->bexpr.lhs);
    add->op1 = make_operand_register(insn->bexpr.rhs);
    if (insn->bexpr.lhs != insn->result)
    {
        x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
        mov->size = c_type_to_x86_operand_size(insn->ctype);
        mov->op1 = make_operand_register(insn->bexpr.lhs);
        mov->op2 = make_operand_register(insn->result);
        add->next = mov;
    }
    return add;
}

x86_insn_t* x86_generate_store_address(ir_insn_t* insn)
{
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(insn->ctype);
    mov->op1 = make_operand_register(insn->store_address.src);
    mov->op2 = locator_to_operand(insn->store_address.dest);
    if (insn->store_address.src != insn->result)
    {
        x86_insn_t* mov2 = make_basic_x86_insn(X86I_MOV);
        mov2->size = c_type_to_x86_operand_size(insn->ctype);
        mov2->op1 = make_operand_register(insn->store_address.src);
        mov2->op2 = make_operand_register(insn->result);
        mov->next = mov2;
    }
    return mov;
}

x86_insn_t* x86_generate(ir_insn_t* insns)
{
    x86_insn_t* dummy = make_basic_x86_insn(X86I_UNKNOWN);
    x86_insn_t* curr = dummy;
    x86_insn_t* next = NULL;
    #define ADD(f) ((next = f(insns)) ? (curr = curr->next = next) : NULL)
    for (; insns; insns = insns->next)
    {
        switch (insns->type)
        {
            case II_LABEL: ADD(x86_generate_label); break;
            case II_ENTER: ADD(x86_generate_enter); break;
            case II_LEAVE: ADD(x86_generate_leave); break;
            case II_LOAD_OBJECT: ADD(x86_generate_load_object); break;
            case II_LOAD_ICONST: ADD(x86_generate_load_iconst); break;
            case II_FUNCTION_CALL: ADD(x86_generate_function_call); break;
            case II_RETURN: ADD(x86_generate_return); break;
            case II_ADDITION: ADD(x86_generate_addition); break;
            case II_STORE_ADDRESS: ADD(x86_generate_store_address); break;
            default:
                break;
        }
        for (; curr->next; curr = curr->next);
    }
    x86_insn_t* head = dummy->next;
    x86_insn_delete(dummy);
    return head;
}