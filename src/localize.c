#include <stdlib.h>
#include <stdio.h>

#include "ecc.h"

#define NEXT_VIRTUAL_REGISTER (air->next_available_temporary++)
#define NEXT_LV (air->next_available_lv++)
#define SYMBOL_TABLE (air->st)

typedef enum arg_class
{
    ARG_NO_CLASS,
    ARG_INTEGER,
    ARG_SSE,
    ARG_SSEUP,
    ARG_X87,
    ARG_X87UP,
    ARG_COMPLEX_X87,
    ARG_MEMORY
} arg_class_t;

// [ %rdi ][ %rsi ][ %rdx ][ %rcx ][ %r8 ][ %r9 ][ sp ][ sp + 8 ]

static arg_class_t* arg_class_singleton(arg_class_t class)
{
    arg_class_t* r = calloc(1, sizeof *r);
    *r = class;
    return r;
}

static arg_class_t* find_aggregate_union_classes(c_type_t* ct, size_t* count);

static arg_class_t* find_classes(c_type_t* ct, size_t* count)
{
    if (count) *count = 0;
    if (!ct) return NULL;
    if (count) *count = 1;
    if (type_is_integer(ct))
        return arg_class_singleton(ARG_INTEGER);
    // shouldn't have array types as args (they get implicitly converted to ptrs) but whatever
    if (ct->class == CTC_POINTER || ct->class == CTC_ARRAY)
        return arg_class_singleton(ARG_INTEGER);
    if (ct->class == CTC_FLOAT || ct->class == CTC_DOUBLE || ct->class == CTC_FLOAT_COMPLEX)
        return arg_class_singleton(ARG_SSE);
    if (count) *count = 2;
    if (ct->class == CTC_LONG_DOUBLE)
    {
        arg_class_t* r = calloc(2, sizeof *r);
        r[0] = ARG_X87;
        r[1] = ARG_X87UP;
        return r;
    }
    if (ct->class == CTC_DOUBLE_COMPLEX)
    {
        arg_class_t* r = calloc(2, sizeof *r);
        r[0] = r[1] = ARG_SSE;
        return r;
    }
    if (count) *count = 4;
    if (ct->class == CTC_LONG_DOUBLE_COMPLEX)
    {
        arg_class_t* r = calloc(4, sizeof *r);
        for (size_t i = 0; i < 4; ++i)
            r[i] = ARG_COMPLEX_X87;
        return r;
    }
    return find_aggregate_union_classes(ct, count);
}

static arg_class_t* find_aggregate_union_classes(c_type_t* ct, size_t* count)
{
    if (!ct) return NULL;
    if (ct->class != CTC_ARRAY && ct->class != CTC_STRUCTURE && ct->class != CTC_UNION) report_return_value(NULL);
    long long size = type_size(ct);
    *count = (size + (8 - (size % 8)) % 8) >> 3;
    arg_class_t* classes = calloc(*count, sizeof *classes);
    if (size > 64)
    {
        for (size_t i = 0; i < *count; ++i)
            classes[i] = ARG_MEMORY;
        return classes;
    }
    for (size_t i = 0; i < *count; ++i)
        classes[i] = ARG_NO_CLASS;
    long long offset = 0;
    VECTOR_FOR(c_type_t*, mt, ct->struct_union.member_types)
    {
        long long ms = type_size(mt); 
        long long ma = type_alignment(mt);
        offset += (ma - (size % ma)) % ma;

        size_t subclasses_count = 0;
        arg_class_t* subclasses = find_classes(mt, &subclasses_count);
        for (size_t j = 0; j < subclasses_count; ++j)
        {
            long long class_idx = (offset >> 3) + j;
            arg_class_t class = classes[class_idx];
            arg_class_t subclass = subclasses[j];
            if (class == subclass)
                continue;
            if (subclass == ARG_NO_CLASS)
                continue;
            if (class == ARG_NO_CLASS || subclass == ARG_MEMORY || subclass == ARG_INTEGER)
                classes[class_idx] = subclass;
            else if (class == ARG_X87 ||
                class == ARG_X87UP ||
                class == ARG_COMPLEX_X87 ||
                subclass == ARG_X87 ||
                subclass == ARG_X87UP || 
                subclass == ARG_COMPLEX_X87)
                classes[class_idx] = ARG_MEMORY;
            else
                classes[class_idx] = ARG_SSE;
        }
        free(subclasses);

        offset += ms;
    }
    // post-merger
    bool first_sse = classes[0] == ARG_SSE;
    bool has_sseup = false;
    for (size_t i = 0; i < *count; ++i)
    {
        if (classes[i] == ARG_MEMORY ||
            (i >= 1 && classes[i] == ARG_X87UP && classes[i - 1] != ARG_X87))
        {
            for (size_t i = 0; i < *count; ++i)
                classes[i] = ARG_MEMORY;
            return classes;
        }

        if (classes[i] == ARG_SSEUP)
            has_sseup = true;

        if (i >= 1 && classes[i] == ARG_SSEUP && classes[i - 1] != ARG_SSE && classes[i - 1] != ARG_SSEUP)
            classes[i] = ARG_SSE;
    }
    if (*count > 2 && (!first_sse || !has_sseup))
    {
        for (size_t i = 0; i < *count; ++i)
            classes[i] = ARG_MEMORY;   
    }
    return classes;
}

