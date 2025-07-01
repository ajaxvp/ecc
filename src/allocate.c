#include <stdlib.h>
#include <stdio.h>

#include "ecc.h"

typedef struct allocinfo
{
    vector_t* conflicts; // vector_t<regid_t>
    vector_t* aliases; // vector_t<regid_t>
} allocinfo_t;

typedef struct allocator
{
    map_t* map; // map_t<regid_t, allocinfo_t>
    map_t* replacements; // map_t<regid_t, regid_t>
} allocator_t;

static int allocinfo_print(allocinfo_t* info, int (*printer)(const char*, ...))
{
    int rv = printer("allocinfo { conflicts: [");
    for (size_t i = 0; i < info->conflicts->size; ++i)
    {
        if (i) rv += printer(", ");
        rv += regid_print((regid_t) vector_get(info->conflicts, i), printer);
    }
    rv += printer("], aliases: [");
    for (size_t i = 0; i < info->aliases->size; ++i)
    {
        if (i) rv += printer(", ");
        rv += regid_print((regid_t) vector_get(info->aliases, i), printer);
    }
    rv += printer("] }");
    return rv;
}


void allocator_delete(allocator_t* a)
{
    if (!a) return;
    map_delete(a->map);
    map_delete(a->replacements);
    free(a);
}

void allocinfo_delete(allocinfo_t* info)
{
    if (!info) return;
    vector_delete(info->conflicts);
    vector_delete(info->aliases);
    free(info);
}

static air_insn_t* find_liveness_end(regid_t reg, air_insn_t* def, air_t* air)
{
    if (!def) return NULL;
    air_insn_t* last = def;
    for (air_insn_t* insn = def->next; insn; insn = insn->next)
    {
        if (insn->type == AIR_FUNC_CALL &&
            air->locale == LOC_X86_64 &&
            reg != X86R_RBX && reg != X86R_RBP && (reg < X86R_R12 || reg > X86R_R15) &&
            reg <= NO_PHYSICAL_REGISTERS)
        {
            last = insn;
            continue;
        }
        for (size_t i = 0; i < insn->noops; ++i)
        {
            air_insn_operand_t* op = insn->ops[i];
            if (!op) continue;
            if (op->type == AOP_INDIRECT_REGISTER && op->content.inreg.id == reg)
                last = insn;
            if (op->type == AOP_REGISTER && op->content.reg == reg)
                last = insn;
        }
    }
    if (get_program_options()->iflag)
    {
        printf("liveness ends for ");
        regid_print(reg, printf);
        printf(" at: ");
        air_insn_print(last, air, printf);
        printf("\n");
    }
    return last;
}

static void find_all_conflicts(air_routine_t* routine, allocator_t* a, air_t* air)
{
    for (air_insn_t* insn = routine->insns; insn; insn = insn->next)
    {
        if (!air_insn_creates_temporary(insn) &&
            (insn->type != AIR_ASSIGN || insn->noops < 1 || insn->ops[0]->type != AOP_REGISTER || insn->ops[0]->content.reg > NO_PHYSICAL_REGISTERS))
            continue;

        if (insn->noops < 1 || insn->ops[0]->type != AOP_REGISTER) report_return;

        regid_t reg = insn->ops[0]->content.reg;

        if (air->locale == LOC_X86_64 && insn->type == AIR_FUNC_CALL && reg == INVALID_VREGID)
            reg = X86R_RAX;

        if (reg == INVALID_VREGID) continue;

        allocinfo_t* info = calloc(1, sizeof *info);
        info->conflicts = vector_init();
        info->aliases = vector_init();
        
        map_add(a->map, (void*) reg, info);

        air_insn_t* end = find_liveness_end(reg, insn, air);

        for (air_insn_t* btinsn = end; btinsn && btinsn != insn; btinsn = btinsn->prev)
        {
            for (size_t i = 0; i < btinsn->noops; ++i)
            {
                if (air_insn_assigns(btinsn) && btinsn == end && i == 0)
                    continue;
                air_insn_operand_t* op = btinsn->ops[i];
                if (!op) continue;
                regid_t btreg = INVALID_VREGID;
                if (op->type == AOP_INDIRECT_REGISTER)
                    btreg = op->content.inreg.id;
                else if (op->type == AOP_REGISTER)
                    btreg = op->content.reg;
                else
                    continue;
                
                if (btreg == INVALID_VREGID)
                    continue;
                
                if (btreg == reg)
                    continue;
                
                vector_add_if_new(info->conflicts, (void*) btreg, (comparator_t) regid_comparator);
            }
        }
    }
}

static void coalesce(air_routine_t* routine, allocator_t* a, air_t* air)
{
    MAP_FOR(regid_t, allocinfo_t*, a->map)
    {
        if (MAP_IS_BAD_KEY) continue;
        regid_t ok = k;
        allocinfo_t* ov = v;
        MAP_FOR(regid_t, allocinfo_t*, a->map)
        {
            if (MAP_IS_BAD_KEY) continue;

            // ignore if it's the same register, we're not coalescing the register and itself
            if (ok == k) continue;

            // cannot coalesce two physical registers
            if (ok <= NO_PHYSICAL_REGISTERS && k <= NO_PHYSICAL_REGISTERS) continue;

            // if this register has the register we're merging as a conflict, skip it
            if (vector_contains(v->conflicts, (void*) ok, (comparator_t) regid_comparator) != -1)
                continue;
            
            // if any of this register's aliases has the register we're merging as a conflict, skip it
            bool found_conflict = false;
            VECTOR_FOR(regid_t, alias, ov->aliases)
            {
                if (vector_contains(v->conflicts, (void*) alias, (comparator_t) regid_comparator) != -1)
                {
                    found_conflict = true;
                    break;
                }
            }
            if (found_conflict)
                continue;
            
            // merge conflicts and add each other as aliases
            vector_add_if_new(ov->aliases, (void*) k, (comparator_t) regid_comparator);
            vector_merge(ov->conflicts, v->conflicts, (comparator_t) regid_comparator);

            vector_add_if_new(v->aliases, (void*) ok, (comparator_t) regid_comparator);
            vector_merge(v->conflicts, ov->conflicts, (comparator_t) regid_comparator);
        }
    }
}

