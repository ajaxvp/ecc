#include <stdlib.h>
#include <stdio.h>

#include "ecc.h"

static opt1_options_t opt_profile_basic = {
    .inline_fcalls = true,
    .remove_fcall_passing_lifetimes = true,
    .inline_integer_constant = true
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

/*

attempt to prevent a temporary's lifetime from
passing a function

int _3 = 5; <-- insn
func(_4)
func(_3) <-- first_used

*/
static bool try_remove_fcall_passing_lifetimes(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (!insn) return false;
    bool side = air_insn_produces_side_effect(insn);
    bool fcall_found = false;
    air_insn_t* first_used = NULL;
    regid_t reg = insn->ops[0]->content.reg;
    for (air_insn_t* trace = insn->next; trace; trace = trace->next)
    {
        if (side && trace->type == AIR_SEQUENCE_POINT)
            break;
        bool uses = air_insn_uses(trace, reg);
        bool fcall = trace->type == AIR_FUNC_CALL;
        if (uses && !fcall_found) // like: _1 where _1 is defining and we're before any function calls
            return false;
        if (fcall) // like: func(_2) where _1 is defining
            fcall_found = true;
        if (uses && !first_used) // like: _3 = _1 where _1 is defining
            first_used = trace;
    }
    if (!first_used)
        return false;
    air_insn_move_before(insn, first_used);
    return true;
}

/*

int _1 = 5;
int _2 = _1;

becomes:

int _2 = 5;

*/

// static bool try_inline_integer_constant(air_insn_t* insn, air_routine_t* routine, air_t* air)
// {
//     if (!insn) return false;
//     if (insn->type != AIR_LOAD) return false;
//     if (insn->ops[1]->type != AOP_INTEGER_CONSTANT) return false;
//     regid_t reg = insn->ops[0]->content.reg;
//     unsigned long long value = insn->ops[1]->content.ic;
//     for (air_insn_t* trace = insn->next; trace; trace = trace->next)
//     {
//         for (size_t i = 0; i < trace->noops; ++i)
//         {
//             air_insn_operand_t* op = trace->ops[i];
//             if (!op) continue;
//             if (op->type == AOP_REGISTER && op->content.reg == reg)
//             {
//                 op->type = AOP_INTEGER_CONSTANT;
//                 op->content.ic = value;
//             }
//         }
//     }
//     air_insn_remove(insn);
//     return true;
// }

// static bool try_simplify_integer_add(air_insn_t* insn, air_routine_t* routine, air_t* air)
// {
//     if (!insn) return false;
//     if (insn->type != AIR_ADD) return false;
//     if (insn->ops[1]->type != AOP_INTEGER_CONSTANT ||
//         insn->ops[2]->type != AOP_INTEGER_CONSTANT)
//         return false;
//     unsigned long long result = insn->ops[1]->content.ic + insn->ops[2]->content.ic;
//     if (!representable(result, insn->ct->class))
//         return false;
//     air_insn_operand_delete(insn->ops[1]);
//     air_insn_operand_delete(insn->ops[2]);
//     insn->ops[1] = air_insn_integer_constant_operand_init(result);
//     insn->noops = 2;
//     insn->type = AIR_LOAD;
//     return true;
// }

// static bool constexpr_simplification(air_routine_t* routine, air_t* air)
// {
//     bool success = false;
//     for (air_insn_t* insn = routine->insns; insn;)
//     {
//         air_insn_t* next = insn->next;
//         switch (insn->type)
//         {
//             case AIR_LOAD:
//                 success |= try_inline_integer_constant(insn, routine, air);
//                 break;
//             case AIR_ADD:
//                 success |= try_simplify_integer_add(insn, routine, air);
//                 break;
//             default:
//                 break;
//         }
//         insn = next;
//     }
//     return success;
// }

void opt1(air_t* air, opt1_options_t* options)
{
    if (!options) return;
    VECTOR_FOR(air_routine_t*, routine, air->routines)
    {
        // if (get_program_options()->xflag)
            // while (constexpr_simplification(routine, air));
        air_insn_t* last = NULL;
        for (air_insn_t* insn = routine->insns; insn;)
        {
            if (!insn->next) last = insn;
            air_insn_t* next = insn->next;
            switch (insn->type)
            {
                case AIR_FUNC_CALL:
                    try_inline_fcalls(insn, routine, air);
                    break;
                default:
                    break;
            }
            insn = next;
        }
        while (last)
        {
            if (air_insn_creates_temporary(last))
            {
                air_insn_t* prev = last->prev;
                try_remove_fcall_passing_lifetimes(last, routine, air);
                last = prev;
            }
            else
                last = last->prev;
        }
    }
}
