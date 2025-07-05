#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "ecc.h"

typedef struct allocinfo
{
    vector_t* live_starts; // vector_t<uint64_t>
    vector_t* live_ends; // vector_t<uint64_t>
    vector_t* aliases; // vector_t<regid_t>
} allocinfo_t;

typedef struct allocator
{
    map_t* map; // map_t<regid_t, allocinfo_t>
    map_t* aliases; // map_t<regid_t, regid_t>
    map_t* replacements; // map_t<regid_t, regid_t>
} allocator_t;

static int allocinfo_print(allocinfo_t* info, int (*printer)(const char*, ...))
{
    int rv = printer("allocinfo { aliases: [");
    for (size_t i = 0; i < info->aliases->size; ++i)
    {
        if (i) rv += printer(", ");
        rv += regid_print((regid_t) vector_get(info->aliases, i), printer);
    }
    rv += printer("], live ranges: [");
    for (size_t i = 0; i < info->live_starts->size; ++i)
    {
        if (i) rv += printer(", ");
        rv += printer("%llu:%llu", (uint64_t) vector_get(info->live_starts, i), (uint64_t) vector_get(info->live_ends, i));
    }
    rv += printer("] }");
    return rv;
}

void allocinfo_delete(allocinfo_t* info)
{
    if (!info) return;
    vector_delete(info->aliases);
    vector_delete(info->live_starts);
    vector_delete(info->live_ends);
    free(info);
}

void allocator_delete(allocator_t* a)
{
    if (!a) return;
    map_delete(a->map);
    map_delete(a->replacements);
    free(a);
}

static air_insn_t* find_liveness_end(regid_t reg, air_insn_t* def, air_t* air, uint64_t* end_mark)
{
    if (!def) return NULL;
    air_insn_t* last = def;
    uint64_t e = *end_mark + 1;
    for (air_insn_t* insn = def->next; insn; insn = insn->next, ++e)
    {
        if (air_insn_creates_temporary(insn) && insn->ops[0]->content.reg == reg)
            break;
        if (insn->type == AIR_FUNC_CALL &&
            air->locale == LOC_X86_64 &&
            reg != X86R_RAX && reg != X86R_RBX && reg != X86R_RBP && (reg < X86R_R12 || reg > X86R_R15) &&
            reg <= NO_PHYSICAL_REGISTERS)
        {
            last = insn;
            *end_mark = e;
            continue;
        }
        for (size_t i = 0; i < insn->noops; ++i)
        {
            air_insn_operand_t* op = insn->ops[i];
            if (!op) continue;
            if ((op->type == AOP_INDIRECT_REGISTER && (op->content.inreg.id == reg || op->content.inreg.roffset == reg)) ||
                (op->type == AOP_REGISTER && op->content.reg == reg))
            {
                last = insn;
                *end_mark = e;
            }
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
    uint64_t i = 0;
    for (air_insn_t* insn = routine->insns; insn; insn = insn->next, ++i)
    {
        if (!air_insn_creates_temporary(insn))
            continue;

        if (insn->noops < 1 || insn->ops[0]->type != AOP_REGISTER) report_return;

        regid_t reg = insn->ops[0]->content.reg;

        if (reg == INVALID_VREGID) continue;

        allocinfo_t* info = map_get(a->map, (void*) reg);

        if (!info)
        {
            info = calloc(1, sizeof *info);
            info->aliases = vector_init();
            info->live_starts = vector_init();
            info->live_ends = vector_init();
            vector_add(info->aliases, (void*) reg);
            map_add(a->map, (void*) reg, info);
        }
        
        uint64_t end_mark = i;
        find_liveness_end(reg, insn, air, &end_mark);

        uint64_t start_mark = i + 1;

        if (start_mark > end_mark && reg > NO_PHYSICAL_REGISTERS)
        {
            map_remove(a->map, (void*) reg);
            insn = air_insn_remove(insn);
            continue;
        }

        vector_add(info->live_starts, (void*) (start_mark));
        vector_add(info->live_ends, (void*) end_mark);
    }
}

static bool live_range_conflicts(allocator_t* a, regid_t r1, regid_t r2)
{
    allocinfo_t* info1 = map_get(a->map, (void*) r1);
    allocinfo_t* info2 = map_get(a->map, (void*) r2);
    if (!info1 || !info2)
        return false;
    for (int i = 0; i < info1->live_starts->size; ++i)
    {
        uint64_t start1 = (uint64_t) vector_get(info1->live_starts, i);
        uint64_t end1 = (uint64_t) vector_get(info1->live_ends, i);
        for (int j = 0; j < info2->live_starts->size; ++j)
        {
            uint64_t start2 = (uint64_t) vector_get(info2->live_starts, j);
            uint64_t end2 = (uint64_t) vector_get(info2->live_ends, j);

            if (max(start1, start2) <= min(end1, end2))
                return true;
        }
    }
    return false;
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
            if (live_range_conflicts(a, k, ok))
                continue;
            
            // if any of this register's aliases has the register we're merging as a conflict, skip it
            bool found_conflict = false;
            VECTOR_FOR(regid_t, alias, ov->aliases)
            {
                if (live_range_conflicts(a, k, alias))
                {
                    found_conflict = true;
                    break;
                }
            }
            if (found_conflict)
                continue;
            
            air_insn_t* odef = air_insn_find_temporary_definition(ok, routine);
            air_insn_t* def = air_insn_find_temporary_definition(k, routine);
            if (!odef || !def) report_return;
            c_type_t* oct = odef->ct;
            c_type_t* ct = def->ct;
            if (air->locale == LOC_X86_64 && !x86_64_c_type_registers_compatible(oct, ct))
                continue;

            // merge conflicts and add each other as aliases
            vector_add_if_new(ov->aliases, (void*) k, (comparator_t) regid_comparator);
            vector_concat(ov->live_starts, v->live_starts);
            vector_concat(ov->live_ends, v->live_ends);

            map_remove(a->map, (void*) k);
            map_add(a->aliases, (void*) k, (void*) ok);
        }
    }
}

