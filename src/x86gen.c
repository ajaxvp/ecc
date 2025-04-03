#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecc.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

#define MAX_CONSTANT_LOCAL_LABEL_LENGTH MAX_STRINGIFIED_INTEGER_LENGTH + 3

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

typedef struct x86_gen_state
{
    long long active_stackalloc;
    unsigned long long next_constant_local_label;
    ir_insn_t* active_function_ir;
    x86_insn_t* active_function;
    symbol_table_t* st;
    x86_insn_t* data_start;
    x86_insn_t* rodata_start;
    x86_insn_t* text_start;
} x86_gen_state_t;

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

void x86_gen_state_delete(x86_gen_state_t* state)
{
    if (!state) return;
    free(state);
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

// location next
// location insn next
void insert_x86_insn_after(x86_insn_t* insn, x86_insn_t* location)
{
    if (!insn || !location) return;
    insn->next = location->next;
    location->next = insn;
}

void insert_x86_insn_rodata(x86_insn_t* insn, x86_gen_state_t* state)
{
    if (!insn || !state) return;
    insert_x86_insn_after(insn, state->rodata_start);
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

void x86_register_write(regid_t reg, x86_insn_size_t size, FILE* file)
{
    fprintf(file, "%%%s", register_name(reg, size));
}

void x86_operand_write(x86_operand_t* op, x86_insn_size_t size, FILE* file)
{
    if (!op) return;
    switch (op->type)
    {
        case X86OP_REGISTER:
            x86_register_write(op->reg, size, file);
            break;
        case X86OP_PTR_REGISTER:
            fprintf(file, "*");
            x86_register_write(op->reg, size, file);
            break;
        case X86OP_DEREF_REGISTER:
            if (op->deref_reg.offset != 0)
                fprintf(file, "%lld", op->deref_reg.offset);
            fprintf(file, "(");
            x86_register_write(op->deref_reg.reg_addr, X86SZ_QWORD, file);
            fprintf(file, ")");
            break;
        case X86OP_ARRAY:
            fprintf(file, "(");
            x86_register_write(op->array.reg_base, X86SZ_QWORD, file);
            fprintf(file, ", ");
            x86_register_write(op->array.reg_offset, X86SZ_QWORD, file);
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

        case X86I_JMP:
            fprintf(file, INDENT "jmp ");
            x86_operand_write(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_JE:
            fprintf(file, INDENT "je ");
            x86_operand_write(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_JNE:
            fprintf(file, INDENT "jne ");
            x86_operand_write(insn->op1, X86SZ_QWORD, file);
            break;

        case X86I_SETE:
            fprintf(file, INDENT "sete ");
            x86_operand_write(insn->op1, X86SZ_BYTE, file);
            break;

        case X86I_SETL:
            fprintf(file, INDENT "setl ");
            x86_operand_write(insn->op1, X86SZ_BYTE, file);
            break;
        
        case X86I_PUSH: USUAL_1OP("push")

        case X86I_MOV: USUAL_2OP("mov")
        case X86I_LEA: USUAL_2OP("lea")
        case X86I_XOR: USUAL_2OP("xor")
        case X86I_ADD: USUAL_2OP("add")
        case X86I_SUB: USUAL_2OP("sub")
        case X86I_CMP: USUAL_2OP("cmp")
        case X86I_NOT: USUAL_2OP("not")

        op1:
            x86_operand_write(insn->op1, insn->size, file);
            break;
        
        op2:
            x86_operand_write(insn->op1, insn->size, file);
            fprintf(file, ", ");
            x86_operand_write(insn->op2, insn->size, file);
            break;
        
        case X86I_GLOBL:
            fprintf(file, INDENT ".globl ");
            x86_operand_write(insn->op1, insn->size, file);
            break;
        
        case X86I_DATA:
            fprintf(file, INDENT ".data");
            break;
        
        case X86I_TEXT:
            fprintf(file, INDENT ".text");
            break;
        
        case X86I_SECTION:
            fprintf(file, INDENT ".section ");
            x86_operand_write(insn->op1, insn->size, file);
            break;
        
        case X86I_STRING:
            fprintf(file, INDENT ".string ");
            x86_operand_write(insn->op1, insn->size, file);
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

char* next_constant_local_label(x86_gen_state_t* state, char* buffer)
{
    snprintf(buffer, MAX_CONSTANT_LOCAL_LABEL_LENGTH + 1, ".LC%llu", state->next_constant_local_label++);
    return buffer;
}

x86_insn_t* make_basic_x86_insn(x86_insn_type_t type);

x86_operand_t* ir_operand_to_operand(c_type_t* ctype, ir_insn_operand_t* iop, x86_gen_state_t* state)
{
    if (!iop) return NULL;
    switch (iop->type)
    {
        case IIOP_IDENTIFIER:
            return locator_to_operand(iop->id_symbol->loc);
        case IIOP_IMMEDIATE:
            return make_operand_immediate(iop->immediate);
        case IIOP_FLOAT:
            // TODO
            return NULL;
        case IIOP_STRING_LITERAL:
        {
            x86_insn_t* label = make_basic_x86_insn(X86I_LABEL);
            char buffer[MAX_CONSTANT_LOCAL_LABEL_LENGTH + 1];
            label->op1 = make_operand_label(next_constant_local_label(state, buffer));
            insert_x86_insn_rodata(label, state);
            if (iop->string_literal.normal)
            {
                x86_insn_t* string = make_basic_x86_insn(X86I_STRING);
                string->op1 = make_operand_string(iop->string_literal.normal);
                insert_x86_insn_after(string, label);
            }
            // TODO: handle wide strings
            // else
            return make_operand_label_ref(buffer);
        }
        case IIOP_LABEL:
            return make_operand_label_ref(iop->label);
        case IIOP_PHYSICAL_REGISTER:
            return make_operand_register(iop->preg);
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

x86_insn_t* make_x86_insn_clear_register(regid_t reg)
{
    x86_insn_t* insn = make_basic_x86_insn(X86I_XOR);
    insn->size = X86SZ_QWORD;
    insn->op1 = make_operand_register(reg);
    insn->op2 = make_operand_register(reg);
    return insn;
}

x86_insn_t* x86_generate_local_label(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* x86_insn = make_basic_x86_insn(X86I_LABEL);
    x86_insn->op1 = make_operand_label(insn->ops[0]->label);
    return x86_insn;
}

x86_insn_t* x86_generate_function_label(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* fl = make_basic_x86_insn(X86I_LABEL);
    fl->op1 = make_operand_label(symbol_get_name(insn->ops[0]->id_symbol));
    state->active_function_ir = insn;
    state->active_function = fl;
    size_t noparams = state->active_function_ir->metadata.function_label.noparams;
    for (size_t i = 0; i < min(noparams, 6); ++i)
    {
        symbol_t* sy = state->active_function_ir->metadata.function_label.params[i];
        if (!sy) continue;
        ir_insn_operand_t* op = make_identifier_insn_operand(sy);
        x86_operand_delete(ir_operand_to_operand(sy->type, op, state));
        insn_operand_delete(op);
    }
    return fl;
}

x86_insn_t* x86_generate_leave(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* lv = make_basic_x86_insn(X86I_LEAVE);
    x86_insn_t* ret = make_basic_x86_insn(X86I_RET);
    lv->next = ret;
    return lv;
}

x86_insn_t* x86_generate_load(ir_insn_t* insn, x86_gen_state_t* state)
{
    bool loadaddr = (insn->ctype->class == CTC_POINTER && insn->ctype->derived_from->class == CTC_FUNCTION) ||
        insn->ctype->class == CTC_ARRAY ||
        insn->type == II_LOAD_ADDRESS;
    x86_insn_t* ld = make_basic_x86_insn(loadaddr ? X86I_LEA : X86I_MOV);
    ld->size = loadaddr ? X86SZ_QWORD : c_type_to_x86_operand_size(insn->ctype);
    ld->op1 = ir_operand_to_operand(insn->ctype, insn->ops[1], state);
    ld->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    return ld;
}

x86_insn_t* x86_generate_function_call(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* cl = make_basic_x86_insn(X86I_CALL);
    switch (insn->ops[1]->type)
    {
        case IIOP_PHYSICAL_REGISTER:
            cl->op1 = make_operand_ptr_register(insn->ops[1]->vreg);
            break;
        case IIOP_IDENTIFIER:
            cl->op1 = make_operand_label(symbol_get_name(insn->ops[1]->id_symbol));
            break;
        default: report_return_value(NULL);
    }
    if (insn->ops[0]->type != IIOP_PHYSICAL_REGISTER || insn->ops[0]->preg != X86R_RAX)
    {
        x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
        mov->size = c_type_to_x86_operand_size(insn->ctype);
        mov->op1 = make_operand_register(X86R_RAX);
        mov->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
        cl->next = mov;
    }
    return cl;
}

x86_insn_t* x86_generate_return(ir_insn_t* insn, x86_gen_state_t* state)
{
    // no need to move to return value register if it's already there
    if (insn->ops[0]->type == IIOP_PHYSICAL_REGISTER && insn->ops[0]->preg == X86R_RAX)
        return NULL;
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(insn->ctype);
    mov->op1 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    mov->op2 = make_operand_register(X86R_RAX);
    return mov;
}

x86_insn_t* x86_generate_subscript(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* ld = make_basic_x86_insn(insn->type == II_SUBSCRIPT_ADDRESS ? X86I_LEA : X86I_MOV);
    ld->size = insn->type == II_SUBSCRIPT_ADDRESS ? X86SZ_QWORD : c_type_to_x86_operand_size(insn->ctype);
    ld->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    if (insn->ops[1]->type != IIOP_PHYSICAL_REGISTER || insn->ops[2]->type != IIOP_PHYSICAL_REGISTER)
        report_return_value(NULL);
    ld->op1 = make_operand_array(insn->ops[1]->vreg, insn->ops[2]->vreg, type_size(insn->type == II_SUBSCRIPT_ADDRESS ? insn->ctype->derived_from : insn->ctype));
    return ld;
}

x86_insn_t* x86_generate_binop(x86_insn_type_t type, ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* mov = NULL;
    if (!insn_operand_equals(insn->ops[1], insn->ops[0]))
    {
        mov = make_basic_x86_insn(X86I_MOV);
        mov->size = c_type_to_x86_operand_size(insn->ctype);
        mov->op1 = ir_operand_to_operand(insn->ctype, insn->ops[1], state);
        mov->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    }
    x86_insn_t* add = make_basic_x86_insn(type);
    if (mov)
        mov->next = add;
    else
        mov = add;
    add->size = c_type_to_x86_operand_size(insn->ctype);
    add->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    add->op1 = ir_operand_to_operand(insn->ctype, insn->ops[2], state);
    return mov;
}

x86_insn_t* x86_generate_addition(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_binop(X86I_ADD, insn, state);
}

x86_insn_t* x86_generate_subtraction(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_binop(X86I_SUB, insn, state);
}

x86_insn_t* x86_generate_unop(x86_insn_type_t type, ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* uninsn = make_basic_x86_insn(type);
    uninsn->size = c_type_to_x86_operand_size(insn->ctype);
    uninsn->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    uninsn->op1 = ir_operand_to_operand(insn->ctype, insn->ops[1], state);
    return uninsn;
}

x86_insn_t* x86_generate_not(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_unop(X86I_NOT, insn, state);
}

x86_insn_t* x86_generate_store_address(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(insn->ctype);
    mov->op1 = ir_operand_to_operand(insn->ctype, insn->ops[1], state);
    switch (insn->ops[0]->type)
    {
        case IIOP_PHYSICAL_REGISTER:
            mov->op2 = make_operand_deref_register(insn->ops[0]->vreg, 0);
            break;
        case IIOP_LABEL:
            mov->op2 = make_operand_label_ref(insn->ops[0]->label);
            break;
        case IIOP_IDENTIFIER:
            mov->op2 = make_operand_deref_register(X86R_RBP, insn->ops[0]->id_symbol->loc->stack_offset);
            break;
        default: report_return_value(NULL);
    }
    return mov;
}

x86_insn_t* x86_generate_endproc(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* root = state->active_function;
    if (state->active_function_ir->metadata.function_label.linkage == LK_EXTERNAL)
    {
        x86_insn_t* globl = make_basic_x86_insn(X86I_GLOBL);
        globl->op1 = make_operand_label(state->active_function->op1->label);
        insert_x86_insn_after(globl, root);
        root = globl;
    }
    x86_insn_t* push = make_basic_x86_insn(X86I_PUSH);
    push->size = X86SZ_QWORD;
    push->op1 = make_operand_register(X86R_RBP);
    insert_x86_insn_after(push, root);
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = X86SZ_QWORD;
    mov->op1 = make_operand_register(X86R_RSP);
    mov->op2 = make_operand_register(X86R_RBP);
    insert_x86_insn_after(mov, push);
    if (state->active_stackalloc)
    {
        state->active_stackalloc = abs(state->active_stackalloc);
        x86_insn_t* sub = make_basic_x86_insn(X86I_SUB);
        sub->size = X86SZ_QWORD;
        sub->op1 = make_operand_immediate(ALIGN(state->active_stackalloc, STACKFRAME_ALIGNMENT));
        sub->op2 = make_operand_register(X86R_RSP);
        insert_x86_insn_after(sub, mov);
        mov = sub;
    }
    size_t noparams = state->active_function_ir->metadata.function_label.noparams;
    long long offset = 0;
    for (size_t i = 0; i < min(noparams, 6); ++i)
    {
        symbol_t* sy = state->active_function_ir->metadata.function_label.params[i];
        if (!sy) continue;
        x86_insn_t* spill = make_basic_x86_insn(X86I_MOV);
        spill->size = c_type_to_x86_operand_size(sy->type);
        spill->op1 = make_operand_register(x86procregmap(i));
        spill->op2 = make_operand_deref_register(X86R_RBP, offset -= type_size(sy->type));
        insert_x86_insn_after(spill, mov);
        mov = spill;
    }
    state->active_function = NULL;
    state->active_function_ir = NULL;
    state->active_stackalloc = 0;
    return NULL;
}

x86_insn_t* x86_generate_jump(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* jmp = make_basic_x86_insn(X86I_JMP);
    if (insn->ops[0]->type != IIOP_LABEL)
        report_return_value(NULL);
    jmp->op1 = make_operand_label(insn->ops[0]->label);
    return jmp;
}

static x86_insn_t* x86_generate_jump_zero(ir_insn_t* insn, x86_gen_state_t* state, x86_insn_type_t type)
{
    x86_insn_t* cmp = make_basic_x86_insn(X86I_CMP);
    cmp->size = c_type_to_x86_operand_size(insn->ctype);
    cmp->op1 = make_operand_immediate(0);
    cmp->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    x86_insn_t* jmp = make_basic_x86_insn(type);
    if (insn->ops[1]->type != IIOP_LABEL)
        report_return_value(NULL);
    jmp->op1 = make_operand_label(insn->ops[1]->label);
    cmp->next = jmp;
    return cmp;
}

x86_insn_t* x86_generate_jump_if_zero(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_jump_zero(insn, state, X86I_JE);
}

x86_insn_t* x86_generate_jump_not_zero(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_jump_zero(insn, state, X86I_JNE);
}

x86_insn_t* x86_generate_declare(ir_insn_t* insn, x86_gen_state_t* state)
{
    ir_insn_operand_t* op = insn->ops[0];
    if (!op || op->type != IIOP_IDENTIFIER) report_return_value(NULL);
    storage_duration_t dur = symbol_get_storage_duration(op->id_symbol);
    if (dur != SD_AUTOMATIC) return NULL;
    symbol_t* sy = op->id_symbol;
    sy->loc = calloc(1, sizeof *sy->loc);
    sy->loc->type = L_OFFSET;
    syntax_component_t* pdecl = syntax_get_enclosing(sy->declarer, SC_PARAMETER_DECLARATION);
    x86_insn_t* r = NULL;
    if (pdecl)
    {
        syntax_component_t* fdeclr = pdecl->parent;
        if (!fdeclr || fdeclr->type != SC_FUNCTION_DECLARATOR) report_return_value(NULL);
        unsigned position = 0;
        VECTOR_FOR(syntax_component_t*, pd, fdeclr->fdeclr_parameter_declarations)
        {
            if (pdecl != pd) continue;
            position = i;
            break;
        }
        if (position < 6)
        {
            sy->loc->stack_offset = state->active_stackalloc -= type_size(sy->type);
            x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
            mov->size = c_type_to_x86_operand_size(sy->type);
            mov->op1 = make_operand_register(x86procregmap(position));
            mov->op2 = ir_operand_to_operand(insn->ctype, op, state);
            r = mov;
        }
        else
            sy->loc->stack_offset = 16 + 8 * (position - 6);
    }
    else
        sy->loc->stack_offset = state->active_stackalloc -= type_size(sy->type);
    return r;
}

x86_insn_t* x86_generate_dereference(ir_insn_t* insn, x86_gen_state_t* state)
{
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(insn->ctype);
    mov->op1 = make_operand_deref_register(insn->ops[1]->preg, 0);
    mov->op2 = make_operand_register(insn->ops[0]->preg);
    return mov;
}

static x86_insn_t* x86_generate_compare(ir_insn_t* insn, x86_gen_state_t* state, x86_insn_type_t type)
{
    x86_insn_t* cmp = make_basic_x86_insn(X86I_CMP);
    cmp->size = c_type_to_x86_operand_size(insn->ctype);
    cmp->op2 = ir_operand_to_operand(insn->ctype, insn->ops[1], state);
    cmp->op1 = ir_operand_to_operand(insn->ctype, insn->ops[2], state);
    if (insn->ops[0]->type != IIOP_PHYSICAL_REGISTER) report_return_value(NULL);
    x86_insn_t* sete = make_basic_x86_insn(type);
    sete->size = X86SZ_BYTE;
    sete->op1 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    cmp->next = sete;
    return cmp;
}

x86_insn_t* x86_generate_equality(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_compare(insn, state, X86I_SETE);
}

x86_insn_t* x86_generate_less(ir_insn_t* insn, x86_gen_state_t* state)
{
    return x86_generate_compare(insn, state, X86I_SETL);
}

x86_insn_t* x86_generate_cast(ir_insn_t* insn, x86_gen_state_t* state)
{
    if (insn->ops[0]->type != IIOP_PHYSICAL_REGISTER && insn->ops[2]->type != IIOP_PHYSICAL_REGISTER)
        report_return_value(NULL);
    x86_insn_t* mov = make_basic_x86_insn(X86I_MOV);
    mov->size = c_type_to_x86_operand_size(insn->ctype);
    mov->op2 = ir_operand_to_operand(insn->ctype, insn->ops[0], state);
    mov->op1 = ir_operand_to_operand(insn->ctype, insn->ops[2], state);
    return mov;
}

x86_insn_t* x86_generate(ir_insn_t* insns, symbol_table_t* st)
{
    x86_gen_state_t* state = calloc(1, sizeof *state);
    state->st = st;
    x86_insn_t* head = make_basic_x86_insn(X86I_DATA);
    x86_insn_t* curr = head;
    state->data_start = curr;
    x86_insn_t* rodata = make_basic_x86_insn(X86I_SECTION);
    rodata->op1 = make_basic_x86_operand(X86OP_TEXT);
    rodata->op1->text = strdup(".rodata");
    curr = curr->next = rodata;
    state->rodata_start = curr;
    x86_insn_t* text = make_basic_x86_insn(X86I_TEXT);
    curr = curr->next = text;
    state->text_start = curr;
    x86_insn_t* next = NULL;
    #define ADD(f) ((next = f(insns, state)) ? (curr = curr->next = next) : NULL)
    for (; insns; insns = insns->next)
    {
        switch (insns->type)
        {
            case II_FUNCTION_LABEL: ADD(x86_generate_function_label); break;
            case II_LOCAL_LABEL: ADD(x86_generate_local_label); break;
            case II_LEAVE: ADD(x86_generate_leave); break;
            case II_LOAD:
            case II_LOAD_ADDRESS:
                ADD(x86_generate_load);
                break;
            case II_FUNCTION_CALL: ADD(x86_generate_function_call); break;
            case II_RETURN: ADD(x86_generate_return); break;
            case II_ADDITION: ADD(x86_generate_addition); break;
            case II_SUBTRACTION: ADD(x86_generate_subtraction); break;
            case II_NOT: ADD(x86_generate_not); break;
            case II_STORE_ADDRESS: ADD(x86_generate_store_address); break;
            case II_SUBSCRIPT:
            case II_SUBSCRIPT_ADDRESS:
                ADD(x86_generate_subscript);
                break;
            case II_ENDPROC: ADD(x86_generate_endproc); break;
            case II_JUMP: ADD(x86_generate_jump); break;
            case II_JUMP_IF_ZERO: ADD(x86_generate_jump_if_zero); break;
            case II_JUMP_NOT_ZERO: ADD(x86_generate_jump_not_zero); break;
            case II_DECLARE: ADD(x86_generate_declare); break;
            case II_DEREFERENCE: ADD(x86_generate_dereference); break;
            case II_EQUALITY: ADD(x86_generate_equality); break;
            case II_LESS: ADD(x86_generate_less); break;
            case II_CAST: ADD(x86_generate_cast); break;
            default:
                warnf("no x86 code generator built for a linear IR instruction: %s\n", IR_INSN_NAMES[insns->type]);
                break;
        }
        for (; curr->next; curr = curr->next);
    }
    x86_gen_state_delete(state);
    return head;
}