#include <stdlib.h>
#include <stdio.h>

#include "cc.h"

ir_opt_options_t opt_profile_basic = {
    .inline_fcalls = true,
};

ir_opt_options_t* ir_opt_profile_basic(void)
{
    return &opt_profile_basic;
}

static ir_insn_t* find_vreg_definition(ir_insn_t* start, regid_t vreg)
{
    if (!start) return NULL;
    for (; start; start = start->prev)
    {
        for (size_t i = 0; i < start->noops; ++i)
        {
            ir_insn_operand_t* op = start->ops[i];
            if (!op->result) continue;
            if (op->type != IIOP_VIRTUAL_REGISTER) continue;
            if (op->vreg == vreg)
                return start;
        }
    }
    return NULL;
}

/*

the following scenario:
    pointer to function(...) returning ... _1 := func;
    ... _2 := _1(...);
will be replaced with:
    ... _1 := func(...);

*/
static void try_inline_fcalls(ir_insn_t* insn)
{
    if (insn->type != II_FUNCTION_CALL) return;
    ir_insn_operand_t* op = insn->ops[1];
    if (op->type != IIOP_VIRTUAL_REGISTER) return;
    ir_insn_t* callexpr_insn = find_vreg_definition(insn, op->vreg);
    if (!callexpr_insn) return;
    if (callexpr_insn->type != II_LOAD) return;
    ir_insn_operand_t* funcop = callexpr_insn->ops[1];
    if (funcop->type != IIOP_IDENTIFIER) return;
    op->type = IIOP_IDENTIFIER;
    op->id_symbol = funcop->id_symbol;
    remove_ir_insn(callexpr_insn);
}

void ir_optimize(ir_insn_t* insns, ir_opt_options_t* options)
{
    if (!insns || !options) return;
    ir_insn_t* dummy = calloc(1, sizeof *dummy);
    dummy->type = II_UNKNOWN;
    dummy->next = insns;
    insns->prev = dummy;
    for (; insns; insns = insns->next)
    {
        switch (insns->type)
        {
            case II_FUNCTION_CALL:
                try_inline_fcalls(insns);
                break;
            default:
                break;
        }
    }
    if (dummy->next)
        dummy->next->prev = NULL;
    insn_delete(dummy);
}