/*

struct point* _1 = &p;

push(*p);

*/

// loc is the location to insert the instructions ABOVE
// tempdef is where the temporary was defined that holds the argument
// ct is the type of the argument
// eightbyte_idx is this eightbyte's index in the current sequence of eightbyte classifications
// return is the first instruction inserted
air_insn_t* store_eightbyte_on_stack(air_insn_t* loc, air_insn_t* tempdef, c_type_t* ct, size_t eightbyte_idx, air_t* air)
{
    regid_t argreg = tempdef->ops[0]->type == AOP_REGISTER ? tempdef->ops[0]->content.reg : tempdef->ops[0]->content.inreg.id;
    if (ct->class == CTC_STRUCTURE || ct->class == CTC_UNION)
    {
        long long size = type_size(ct);
        size_t progress = eightbyte_idx << 3;
        long long total_remaining = size - progress;
        if (total_remaining >= UNSIGNED_LONG_LONG_INT_WIDTH)
        {
            air_insn_t* push = air_insn_init(AIR_PUSH, 1);
            push->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
            push->ops[0] = air_insn_indirect_register_operand_init(argreg, progress, INVALID_VREGID, 1);
            air_insn_insert_before(push, loc);
            return push;
        }
        air_insn_t* first = NULL;
        regid_t tmpreg = NEXT_VIRTUAL_REGISTER;

        air_insn_t* xor = air_insn_init(AIR_DIRECT_XOR, 2);
        xor->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        xor->ops[0] = air_insn_register_operand_init(tmpreg);
        xor->ops[1] = air_insn_register_operand_init(tmpreg);
        air_insn_insert_before(xor, loc);

        first = xor;

        for (long long copied = 0; copied < total_remaining;)
        {
            long long remaining = total_remaining - copied;
            c_type_class_t class = CTC_UNSIGNED_LONG_LONG_INT;
            if (remaining < UNSIGNED_SHORT_INT_WIDTH)
                class = CTC_UNSIGNED_CHAR;
            else if (remaining < UNSIGNED_INT_WIDTH)
                class = CTC_UNSIGNED_SHORT_INT;
            else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
                class = CTC_UNSIGNED_INT;
            c_type_t* cyt = make_basic_type(class);

            if (copied)
            {
                air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
                shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shl->ops[0] = air_insn_register_operand_init(tmpreg);
                shl->ops[1] = air_insn_integer_constant_operand_init(type_size(cyt) << 3);
                air_insn_insert_before(shl, loc);
            }

            air_insn_t* or = air_insn_init(AIR_DIRECT_OR, 2);
            or->ct = cyt;
            or->ops[0] = air_insn_register_operand_init(tmpreg);
            or->ops[1] = air_insn_indirect_register_operand_init(argreg, progress + copied, INVALID_VREGID, 1);
            air_insn_insert_before(or, loc);

            copied += type_size(cyt);
        }
        
        air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
        shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        shl->ops[0] = air_insn_register_operand_init(tmpreg);
        shl->ops[1] = air_insn_integer_constant_operand_init((8 - total_remaining) << 3);
        air_insn_insert_before(shl, loc);

        air_insn_t* push = air_insn_init(AIR_PUSH, 1);
        push->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        push->ops[0] = air_insn_register_operand_init(tmpreg);
        air_insn_insert_before(push, loc);

        return first;
    }
    air_insn_t* push = air_insn_init(AIR_PUSH, 1);
    push->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    push->ops[0] = air_insn_register_operand_init(argreg);
    air_insn_insert_before(push, loc);
    return push;
}

