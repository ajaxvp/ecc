#include <stdlib.h>
#include <stdio.h>

#include "ecc.h"

opt1_options_t opt_profile_basic = {
    .inline_fcalls = true,
};

opt1_options_t* opt1_profile_basic(void)
{
    return &opt_profile_basic;
}

/*

the following scenario:
    pointer to function(...) returning ... _1 = func;
    ... _2 = _1(...);
will be replaced with:
    ... _1 = func(...);

*/
static void try_inline_fcalls(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (insn->type != AIR_FUNC_CALL) return;
    air_insn_operand_t* op = insn->ops[1];
    if (op->type != AOP_REGISTER) return;
    air_insn_t* callexpr_insn = air_insn_find_temporary_definition_above(op->content.reg, insn);
    if (!callexpr_insn) return;
    if (callexpr_insn->type != AIR_LOAD_ADDR) return;
    air_insn_operand_t* funcop = callexpr_insn->ops[1];
    if (funcop->type != AOP_SYMBOL) return;
    op->type = AOP_SYMBOL;
    op->content.sy = funcop->content.sy;
    air_insn_remove(callexpr_insn);
}

void opt1(air_t* air, opt1_options_t* options)
{
    if (!options) return;
    VECTOR_FOR(air_routine_t*, routine, air->routines)
    {
        for (air_insn_t* insn = routine->insns; insn; insn = insn->next)
        {
            switch (insn->type)
            {
                case AIR_FUNC_CALL:
                    try_inline_fcalls(insn, routine, air);
                    break;
                default:
                    break;
            }
        }
    }
}
