#include <stdlib.h>
#include <string.h>

#include "cc.h"

/*

for each instruction:
    if instruction is a load with two operands which are phys. registers:
        remap the virtual register mapped by the right to the left
    replace all non-resulting virt. registers with their phys. mapping
    release any register mappings that expire on this instruction
    if there is no resulting virt. register:
        continue
    store where the resulting virt. register expires
    if the resulting virt. register is used in a procedure call:
        get the phys. register corresponding to that virt. register's index in the argument list
        if that phys. register is occupied:
            return to the last time that register had a value moved to it and store its value in the next nonvolatile phys. register
            insert a 'load' instruction to move that physical register's value into the corresponding argument phys. register just before the function call
    otherwise:
        get the next available volatile phys. register
        if there is a procedure call while the register's live:
            get the next available nonvolatile phys. register
    map that phys. register to the resulting virt. register
    replace the resulting register with the phys. one

*/

ir_insn_t* find_last_update(regid_t reg, ir_insn_t* start)
{
    for (; start; start = start->prev)
    {
        for (size_t i = 0; i < start->noops; ++i)
        {
            ir_insn_operand_t* op = start->ops[i];
            if (!op->result) continue;
            if ((op->type == IIOP_PHYSICAL_REGISTER && op->preg == reg) ||
                (op->type == IIOP_VIRTUAL_REGISTER && op->vreg == reg))
                return start;
        }
    }
    return NULL;
}

regid_t find_physical_register(regid_t vreg, regid_t* regmap, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (regmap[i] == vreg)
            return i + 1;
    }
    return INVALID_VREGID;
}

void gather_vreg_info(regid_t vreg, ir_insn_t* insn, ir_insn_t* end, ir_insn_t** expiry, ir_insn_t** proc_call, long long* proc_arg_index, bool* proc_call_inbetween)
{
    ir_insn_t* start = insn;
    *expiry = insn;
    *proc_call = NULL;
    *proc_arg_index = -1;
    bool inbetween = false;
    *proc_call_inbetween = false;
    for (; insn && insn != end; insn = insn->next)
    {
        if (insn->type == II_FUNCTION_CALL && insn != start)
            inbetween = true;
        for (size_t i = 0; i < insn->noops; ++i)
        {
            ir_insn_operand_t* op = insn->ops[i];
            if (op->type != IIOP_VIRTUAL_REGISTER) continue;
            if (op->vreg != vreg) continue;
            *expiry = insn;
            if (inbetween)
                *proc_call_inbetween = true;
            if (insn->type == II_FUNCTION_CALL && i > 1)
            {
                *proc_arg_index = i - 2;
                *proc_call = insn;
            }
        }
    }
}