air_insn_t* store_eightbyte_in_register(air_insn_t* loc, air_insn_t* tempdef, c_type_t* ct, size_t eightbyte_idx, regid_t dest, air_t* air)
{
    regid_t argreg = tempdef->ops[0]->type == AOP_REGISTER ? tempdef->ops[0]->content.reg : tempdef->ops[0]->content.inreg.id;
    if (ct->class == CTC_STRUCTURE || ct->class == CTC_UNION)
    {
        long long size = type_size(ct);
        size_t progress = eightbyte_idx << 3;
        long long total_remaining = size - progress;
        if (total_remaining >= UNSIGNED_LONG_LONG_INT_WIDTH)
        {
            air_insn_t* deref = air_insn_init(AIR_ASSIGN, 2);
            deref->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
            deref->ops[0] = air_insn_register_operand_init(dest);
            deref->ops[1] = air_insn_indirect_register_operand_init(argreg, progress, INVALID_VREGID, 1);
            air_insn_insert_before(deref, loc);
            return deref;
        }
        air_insn_t* first = NULL;
        for (long long copied = 0; copied < total_remaining;)
        {
            long long remaining = total_remaining - copied;
            c_type_class_t class = CTC_UNSIGNED_LONG_LONG_INT;
            if (remaining < UNSIGNED_SHORT_INT_WIDTH)
                class = CTC_UNSIGNED_CHAR;
            else if (remaining < UNSIGNED_INT_WIDTH)
                class = CTC_UNSIGNED_SHORT_INT;
            else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
                class = CTC_UNSIGNED_INT;
            c_type_t* cyt = make_basic_type(class);

            if (copied)
            {
                air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
                shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shl->ops[0] = air_insn_register_operand_init(dest);
                shl->ops[1] = air_insn_integer_constant_operand_init(type_size(cyt) << 3);
                air_insn_insert_before(shl, loc);
            }

            air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
            assign->ct = cyt;
            assign->ops[0] = air_insn_register_operand_init(dest);
            assign->ops[1] = air_insn_indirect_register_operand_init(argreg, progress + copied, INVALID_VREGID, 1);
            air_insn_insert_before(assign, loc);

            if (!first) first = assign;

            copied += type_size(cyt);
        }

        air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
        shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        shl->ops[0] = air_insn_register_operand_init(dest);
        shl->ops[1] = air_insn_integer_constant_operand_init((8 - total_remaining) << 3);
        air_insn_insert_before(shl, loc);

        return first;
    }
    air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
    assign->ct = type_copy(ct);
    assign->ops[0] = air_insn_register_operand_init(dest);
    assign->ops[1] = air_insn_register_operand_init(argreg);
    air_insn_insert_before(assign, loc);
    return assign;
}