static regid_t find_replacement_x86_64(regid_t reg, air_insn_t* insn, allocator_t* a, regid_t* nextintreg, regid_t* nextssereg)
{
    // get allocation info and its replacement, if any
    for (regid_t actual = INVALID_VREGID;
        (actual = (regid_t) map_get(a->aliases, (void*) reg)) != INVALID_VREGID;
        reg = actual);
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
            if (!def) report_return_value(INVALID_VREGID);
            // and examine its type

            // if it's an integer/pointer type, take next integer register available (skipping ones that were part of the allocation process)
            if (type_is_integer(def->ct) || def->ct->class == CTC_POINTER)
            {
                for (; *nextintreg <= X86R_R15 && map_contains_key(a->map, (void*) (*nextintreg)); ++(*nextintreg));
                if (*nextintreg >= X86R_R15) report_return_value(INVALID_VREGID);
                map_add(a->replacements, (void*) reg, (void*) (repl = (*nextintreg)++));
            }
            // if it's a floating type, take next SSE register available (skipping in the same manner as above)
            else if (type_is_real_floating(def->ct))
            {
                for (; *nextssereg <= X86R_XMM7 && map_contains_key(a->map, (void*) (*nextssereg)); ++(*nextssereg));
                if (*nextssereg >= X86R_XMM7) report_return_value(INVALID_VREGID);
                map_add(a->replacements, (void*) reg, (void*) (repl = (*nextssereg)++));
            }
            // TODO: complex numbas and maybe other?
            else
                report_return_value(INVALID_VREGID);
        }
        // if there is
        else
            // use that
            map_add(a->replacements, (void*) reg, (void*) repl);
    }

    return repl;
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
            
            regid_t roffset = INVALID_VREGID;
            if (op->type == AOP_INDIRECT_REGISTER)
                roffset = op->content.inreg.roffset;
            
            // if the register's not physical, replace it
            if (reg != INVALID_VREGID && reg > NO_PHYSICAL_REGISTERS)
                reg = find_replacement_x86_64(reg, insn, a, &nextintreg, &nextssereg);
            
            // then replacement the temporary with the physical register
            if (op->type == AOP_REGISTER)
                op->content.reg = reg;
            else if (op->type == AOP_INDIRECT_REGISTER)
                op->content.inreg.id = reg;

            // if the offset register is not physical, replace it
            if (roffset != INVALID_VREGID && roffset > NO_PHYSICAL_REGISTERS)
                op->content.inreg.roffset = find_replacement_x86_64(roffset, insn, a, &nextintreg, &nextssereg);
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
    a->aliases = map_init((comparator_t) regid_comparator, (hash_function_t) regid_hash);
    a->replacements = map_init((comparator_t) regid_comparator, (hash_function_t) regid_hash);

    map_set_deleters(a->map, NULL, (deleter_t) allocinfo_delete);

    map_set_printers(a->map,
        (int (*)(void*, int (*)(const char*, ...))) regid_print,
        (int (*)(void*, int (*)(const char*, ...))) allocinfo_print);

    map_set_printers(a->aliases,
        (int (*)(void*, int (*)(const char*, ...))) regid_print,
        (int (*)(void*, int (*)(const char*, ...))) regid_print);
    
    map_set_printers(a->replacements,
        (int (*)(void*, int (*)(const char*, ...))) regid_print,
        (int (*)(void*, int (*)(const char*, ...))) regid_print);

    find_all_conflicts(routine, a, air);

    if (get_program_options()->iflag)
        map_print(a->map, printf);

    coalesce(routine, a, air);

    if (get_program_options()->iflag)
    {
        map_print(a->map, printf);
        map_print(a->aliases, printf);
    }

    replace_registers(routine, a, air);

    allocator_delete(a);
}

void allocate(air_t* air)
{
    VECTOR_FOR(air_routine_t*, routine, air->routines)
        allocate_routine(routine, air);
}
