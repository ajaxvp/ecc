#include <stdlib.h>
#include <string.h>

#include "cc.h"

/*

for each instruction:
    if instruction is a retain or restore instruction:
        continue
    replace all non-resulting virt. registers with their phys. mapping
    release any register mappings that expire on this instruction
    if there is no resulting virt. register:
        continue
    store where the resulting virt. register expires
    if the resulting virt. register is used a procedure call:
        get the phys. register corresponding to that virt. register's index in the argument list
        if that phys. register is occupied:
            insert a 'retain' for that phys. register before the current instruction
            insert a 'restore' for that phys. register after the resulting virt. register expires
    otherwise:
        get the next available phys. register
    map that phys. register to the resulting virt. register
    replace the resulting register with the phys. one

*/

regid_t find_physical_register(regid_t vreg, regid_t* regmap, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (regmap[i] == vreg)
            return i + 1;
    }
    return INVALID_VREGID;
}

void gather_vreg_info(regid_t vreg, ir_insn_t* insn, ir_insn_t* end, ir_insn_t** expiry, long long* proc_arg_index)
{
    *expiry = insn;
    *proc_arg_index = -1;
    for (; insn && insn != end; insn = insn->next)
    {
        for (size_t i = 0; i < insn->noops; ++i)
        {
            ir_insn_operand_t* op = insn->ops[i];
            if (op->type != IIOP_VIRTUAL_REGISTER) continue;
            if (op->vreg != vreg) continue;
            *expiry = insn;
            if (insn->type == II_FUNCTION_CALL && i > 1)
                *proc_arg_index = i - 2;
        }
    }
}


// prev inserting location next
void insert_insn_before(ir_insn_t* location, ir_insn_t* inserting)
{
    if (!location || !inserting) return;
    inserting->prev = location->prev;
    inserting->next = location;
    if (location->prev)
        location->prev->next = inserting;
    location->prev = inserting;
}

// prev location inserting next
void insert_insn_after(ir_insn_t* location, ir_insn_t* inserting)
{
    if (!location || !inserting) return;
    inserting->next = location->next;
    inserting->prev = location;
    if (location->next)
        location->next->prev = inserting;
    location->next = inserting;
}

void allocate_region(ir_insn_t* insns, ir_insn_t* end, regid_t (*procregmap)(long long), size_t count)
{
    // preg -> vreg
    regid_t* regmap = calloc(count, sizeof(regid_t));
    // preg -> last insn
    ir_insn_t** insnmap = calloc(count, sizeof(ir_insn_t*));
    
    for (ir_insn_t* insn = insns; insn && insn != end; insn = insn->next)
    {
        // if instruction is a retain or restore instruction, don't mess with it
        if (insn->type == II_RETAIN || insn->type == II_RESTORE)
            continue;
        // replace all non-resulting virt. registers with their phys. mapping
        for (size_t i = 0; i < insn->noops; ++i)
        {
            ir_insn_operand_t* op = insn->ops[i];
            if (op->type != IIOP_VIRTUAL_REGISTER) continue;
            if (op->result) continue;
            op->type = IIOP_PHYSICAL_REGISTER;
            op->preg = find_physical_register(op->vreg, regmap, count);
        }
        // release any register mappings that expire on this instruction
        for (size_t i = 0; i < count; ++i)
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
        gather_vreg_info(result->vreg, insn, end, &expiry, &proc_arg_index);
        regid_t preg = INVALID_VREGID;
        if (proc_arg_index != -1)
        {
            preg = procregmap(proc_arg_index);
            if (regmap[preg - 1] != INVALID_VREGID)
            {
                ir_insn_t* retain = make_1op(II_RETAIN, NULL, make_preg_insn_operand(preg, false));
                ir_insn_t* restore = make_1op(II_RESTORE, NULL, make_preg_insn_operand(preg, false));
                insert_insn_before(insn, retain);
                insert_insn_after(expiry, restore);
            }
        }
        else for (size_t i = 0; i < count; ++i)
            if (regmap[i] == INVALID_VREGID)
            {
                preg = i + 1;
                break;
            }
        if (preg == INVALID_VREGID)
            report_return;
        regmap[preg - 1] = result->vreg;
        insnmap[preg - 1] = expiry;
        result->type = IIOP_PHYSICAL_REGISTER;
        result->preg = preg;
    }

    free(regmap);
    free(insnmap);
}

void allocate(ir_insn_t* insns, regid_t (*procregmap)(long long), size_t count)
{
    if (!insns) return;
    // if we find that we need to allocate on the
    // basis of each basic block, we can do that
    // for now, don't worry about it
    ir_insn_t* dummy = calloc(1, sizeof *dummy);
    dummy->type = II_UNKNOWN;
    dummy->next = insns;
    insns->prev = dummy;
    allocate_region(insns, NULL, procregmap, count);
    insns->prev = NULL;
    insn_delete(dummy);
}