// guyyyyy this sucks
void localize_x86_64_func_call_args(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    regid_t nextintreg = X86R_RDI;
    bool rdiret = insn->metadata.fcall_sret && type_size(insn->ct->derived_from) > 16;
    if (rdiret)
        nextintreg = X86R_RSI;
    regid_t nextssereg = X86R_XMM0;
    air_insn_t* inserting = insn;
    for (size_t i = 2; i < insn->noops; ++i)
    {
        air_insn_operand_t* op = insn->ops[i];
        if (op->type != AOP_REGISTER && op->type != AOP_INDIRECT_REGISTER) report_return;
        air_insn_t* tempdef = air_insn_find_temporary_definition_above(op->type == AOP_REGISTER ? op->content.reg : op->content.inreg.id, insn);
        if (!tempdef) report_return;

        c_type_t* at = tempdef->ct;
        if (op->type == AOP_INDIRECT_REGISTER)
            at = at->derived_from;
        
        size_t ccount = 0;
        arg_class_t* classes = find_classes(at, &ccount);
        if (!classes) report_return;

        for (size_t i = 0; i < ccount; ++i)
        {
            arg_class_t class = classes[i];
            if (class == ARG_MEMORY ||
                class == ARG_X87 ||
                class == ARG_X87UP ||
                class == ARG_COMPLEX_X87 ||
                (class == ARG_INTEGER && nextintreg > X86R_R9) ||
                (class == ARG_SSE && nextssereg > X86R_XMM7))
                inserting = store_eightbyte_on_stack(inserting, tempdef, at, i, air);
            else if (class == ARG_INTEGER)
                inserting = store_eightbyte_in_register(inserting, tempdef, at, i, nextintreg++, air);
            else if (class == ARG_SSE)
                inserting = store_eightbyte_in_register(inserting, tempdef, at, i, nextssereg++, air);
            else if (class == ARG_SSEUP)
                inserting = store_eightbyte_in_register(inserting, tempdef, at, i, nextssereg - 1, air);
        }

        free(classes);
    }

    if (rdiret)
    {
        symbol_t* sy = symbol_table_add(SYMBOL_TABLE, "__anonymous_lv__", symbol_init(NULL));
        sy->type = type_copy(insn->ct->derived_from);

        air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
        decl->ops[0] = air_insn_symbol_operand_init(sy);
        air_insn_insert_before(decl, insn);

        air_insn_t* loadaddr = air_insn_init(AIR_LOAD_ADDR, 2);
        loadaddr->ct = type_copy(insn->ct);
        loadaddr->ops[0] = air_insn_register_operand_init(X86R_RDI);
        loadaddr->ops[1] = air_insn_symbol_operand_init(sy);
        air_insn_insert_before(loadaddr, insn);
    }

    // TODO: check to see if the function has no prototype or varargs
    if (nextssereg - X86R_XMM0 > 0)
    {
        air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
        assign->ct = make_basic_type(CTC_UNSIGNED_CHAR);
        assign->ops[0] = air_insn_register_operand_init(X86R_RAX);
        assign->ops[1] = air_insn_integer_constant_operand_init(nextssereg - X86R_XMM0);
        air_insn_insert_before(assign, insn);
    }

    for (size_t i = 2; i < insn->noops; ++i)
    {
        air_insn_operand_delete(insn->ops[i]);
        insn->ops[i] = air_insn_register_operand_init(INVALID_VREGID);
    }
}

/*

type _1 = _2(_3, _4, _5, ...)

becomes:

MEMORY:
    call(_2);
    type _1 = %rax;

INTEGER:
    (if returning anything typical)
    call(_2);
    type _1 = %rax;

    (if returning a struct)
    call(_2);
    deref type __lv0;
    type _1 = &__lv0;
    *_1 = %rax;

*/