void allocate_region(ir_insn_t* insns, ir_insn_t* end, allocator_options_t* options)
{
    // preg -> vreg
    regid_t* regmap = calloc(options->no_volatile + options->no_nonvolatile, sizeof(regid_t));
    // preg -> last insn
    ir_insn_t** insnmap = calloc(options->no_volatile + options->no_nonvolatile, sizeof(ir_insn_t*));
    
    for (ir_insn_t* insn = insns; insn && insn != end; insn = insn->next)
    {
        // if instruction is a retain or restore instruction, don't mess with it
        if (insn->type == II_RETAIN || insn->type == II_RESTORE)
            continue;
        // special case: load phys. register into phys. register will remap
        if (insn->type == II_LOAD && insn->noops == 2 &&
            insn->ops[0]->type == IIOP_PHYSICAL_REGISTER &&
            insn->ops[1]->type == IIOP_PHYSICAL_REGISTER)
        {
            regmap[insn->ops[0]->preg - 1] = regmap[insn->ops[1]->preg - 1];
            insnmap[insn->ops[0]->preg - 1] = insnmap[insn->ops[1]->preg - 1];
            regmap[insn->ops[1]->preg - 1] = INVALID_VREGID;
            insnmap[insn->ops[1]->preg - 1] = NULL;
            continue;
        }
        // replace all non-resulting virt. registers with their phys. mapping
        for (size_t i = 0; i < insn->noops; ++i)
        {
            ir_insn_operand_t* op = insn->ops[i];
            if (op->type != IIOP_VIRTUAL_REGISTER) continue;
            if (op->result) continue;
            op->type = IIOP_PHYSICAL_REGISTER;
            op->preg = find_physical_register(op->vreg, regmap, options->no_volatile + options->no_nonvolatile);
        }
        // release any register mappings that expire on this instruction
        for (size_t i = 0; i < options->no_volatile + options->no_nonvolatile; ++i)
        {
            if (insnmap[i] == insn)
            {
                insnmap[i] = NULL;
                regmap[i] = INVALID_VREGID;
            }
        }
        // if there is no resulting virt. register, don't mess with it
        if (insn->noops < 1)
            continue;
        if (!insn->ops[0]->result)
            continue;
        if (insn->ops[0]->type != IIOP_VIRTUAL_REGISTER)
            continue;
        ir_insn_operand_t* result = insn->ops[0];
        ir_insn_t* expiry = insn;
        long long proc_arg_index = -1;
        ir_insn_t* proc_call = NULL;
        bool proc_call_inbetween = false;
        gather_vreg_info(result->vreg, insn, end, &expiry, &proc_call, &proc_arg_index, &proc_call_inbetween);
        regid_t preg = INVALID_VREGID;
        // if the resulting virt. register is used a procedure call:
        if (proc_arg_index != -1)
        {
            // get the phys. register corresponding to that virt. register's index in the argument list
            preg = options->procregmap(proc_arg_index);
            // if that phys. register is occupied
            if (regmap[preg - 1] != INVALID_VREGID)
            {
                ir_insn_t* last = NULL;
                // return to the last time that register had a value moved to it and
                if ((last = find_last_update(preg, insn)))
                {
                    // store its value in the next nonvolatile phys. register
                    for (size_t i = 0; i < last->noops; ++i)
                    {
                        ir_insn_operand_t* op = last->ops[i];
                        if (!op->result) continue;
                        op->type = IIOP_PHYSICAL_REGISTER;
                        for (size_t j = options->no_volatile; j < options->no_volatile + options->no_nonvolatile; ++j)
                        {
                            if (regmap[j] != INVALID_VREGID) continue;
                            op->preg = j + 1;
                            break;
                        }
                        regmap[op->preg - 1] = regmap[preg - 1];
                        insnmap[op->preg - 1] = insnmap[preg - 1];
                        ir_insn_t* insn = make_2op(II_LOAD, last->ctype, make_preg_insn_operand(preg, true), make_preg_insn_operand(op->preg, false));
                        insert_ir_insn_after(proc_call, insn);
                        break;
                    }
                }
            }
        }
        else
        {
            size_t start = proc_call_inbetween ? options->no_volatile : 0;
            size_t end = proc_call_inbetween ? start + options->no_nonvolatile : options->no_volatile;
            for (size_t i = start; i < end; ++i)
                if (regmap[i] == INVALID_VREGID)
                {
                    preg = i + 1;
                    break;
                }
            
        }
        if (preg == INVALID_VREGID)
            report_return;
        if (expiry != insn)
        {
            regmap[preg - 1] = result->vreg;
            insnmap[preg - 1] = expiry;
        }
        else
        {
            regmap[preg - 1] = INVALID_VREGID;
            insnmap[preg - 1] = NULL;
        }
        result->type = IIOP_PHYSICAL_REGISTER;
        result->preg = preg;
    }

    free(regmap);
    free(insnmap);
}

void allocate(ir_insn_t* insns, allocator_options_t* options)
{
    if (!insns) return;
    // if we find that we need to allocate on the
    // basis of each basic block, we can do that
    // for now, don't worry about it
    ir_insn_t* dummy = calloc(1, sizeof *dummy);
    dummy->type = II_UNKNOWN;
    dummy->next = insns;
    insns->prev = dummy;
    allocate_region(insns, NULL, options);
    insns->prev = NULL;
    insn_delete(dummy);
}