static void replace_registers_x86_64(air_routine_t* routine, allocator_t* a, air_t* air)
{
    regid_t nextintreg = X86R_RAX;
    regid_t nextssereg = X86R_XMM0;

    // go thru each instruction and replace of all its temps with physical registers
    for (air_insn_t* insn = routine->insns; insn; insn = insn->next)
    {
        for (size_t i = 0; i < insn->noops; ++i)
        {
            air_insn_operand_t* op = insn->ops[i];
            if (!op) continue;

            // get the register associated with this operand, if any
            regid_t reg = INVALID_VREGID;
            if (op->type == AOP_REGISTER)
                reg = op->content.reg;
            else if (op->type == AOP_INDIRECT_REGISTER)
                reg = op->content.inreg.id;
            else
                continue;
            
            // if the register's already physical, we don't need to replace anything
            if (reg <= NO_PHYSICAL_REGISTERS)
                continue;
            
            // get allocation info and its replacement, if any
            allocinfo_t* info = map_get(a->map, (void*) reg);
            regid_t repl = (regid_t) map_get(a->replacements, (void*) reg);

            // if there's no replacement
            if (repl == INVALID_VREGID)
            {
                // search the aliases for a physical register
                VECTOR_FOR(regid_t, alias, info->aliases)
                {
                    if (alias <= NO_PHYSICAL_REGISTERS)
                    {
                        repl = alias;
                        break;
                    }
                    repl = (regid_t) map_get(a->replacements, (void*) alias);
                    if (repl != INVALID_VREGID)
                        break;
                }

                // if there isn't a physical register in the aliases
                if (repl == INVALID_VREGID)
                {
                    // go to the definition of the temporary
                    air_insn_t* def = air_insn_find_temporary_definition_above(reg, insn);
                    if (!def) report_return;
                    // and examine its type

                    // if it's an integer/pointer type, take next integer register available (skipping ones that were part of the allocation process)
                    if (type_is_integer(def->ct) || def->ct->class == CTC_POINTER)
                    {
                        for (; nextintreg <= X86R_R15 && map_contains_key(a->map, (void*) nextintreg); ++nextintreg);
                        if (nextintreg >= X86R_R15) report_return;
                        map_add(a->replacements, (void*) reg, (void*) (repl = nextintreg++));
                    }
                    // if it's a floating type, take next SSE register available (skipping in the same manner as above)
                    else if (type_is_real_floating(def->ct))
                    {
                        for (; nextintreg <= X86R_XMM7 && map_contains_key(a->map, (void*) nextssereg); ++nextssereg);
                        if (nextintreg >= X86R_XMM7) report_return;
                        map_add(a->replacements, (void*) reg, (void*) (repl = nextssereg++));
                    }
                    // TODO: complex numbas and maybe other?
                    else
                        report_return;
                }
                // if there is
                else
                    // use that
                    map_add(a->replacements, (void*) reg, (void*) repl);
            }
            
            // then replacement the temporary with the physical register
            if (op->type == AOP_REGISTER)
                op->content.reg = repl;
            else if (op->type == AOP_INDIRECT_REGISTER)
                op->content.inreg.id = repl;
            else 
                report_return;
        }
    }
}

static void replace_registers(air_routine_t* routine, allocator_t* a, air_t* air)
{
    switch (air->locale)
    {
        case LOC_X86_64: replace_registers_x86_64(routine, a, air); break;
        default: report_return;
    }
}

static void allocate_routine(air_routine_t* routine, air_t* air)
{
    if (!routine) return;

    allocator_t* a = calloc(1, sizeof *a);
    a->map = map_init((comparator_t) regid_comparator, (hash_function_t) regid_hash);
    a->replacements = map_init((comparator_t) regid_comparator, (hash_function_t) regid_hash);

    map_set_deleters(a->map, NULL, (deleter_t) allocinfo_delete);

    map_set_printers(a->map,
        (int (*)(void*, int (*)(const char*, ...))) regid_print,
        (int (*)(void*, int (*)(const char*, ...))) allocinfo_print);
    
    map_set_printers(a->replacements,
        (int (*)(void*, int (*)(const char*, ...))) regid_print,
        (int (*)(void*, int (*)(const char*, ...))) regid_print);

    find_all_conflicts(routine, a, air);

    if (get_program_options()->iflag)
        map_print(a->map, printf);

    coalesce(routine, a, air);

    if (get_program_options()->iflag)
        map_print(a->map, printf);

    replace_registers(routine, a, air);

    allocator_delete(a);
}

void allocate(air_t* air)
{
    VECTOR_FOR(air_routine_t*, routine, air->routines)
        allocate_routine(routine, air);
}