void localize_x86_64_func_call_return(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (insn->ops[0]->type != AOP_REGISTER) report_return;

    regid_t integer_return_sequence[] = { X86R_RAX, X86R_RDX };
    size_t next_intretreg = 0;
    // regid_t next_sseretreg = X86R_XMM0;

    regid_t resreg = insn->ops[0]->content.reg;
    air_insn_operand_delete(insn->ops[0]);
    insn->ops[0] = air_insn_register_operand_init(INVALID_VREGID);

    size_t ccount = 0;
    c_type_t* ct = insn->ct;
    if (insn->metadata.fcall_sret)
        ct = ct->derived_from;
    arg_class_t* classes = find_classes(ct, &ccount);
    if (!classes) report_return;
    if (classes[0] == ARG_MEMORY ||
        (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION && classes[0] == ARG_INTEGER))
    {
        // TODO: support cases where there are multiple INTEGER classes
        air_insn_t* load = air_insn_init(AIR_LOAD, 2);
        load->ct = type_copy(insn->ct);
        load->ops[0] = air_insn_register_operand_init(resreg);
        load->ops[1] = air_insn_register_operand_init(X86R_RAX);
        air_insn_insert_after(load, insn);
        free(classes);
        return;
    }
    if (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION && classes[0] == ARG_SSE)
    {
        air_insn_t* load = air_insn_init(AIR_LOAD, 2);
        load->ct = type_copy(insn->ct);
        load->ops[0] = air_insn_register_operand_init(resreg);
        load->ops[1] = air_insn_register_operand_init(X86R_XMM0);
        air_insn_insert_after(load, insn);
        free(classes);
        return;
    }
    // TODO: all the x87 stuff

    // handling struct returns <16 bytes in width
    if (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION)
        report_return;
    
    long long size = type_size(ct);

    if (size > 16)
        report_return;

    symbol_t* lv = symbol_table_add(SYMBOL_TABLE, "__anonymous_lv__", symbol_init(NULL));
    lv->type = type_copy(ct);

    air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
    decl->ops[0] = air_insn_symbol_operand_init(lv);
    air_insn_t* pos = air_insn_insert_after(decl, insn);

    air_insn_t* loadaddr = air_insn_init(AIR_LOAD_ADDR, 2);
    loadaddr->ct = make_reference_type(ct);
    loadaddr->ops[0] = air_insn_register_operand_init(resreg);
    loadaddr->ops[1] = air_insn_symbol_operand_init(lv);
    pos = air_insn_insert_after(loadaddr, pos);
    
    for (size_t i = 0; i < ccount; ++i)
    {
        long long total_remaining = size - (i * 8);
        arg_class_t class = classes[i];
        if (class == ARG_INTEGER)
        {
            long long to_be_copied = min(total_remaining, 8);
            for (long long copied = 0; copied < to_be_copied;)
            {
                long long remaining = to_be_copied - copied;
                c_type_class_t class = CTC_UNSIGNED_LONG_LONG_INT;
                if (remaining < UNSIGNED_SHORT_INT_WIDTH)
                    class = CTC_UNSIGNED_CHAR;
                else if (remaining < UNSIGNED_INT_WIDTH)
                    class = CTC_UNSIGNED_SHORT_INT;
                else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
                    class = CTC_UNSIGNED_INT;
                long long shift = (8 - remaining) * 8;
                if (shift)
                {
                    air_insn_t* shr = air_insn_init(AIR_DIRECT_SHIFT_RIGHT, 2);
                    shr->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                    shr->ops[0] = air_insn_register_operand_init(integer_return_sequence[next_intretreg]);
                    shr->ops[1] = air_insn_integer_constant_operand_init(shift);
                    pos = air_insn_insert_after(shr, pos);
                }
                air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
                assign->ct = make_basic_type(class);
                long long csize = type_size(assign->ct);
                assign->ops[0] = air_insn_indirect_register_operand_init(resreg, (i * 8) + remaining - csize, INVALID_VREGID, 1);
                assign->ops[1] = air_insn_register_operand_init(integer_return_sequence[next_intretreg]);
                pos = air_insn_insert_after(assign, pos);
                copied += csize;
            }
            ++next_intretreg;
        }
        else
            // TODO handle SSE, X87, etc.
            report_return;
    }
}

// inserts necessary System V ABI loads and stores around the call site
void localize_x86_64_func_call(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    localize_x86_64_func_call_return(insn, routine, air);

    if (insn->noops > 2)
        localize_x86_64_func_call_args(insn, routine, air);
}

/*

integer _1 = ...;
%rax = _1;
return;

sse _1 = ...;
%xmm0 = _1;
return;

struct point* _1 = &p;
return _1;

turned into...

<=16-byte struct

struct point* _1 = &p;
%rax = *_1;
%rdx = *(_1 + 8);
return;

>16-byte struct

struct point* _1 = &p;
__sret__ = *_1;
__sret__ + 8 = *(_1 + 8);
...
return;

*/

