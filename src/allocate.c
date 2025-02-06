#include <stdlib.h>
#include <string.h>

#include "cc.h"

#define REG_ARRAY_LENGTH 3

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

vector_t* get_registers(ir_insn_t* insn)
{
    if (!insn) return NULL;
    vector_t* v = vector_init();
    #define ADD(x) vector_add(v, (x))
    switch (insn->type)
    {
        case II_FUNCTION_CALL:
            ADD(&insn->function_call.calling);
            for (size_t i = 0; i < insn->function_call.noargs; ++i)
                ADD(insn->function_call.regargs + i);
            break;
        case II_RETURN:
            ADD(&insn->return_reg);
            break;
        case II_STORE_ADDRESS:
            ADD(&insn->store_address.src);
            break;
        case II_SUBSCRIPT:
        case II_ADDITION:
            ADD(&insn->bexpr.lhs);
            ADD(&insn->bexpr.rhs);
            break;
        case II_JUMP_IF_ZERO:
            ADD(&insn->cjmp.condition);
            break;
        case II_RETAIN:
            ADD(&insn->retain_reg);
            break;
        case II_RESTORE:
            ADD(&insn->restore_reg);
            break;
        default:
            break;
    }
    #undef ADD
    return v;
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

void gather_vreg_info(regid_t vreg, ir_insn_t* insn, ir_insn_t* end, ir_insn_t** expiry, long long* proc_arg_index)
{
    *expiry = insn;
    *proc_arg_index = -1;
    for (; insn && insn != end; insn = insn->next)
    {
        vector_t* regs = get_registers(insn);
        VECTOR_FOR(regid_t*, reg, regs)
        {
            if (*reg != vreg) continue;
            *expiry = insn;
            if (insn->type == II_FUNCTION_CALL && i > 0)
                *proc_arg_index = i - 1;
        }
        vector_delete(regs);
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
        vector_t* regs = get_registers(insn);
        VECTOR_FOR(regid_t*, reg, regs)
            *reg = find_physical_register(*reg, regmap, count);
        vector_delete(regs);
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
        if (insn->result == INVALID_VREGID)
            continue;
        ir_insn_t* expiry = insn;
        long long proc_arg_index = -1;
        gather_vreg_info(insn->result, insn, end, &expiry, &proc_arg_index);
        regid_t preg = INVALID_VREGID;
        if (proc_arg_index != -1)
        {
            preg = procregmap(proc_arg_index);
            if (regmap[preg - 1] != INVALID_VREGID)
            {
                ir_insn_t* retain = calloc(1, sizeof *retain);
                retain->type = II_RETAIN;
                retain->retain_reg = preg;
                ir_insn_t* restore = calloc(1, sizeof *restore);
                restore->type = II_RESTORE;
                restore->restore_reg = preg;
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
        regmap[preg - 1] = insn->result;
        insnmap[preg - 1] = expiry;
        insn->result = preg;
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
