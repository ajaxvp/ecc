#include <stdio.h>
#include <stdlib.h>

#include "ecc.h"

static opt4_options_t opt_profile_basic = {
    .remove_same_reg_moves = true,
    .xor_zero_moves = true
};

opt4_options_t* opt4_profile_basic(void)
{
    return &opt_profile_basic;
}

/*

removes instructions like:
    movl %eax, %eax
    movss %xmm0, %xmm0

*/
static bool try_remove_same_reg_moves(x86_insn_t* insn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    if (!insn->op1 || !insn->op2) return false;
    if (insn->op1->type != X86OP_REGISTER || insn->op2->type != X86OP_REGISTER) return false;
    if (insn->op1->reg != insn->op2->reg) return false;
    x86_operand_delete(insn->op1);
    x86_operand_delete(insn->op2);
    insn->op1 = insn->op2 = NULL;
    insn->type = X86I_SKIP;
    return true;
}

/*

converts instructions like:
    movl $0, %eax
into:
    xorl %eax, %eax

*/
static bool try_xor_zero_moves(x86_insn_t* insn, x86_asm_routine_t* routine, x86_asm_file_t* file)
{
    if (!insn->op1 || !insn->op2) return false;
    if (insn->op2->type != X86OP_REGISTER) return false;
    if (insn->op1->type != X86OP_IMMEDIATE) return false;
    if (insn->op1->immediate != 0) return false;
    if (!x86_64_is_integer_register(insn->op2->reg)) return false;
    insn->type = X86I_XOR;
    insn->op1->type = X86OP_REGISTER;
    insn->op1->reg = insn->op2->reg;
    return true;
}

void opt4(x86_asm_file_t* file, opt4_options_t* options)
{
    VECTOR_FOR(x86_asm_routine_t*, routine, file->routines)
    {
        for (x86_insn_t* insn = routine->insns; insn; insn = insn->next)
        {
            switch (insn->type)
            {
                case X86I_MOV:
                    if (try_xor_zero_moves(insn, routine, file))
                        break;
                case X86I_MOVSD:
                case X86I_MOVSS:
                    try_remove_same_reg_moves(insn, routine, file);
                    break;
                default:
                    break;
            }
        }
    }
}