/*

int _3 = _1 % _2;

becomes:

%eax = _1;
%edx = 0;
%edx = %edx:%eax / _2;
int _3 = %edx;

(integer type only)
int _3 = _1 / _2;

becomes:

%eax = _1;
%edx = 0;
%eax = %edx:%eax / _2;
int _3 = %eax;

*/
void localize_x86_64_divide_modulo(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    // localization only applies to integer division
    if (insn->type == AIR_DIVIDE && !type_is_integer(insn->ct))
        return;
    regid_t hresultreg = insn->type == AIR_DIVIDE ? X86R_RAX : X86R_RDX;
    if (insn->ops[0]->type != AOP_REGISTER) report_return;
    if (insn->ops[1]->type != AOP_REGISTER) report_return;
    air_insn_t* assign_top = air_insn_init(AIR_ASSIGN, 2);
    assign_top->ct = type_copy(insn->ct);
    assign_top->ops[0] = air_insn_register_operand_init(X86R_RAX);
    assign_top->ops[1] = air_insn_register_operand_init(insn->ops[1]->content.reg);
    air_insn_insert_before(assign_top, insn);
    air_insn_t* zero_rdx = air_insn_init(AIR_ASSIGN, 2);
    zero_rdx->ct = type_copy(insn->ct);
    zero_rdx->ops[0] = air_insn_register_operand_init(X86R_RDX);
    zero_rdx->ops[1] = air_insn_integer_constant_operand_init(0);
    air_insn_insert_before(zero_rdx, insn);
    regid_t resultreg = insn->ops[0]->content.reg;
    insn->ops[0]->content.reg = hresultreg;
    insn->ops[1]->content.reg = INVALID_VREGID;
    air_insn_t* load_result = air_insn_init(AIR_LOAD, 2);
    load_result->ct = type_copy(insn->ct);
    load_result->ops[0] = air_insn_register_operand_init(resultreg);
    load_result->ops[1] = air_insn_register_operand_init(hresultreg);
    air_insn_insert_after(load_result, insn);
    insn->type = AIR_DIVIDE;
}

// handles return values w/ respect to the System V ABI
void localize_x86_64_return(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    warnf("return statements have not been localized for an x86 target, they will not behave as expected\n");
}

// assigns parameter locals
void localize_x86_64_routine_before(air_routine_t* routine, air_t* air)
{
    c_type_t* ftype = routine->sy->type;
    c_type_t* rettype = ftype->derived_from;

    regid_t nextintreg = X86R_RDI;
    regid_t nextssereg = X86R_XMM0;
    long long nexteightbyteoffset = 16;

    air_insn_t* inserting = routine->insns;

    if ((rettype->class == CTC_STRUCTURE || rettype->class == CTC_UNION) && type_size(rettype) > 16)
    {
        /*
        struct point* __anonymous_lv__;
        __anonymous_lv__ = %rdi;
        */
       symbol_t* sy = symbol_table_add(air->st, "__anonymous_lv__", symbol_init(NULL));
       sy->type = make_reference_type(rettype);

       air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
       decl->ops[0] = air_insn_symbol_operand_init(sy);

       air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
       assign->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
       assign->ops[0] = air_insn_symbol_operand_init(sy);
       assign->ops[1] = air_insn_register_operand_init(nextintreg++);

       inserting = air_insn_insert_after(assign, inserting);
       inserting = air_insn_insert_after(decl, inserting);

       routine->retptr = sy;
    }

    syntax_component_t* fdeclr = syntax_get_full_declarator(routine->sy->declarer);

    if (!fdeclr) report_return;

    if (!fdeclr->fdeclr_parameter_declarations)
        // TODO: need to handle K&R functions (maybe?)
        report_return;

    VECTOR_FOR(syntax_component_t*, pdecl, fdeclr->fdeclr_parameter_declarations)
    {
        syntax_component_t* pid = syntax_get_declarator_identifier(pdecl->pdecl_declr);
        if (!pid) continue;
        symbol_t* psy = symbol_table_get_syn_id(air->st, pid);
        c_type_t* pt = vector_get(ftype->function.param_types, i);

        size_t ccount = 0;
        arg_class_t* classes = find_classes(pt, &ccount);
        free(classes);

        /*
        
        void f(struct point{16} p);

        p = %rdi;
        p + 8 = %rsi;

        void f(struct point{14} p);

        p = %rdi;
        %rsi >>>= 16;
        p + 8 = %esi;

        void f(struct point{20} p);

        unsigned long long int _1 = *(%rbp + 16);
        p = _1;
        unsigned long long int _2 = *(%rbp + 24);
        p + 8 = _2;
        unsigned int _3 = *(%rbp + 32);
        p + 16 = _3;

        void f(integer i);
        i = %rdi;

        void f(fp f);
        f = %xmm0;
        
        */

        if (type_is_integer(pt) || pt->class == CTC_POINTER || pt->class == CTC_ARRAY)
        {
            if (ccount > 1) report_return;
            bool noreg = nextintreg > X86R_R9;
            regid_t reg = noreg ? NEXT_VIRTUAL_REGISTER : nextintreg++;
            if (noreg)
            {
                air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
                insn->ct = pt->class == CTC_ARRAY ? make_reference_type(pt) : type_copy(pt);
                insn->ops[0] = air_insn_register_operand_init(reg);
                insn->ops[1] = air_insn_indirect_register_operand_init(X86R_RBP, nexteightbyteoffset, INVALID_VREGID, 1);
                inserting = air_insn_insert_after(insn, inserting);
                nexteightbyteoffset += 8;
            }
            air_insn_t* insn = air_insn_init(AIR_ASSIGN, 2);
            insn->ct = pt->class == CTC_ARRAY ? make_reference_type(pt) : type_copy(pt);
            insn->ops[0] = air_insn_symbol_operand_init(psy);
            insn->ops[1] = air_insn_register_operand_init(reg);
            inserting = air_insn_insert_after(insn, inserting);
            break;
        }
        
        if (type_is_real_floating(pt))
        {
            if (ccount > 1) report_return;
            bool noreg = nextintreg > X86R_XMM7;
            regid_t reg = noreg ? NEXT_VIRTUAL_REGISTER : nextssereg++;
            if (noreg)
            {
                air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
                insn->ct = type_copy(pt);
                insn->ops[0] = air_insn_register_operand_init(reg);
                insn->ops[1] = air_insn_indirect_register_operand_init(X86R_RBP, nexteightbyteoffset, INVALID_VREGID, 1);
                inserting = air_insn_insert_after(insn, inserting);
                nexteightbyteoffset += 8;
            }
            air_insn_t* insn = air_insn_init(AIR_ASSIGN, 2);
            insn->ct = type_copy(pt);
            insn->ops[0] = air_insn_symbol_operand_init(psy);
            insn->ops[1] = air_insn_register_operand_init(reg);
            inserting = air_insn_insert_after(insn, inserting);
            break;
        }

        if (pt->class != CTC_STRUCTURE && pt->class != CTC_UNION)
            report_return;
        
        long long psize = type_size(pt);
        bool vreg = psize > 16 || nextintreg > X86R_R9;
        regid_t reg = vreg ? NEXT_VIRTUAL_REGISTER : nextintreg++;
        for (long long copied = 0; copied < psize;)
        {
            long long remaining = psize - copied;
            c_type_class_t class = CTC_UNSIGNED_LONG_LONG_INT;
            if (remaining < UNSIGNED_SHORT_INT_WIDTH)
                class = CTC_UNSIGNED_CHAR;
            else if (remaining < UNSIGNED_INT_WIDTH)
                class = CTC_UNSIGNED_SHORT_INT;
            else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
                class = CTC_UNSIGNED_INT;
            if (remaining < 8 && !vreg)
            {
                air_insn_t* shr = air_insn_init(AIR_DIRECT_SHIFT_RIGHT, 2);
                shr->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shr->ops[0] = air_insn_register_operand_init(reg);
                shr->ops[1] = air_insn_integer_constant_operand_init((8 - type_size(shr->ct)) * 8);
                inserting = air_insn_insert_after(shr, inserting);
            }
            if (vreg)
            {
                air_insn_t* load = air_insn_init(AIR_LOAD, 2);
                load->ct = make_basic_type(class);
                load->ops[0] = air_insn_register_operand_init(reg);
                load->ops[1] = air_insn_indirect_register_operand_init(X86R_RBP, nexteightbyteoffset + copied, INVALID_VREGID, 1);
                inserting = air_insn_insert_after(load, inserting);
            }
            air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
            assign->ct = make_basic_type(class);
            assign->ops[0] = air_insn_indirect_symbol_operand_init(psy, copied);
            assign->ops[1] = air_insn_register_operand_init(reg);
            inserting = air_insn_insert_after(assign, inserting);
            copied += type_size(assign->ct);
        }
        // round up to next eight byte boundary if not there
        // (8 - (24 % 8) % 8)
        nexteightbyteoffset = nexteightbyteoffset + (8 - (nexteightbyteoffset % 8) % 8);
    }
}

void localize_x86_64(air_t* air)
{
    VECTOR_FOR(air_routine_t*, routine, air->routines)
    {
        localize_x86_64_routine_before(routine, air);
        for (air_insn_t* insn = routine->insns; insn; insn = insn->next)
        {
            switch (insn->type)
            {
                case AIR_FUNC_CALL:
                    localize_x86_64_func_call(insn, routine, air);
                    break;
                case AIR_RETURN:
                    localize_x86_64_return(insn, routine, air);
                    break;
                case AIR_DIVIDE:
                case AIR_MODULO:
                    localize_x86_64_divide_modulo(insn, routine, air);
                    break;
                default:
                    break;
            }
        }
    }
}

void remove_phi_instructions(air_t* air)
{
    map_t* map = map_init((comparator_t) regid_comparator, (hash_function_t) regid_hash);

    VECTOR_FOR(air_routine_t*, routine, air->routines)
    {
        air_insn_t* last = routine->insns;
        for (; last && last->next; last = last->next);

        for (air_insn_t* insn = last; insn;)
        {
            if (insn->type == AIR_PHI)
            {
                air_insn_operand_t* op1 = insn->ops[0];
                if (!op1 || op1->type != AOP_REGISTER) report_return;
                regid_t reg = op1->content.reg;

                for (int i = 1; i < insn->noops; ++i)
                {
                    air_insn_operand_t* op = insn->ops[i];
                    if (!op || op->type != AOP_REGISTER) report_return;
                    regid_t opreg = op->content.reg;

                    map_add(map, (void*) opreg, (void*) reg);
                }

                insn = air_insn_remove(insn);
                continue;
            }

            for (int i = 0; i < insn->noops; ++i)
            {
                air_insn_operand_t* op = insn->ops[i];
                if (!op) continue;

                regid_t reg = INVALID_VREGID;
                if (op->type == AOP_REGISTER)
                    reg = op->content.reg;
                else if (op->type == AOP_INDIRECT_REGISTER)
                    reg = op->content.inreg.id;
                else
                    continue;
                
                regid_t alt = (regid_t) map_get(map, (void*) reg);
                if (alt == INVALID_VREGID)
                    continue;
                
                if (op->type == AOP_REGISTER)
                    op->content.reg = alt;
                else if (op->type == AOP_INDIRECT_REGISTER)
                    op->content.inreg.id = alt;
            }

            insn = insn->prev;
        }
    }

    map_delete(map);
}

// "localize" an AIR instance to a particular architecture
void localize(air_t* air, air_locale_t locale)
{
    remove_phi_instructions(air);

    air->locale = locale;
    switch (locale)
    {
        case LOC_X86_64:
            localize_x86_64(air);
            break;
        default:
            report_return;
    }
}
