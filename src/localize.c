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

static c_type_class_t largest_type_class_for_eightbyte(long long remaining)
{
    if (remaining < UNSIGNED_SHORT_INT_WIDTH)
        return CTC_UNSIGNED_CHAR;
    else if (remaining < UNSIGNED_INT_WIDTH)
        return CTC_UNSIGNED_SHORT_INT;
    else if (remaining < UNSIGNED_LONG_LONG_INT_WIDTH)
        return CTC_UNSIGNED_INT;
    else
        return CTC_UNSIGNED_LONG_LONG_INT;
}

static c_type_class_t largest_sse_type_class_for_eightbyte(long long remaining)
{
    if (remaining == FLOAT_WIDTH)
        return CTC_FLOAT;
    if (remaining == DOUBLE_WIDTH)
        return CTC_DOUBLE;
    report_return_value(CTC_ERROR);
}

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
    // if the argument is an integer, it only has one class, INTEGER
    if (type_is_integer(ct))
        return arg_class_singleton(ARG_INTEGER);
    
    // shouldn't have array types as args (they get implicitly converted to ptrs) but whatever
    // pointers also have the INTEGER class
    if (ct->class == CTC_POINTER || ct->class == CTC_ARRAY)
        return arg_class_singleton(ARG_INTEGER);
    
    // floats and doubles are SSE
    // float _Complex is treated as a struct of two floats, in which the class becomes SSE
    if (ct->class == CTC_FLOAT || ct->class == CTC_DOUBLE || ct->class == CTC_FLOAT_COMPLEX)
        return arg_class_singleton(ARG_SSE);

    if (count) *count = 2;

    // the lower byte of the long double is classified as X87
    // the higher byte is classified as X87UP
    if (ct->class == CTC_LONG_DOUBLE)
    {
        arg_class_t* r = calloc(2, sizeof *r);
        r[0] = ARG_X87;
        r[1] = ARG_X87UP;
        return r;
    }

    // both bytes get classified as SSE for double _Complex because it's treated as as struct of two doubles
    if (ct->class == CTC_DOUBLE_COMPLEX)
    {
        arg_class_t* r = calloc(2, sizeof *r);
        r[0] = r[1] = ARG_SSE;
        return r;
    }

    if (count) *count = 4;

    // long double _Complex has all four of its classes (32 bytes) set to COMPLEX_X87
    if (ct->class == CTC_LONG_DOUBLE_COMPLEX)
    {
        arg_class_t* r = calloc(4, sizeof *r);
        for (size_t i = 0; i < 4; ++i)
            r[i] = ARG_COMPLEX_X87;
        return r;
    }

    // beyond this, we have an aggregate or union type
    return find_aggregate_union_classes(ct, count);
}

static arg_class_t* find_aggregate_union_classes(c_type_t* ct, size_t* count)
{
    // get rid of any bad cookies
    // this should actually probably get rid of arrays too
    // because the function doesn't support it but eh, it's
    // not like arrays can even be passed as params in C
    if (!ct) return NULL;
    if (ct->class != CTC_ARRAY && ct->class != CTC_STRUCTURE && ct->class != CTC_UNION) report_return_value(NULL);

    // get the size of the aggregate/union
    long long size = type_size(ct);

    // the number of classes this type will have is the amount
    // of eightbytes necessary to hold the object, rounded up
    // to the nearest eightbyte
    *count = (size + (8 - (size % 8)) % 8) >> 3;

    arg_class_t* classes = calloc(*count, sizeof *classes);

    // if the aggregate/union is over 8 eightbytes wide, all
    // of its eightbytes get classified as MEMORY
    if (size > 64)
    {
        for (size_t i = 0; i < *count; ++i)
            classes[i] = ARG_MEMORY;
        return classes;
    }

    // all eightbytes are initialized with NO_CLASS
    for (size_t i = 0; i < *count; ++i)
        classes[i] = ARG_NO_CLASS;

    // offset (in bytes) of where we are within the struct/union
    long long offset = 0;

    // go thru all member types
    VECTOR_FOR(c_type_t*, mt, ct->struct_union.member_types)
    {
        // get their sizes and alignments
        long long ms = type_size(mt); 
        long long ma = type_alignment(mt);

        // jump to the member's alignment, if necessary
        offset += (ma - (size % ma)) % ma;

        // collect the classes of this member's type
        size_t subclasses_count = 0;
        arg_class_t* subclasses = find_classes(mt, &subclasses_count);

        // go thru all 'em
        for (size_t j = 0; j < subclasses_count; ++j)
        {
            // find the class within the whole struct/union type which
            // our current subclass (the "new" class) would apply to
            long long class_idx = (offset >> 3) + j;
            arg_class_t class = classes[class_idx];
            arg_class_t subclass = subclasses[j];
            // then we begin to compare the subclass with the current class
            // we have for this eightbyte

            // ignore if they're the same
            if (class == subclass)
                continue;
            
            // ignore if the new class type is NO_CLASS
            if (subclass == ARG_NO_CLASS)
                continue;
            
            // if we currently have NO_CLASS, take whatever the subclass is
            // if the new class is MEMORY or INTEGER, take that class
            if (class == ARG_NO_CLASS || subclass == ARG_MEMORY || subclass == ARG_INTEGER)
                classes[class_idx] = subclass;
            // if the anything is an X87 class, put it in memory
            else if (class == ARG_X87 ||
                class == ARG_X87UP ||
                class == ARG_COMPLEX_X87 ||
                subclass == ARG_X87 ||
                subclass == ARG_X87UP || 
                subclass == ARG_COMPLEX_X87)
                classes[class_idx] = ARG_MEMORY;
            else
            // otherwise, make it SSE
                classes[class_idx] = ARG_SSE;
        }
        free(subclasses);

        // jump the offset to after the type
        offset += ms;
    }

    // "post-merger" from the ABI:
    // if any of the classes are MEMORY, everything becomes MEMORY
    // if the X87UP does not have an X87 before it, everything becomes MEMORY
    // if the type's size is >16 bytes and the first class isn't SSE or any other class isn't SSEUP, everything becomes MEMORY
    // if an SSEUP does not have an SSE or SSEUP before it, it becomes SSE

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

takes an instruction like:
    int _1 = 5 + 3;
and if we want to extract the '3', it would convert it to:
    int _2 = 3;
    int _1 = 5 + _2;

*/
static regid_t try_extract_integer_constant(air_insn_t* insn, air_t* air, size_t index, c_type_t* ct)
{
    if (index >= insn->noops)
        return INVALID_VREGID;
    air_insn_operand_t* op = insn->ops[index];
    if (!op || op->type != AOP_INTEGER_CONSTANT)
        return INVALID_VREGID;
    unsigned long long value = op->content.ic;
    
    regid_t reg = NEXT_VIRTUAL_REGISTER;

    air_insn_t* def = air_insn_init(AIR_LOAD, 2);
    def->ct = type_copy(ct);
    def->ops[0] = air_insn_register_operand_init(reg);
    def->ops[1] = air_insn_integer_constant_operand_init(value);
    air_insn_insert_before(def, insn);

    op->type = AOP_REGISTER;
    op->content.reg = reg;
    return reg;
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
    // get the argument register, this is where we are going to be copying data from
    regid_t argreg = tempdef->ops[0]->type == AOP_REGISTER ? tempdef->ops[0]->content.reg : tempdef->ops[0]->content.inreg.id;

    // if the type of the argument is struct/union
    if (ct->class == CTC_STRUCTURE || ct->class == CTC_UNION)
    {
        // get its type
        long long size = type_size(ct);

        // progress begins at the start of the eightbyte we're storing
        size_t progress = eightbyte_idx << 3;

        // if the amount of bytes remaining in the entire argument is greater than 8,
        // then just push those eight bytes onto the stack
        long long total_remaining = size - progress;
        if (total_remaining >= UNSIGNED_LONG_LONG_INT_WIDTH)
        {
            air_insn_t* push = air_insn_init(AIR_PUSH, 1);
            push->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
            push->ops[0] = air_insn_indirect_register_operand_init(argreg, progress, INVALID_VREGID, 1);
            air_insn_insert_before(push, loc);
            return push;
        }

        // if the amount of bytes remaining in the entire argument is less than 8,
        // then we gotta do this garbage
        // (i.e., load a temporary with the remaining bytes)

        air_insn_t* first = NULL;

        // create a temporary with zero as its value

        regid_t tmpreg = NEXT_VIRTUAL_REGISTER;

        air_insn_t* init = air_insn_init(AIR_LOAD, 2);
        init->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        init->ops[0] = air_insn_register_operand_init(tmpreg);
        init->ops[1] = air_insn_integer_constant_operand_init(0);
        air_insn_insert_before(init, loc);

        first = init;

        // continue to take the largest amount of bytes possible
        // shift and or the temporary with the values loaded
        for (long long copied = 0; copied < total_remaining;)
        {
            // remaining of the stuff we've already copied
            long long remaining = total_remaining - copied;

            // create the type that can fit the largest number of bytes we still need to copy
            c_type_t* cyt = make_basic_type(largest_type_class_for_eightbyte(remaining));
            long long cytsize = type_size(cyt);

            // if this isn't the first time adding to the temporary,
            // we need to shift the bits over by the bit size of the type
            // we just made
            if (copied)
            {
                air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
                shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shl->ops[0] = air_insn_register_operand_init(tmpreg);
                shl->ops[1] = air_insn_integer_constant_operand_init(type_size(cyt) << 3);
                air_insn_insert_before(shl, loc);
            }

            // then or the bytes we're copying with the temporary
            air_insn_t* or = air_insn_init(AIR_DIRECT_OR, 2);
            or->ct = cyt;
            or->ops[0] = air_insn_register_operand_init(tmpreg);
            or->ops[1] = air_insn_indirect_register_operand_init(argreg, progress + (remaining - cytsize), INVALID_VREGID, 1);
            air_insn_insert_before(or, loc);

            // move copied forwad
            copied += type_size(cyt);
        }

        // then push the temporary
        air_insn_t* push = air_insn_init(AIR_PUSH, 1);
        push->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        push->ops[0] = air_insn_register_operand_init(tmpreg);
        air_insn_insert_before(push, loc);

        return first;
    }

    // look how easy it is when we don't have to do dumb garbage!
    // just a quick push onto the stack!
    air_insn_t* push = air_insn_init(AIR_PUSH, 1);
    push->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    push->ops[0] = air_insn_register_operand_init(argreg);
    air_insn_insert_before(push, loc);
    return push;
}

// loc is the location to insert the instructions ABOVE
// tempdef is where the temporary was defined that holds the argument
// ct is the type of the argument
// eightbyte_idx is this eightbyte's index in the current sequence of eightbyte classifications
// dest is the register that the eightbyte will get loaded into
// return is the first instruction inserted
air_insn_t* store_eightbyte_in_register(air_insn_t* loc, air_insn_t* tempdef, c_type_t* ct, size_t eightbyte_idx, regid_t dest, air_t* air)
{
    // this is where we copying data from
    regid_t argreg = tempdef->ops[0]->type == AOP_REGISTER ? tempdef->ops[0]->content.reg : tempdef->ops[0]->content.inreg.id;

    // if it's a struct/union
    if (ct->class == CTC_STRUCTURE || ct->class == CTC_UNION)
    {
        // get the type
        long long size = type_size(ct);

        // get the progress of loading the entire struct/union, across eightbytes
        size_t progress = eightbyte_idx << 3;

        // then get the total remaining from that progress
        long long total_remaining = size - progress;

        // if there's over an eightbyte left, let's just copy 8 bytes from the source argument
        if (total_remaining >= UNSIGNED_LONG_LONG_INT_WIDTH)
        {
            air_insn_t* deref = air_insn_init(AIR_LOAD, 2);
            deref->ct = make_basic_type(x86_64_is_sse_register(dest) ? CTC_DOUBLE : CTC_UNSIGNED_LONG_LONG_INT);
            deref->ops[0] = air_insn_register_operand_init(dest);
            deref->ops[1] = air_insn_indirect_register_operand_init(argreg, progress, INVALID_VREGID, 1);
            air_insn_insert_before(deref, loc);
            return deref;
        }

        // otherwise, we gotta do the same garbage we did above,
        // copy the largest amount of bytes we can repeatedly until
        // we've copied the entire rest of the struct/union

        air_insn_t* first = NULL;
        for (long long copied = 0; copied < total_remaining;)
        {
            long long remaining = total_remaining - copied;

            // find the type that can fit the largest number of bytes
            // for the amount remaining
            c_type_t* cyt = make_basic_type(x86_64_is_sse_register(dest) ?
                largest_sse_type_class_for_eightbyte(remaining) : largest_type_class_for_eightbyte(remaining));
            long long cytsize = type_size(cyt);

            // shift if we need to make space for the new data we copying
            if (copied)
            {
                air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
                shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shl->ops[0] = air_insn_register_operand_init(dest);
                shl->ops[1] = air_insn_integer_constant_operand_init(type_size(cyt) << 3);
                air_insn_insert_before(shl, loc);
            }

            // skraight load those bytes into the register
            // we can do it with a load this time instead of an OR
            // because of how x86's <64-bit registers are just alias for the lower bytes of the 64-bit version
            air_insn_t* assign = air_insn_init(AIR_LOAD, 2);
            assign->ct = cyt;
            assign->ops[0] = air_insn_register_operand_init(dest);
            assign->ops[1] = air_insn_indirect_register_operand_init(argreg, progress + (remaining - cytsize), INVALID_VREGID, 1);
            air_insn_insert_before(assign, loc);

            if (!first) first = assign;

            // move it forwad
            copied += type_size(cyt);
        }

        return first;
    }

    // if there's no garbage, we just do a skraight load
    air_insn_t* assign = air_insn_init(AIR_LOAD, 2);
    assign->ct = type_copy(ct);
    assign->ops[0] = air_insn_register_operand_init(dest);
    assign->ops[1] = air_insn_register_operand_init(argreg);
    air_insn_insert_before(assign, loc);
    return assign;
}

// guyyyyy this sucks (it really doesn't)
void localize_x86_64_func_call_args(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    // ignore if there's no arguments
    if (insn->noops <= 2) return;

    // initialize the next integer register in the sequence
    // %rdi normally, %rsi if %rdi is used as the return value for a struct
    regid_t nextintreg = X86R_RDI;
    bool rdi_ret = insn->metadata.fcall_sret && type_size(insn->ct->derived_from) > 16;
    if (rdi_ret)
        nextintreg = X86R_RSI;

    // initialize the next SSE register in the sequence
    regid_t nextssereg = X86R_XMM0;

    air_insn_t* inserting = insn;

    // loop thru every argument to the function call
    for (size_t i = 2; i < insn->noops; ++i)
    {
        air_insn_operand_t* op = insn->ops[i];
        // report any funky operands (not a register)
        if (op->type != AOP_REGISTER && op->type != AOP_INDIRECT_REGISTER) report_return;

        // find the definition for the temporary used here as an argument
        air_insn_t* tempdef = air_insn_find_temporary_definition_above(op->type == AOP_REGISTER ? op->content.reg : op->content.inreg.id, insn);
        if (!tempdef) report_return;

        // so that we can get its type
        c_type_t* at = tempdef->ct;
        if (op->type == AOP_INDIRECT_REGISTER)
            at = at->derived_from;
        
        // get the ABI classes for this argument
        size_t ccount = 0;
        arg_class_t* classes = find_classes(at, &ccount);
        if (!classes) report_return;

        // for each eightbyte/class, store it in a location
        for (size_t i = 0; i < ccount; ++i)
        {
            arg_class_t class = classes[i];
            // if it's X87, MEMORY, or there aren't any more integer or SSE registers, store it on the stack
            if (class == ARG_MEMORY ||
                class == ARG_X87 ||
                class == ARG_X87UP ||
                class == ARG_COMPLEX_X87 ||
                (class == ARG_INTEGER && nextintreg > X86R_R9) ||
                (class == ARG_SSE && nextssereg > X86R_XMM7))
                inserting = store_eightbyte_on_stack(inserting, tempdef, at, i, air);
            // if it has the INTEGER class, store it in an integer register
            else if (class == ARG_INTEGER)
                inserting = store_eightbyte_in_register(inserting, tempdef, at, i, nextintreg++, air);
            // if it has the SSE class, store it in an SSE register
            else if (class == ARG_SSE)
                inserting = store_eightbyte_in_register(inserting, tempdef, at, i, nextssereg++, air);
            // if it has the SSEUP class, store it in the previously used SSE register
            else if (class == ARG_SSEUP)
                inserting = store_eightbyte_in_register(inserting, tempdef, at, i, nextssereg - 1, air);
        }

        free(classes);
    }

    // if we got a struct returned thru rdi
    if (rdi_ret)
    {
        // create and declare a local variable of the struct type
        symbol_t* sy = symbol_table_add(SYMBOL_TABLE, "__anonymous_lv__", symbol_init(NULL));
        sy->type = type_copy(insn->ct->derived_from);

        air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
        decl->ops[0] = air_insn_symbol_operand_init(sy);
        air_insn_insert_before(decl, insn);

        // and then give the address of that local variable to %rdi
        air_insn_t* loadaddr = air_insn_init(AIR_LOAD_ADDR, 2);
        loadaddr->ct = type_copy(insn->ct);
        loadaddr->ops[0] = air_insn_register_operand_init(X86R_RDI);
        loadaddr->ops[1] = air_insn_symbol_operand_init(sy);
        air_insn_insert_before(loadaddr, insn);
    }

    // TODO: check to see if the function has no prototype or varargs
    // if (nextssereg - X86R_XMM0 > 0)
    // {
    //     air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
    //     assign->ct = make_basic_type(CTC_UNSIGNED_CHAR);
    //     assign->ops[0] = air_insn_register_operand_init(X86R_RAX);
    //     assign->ops[1] = air_insn_integer_constant_operand_init(nextssereg - X86R_XMM0);
    //     air_insn_insert_before(assign, insn);
    // }

    // delete all the old temporary usages in the original call instruction
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
    unsigned long long int %rax, %rdi, %rsi, %rdx, %rcx, %r8, %r9, %r10, %r11, %rsp;
    double %xmm0, %xmm1, %xmm2, %xmm3, %xmm4, %xmm5, %xmm6, %xmm7;
    deref type __lv0;
    type _1 = &__lv0;
    *_1 = %rax;

*/

static air_insn_t* blip_volatiles_after(air_insn_t* insn)
{
    static const regid_t volatile_integer_registers[] = {
        X86R_RAX,
        X86R_RDI,
        X86R_RSI,
        X86R_RDX,
        X86R_RCX,
        X86R_R8,
        X86R_R9,
        X86R_R10,
        X86R_R11,
        X86R_RSP
    };

    static const regid_t volatile_sse_registers[] = {
        X86R_XMM0,
        X86R_XMM1,
        X86R_XMM2,
        X86R_XMM3,
        X86R_XMM4,
        X86R_XMM5,
        X86R_XMM6,
        X86R_XMM7
    };

    air_insn_t* pos = insn;

    for (size_t i = 0; i < sizeof(volatile_integer_registers) / sizeof(volatile_integer_registers[0]); ++i)
    {
        regid_t reg = volatile_integer_registers[i];
        air_insn_t* decl = air_insn_init(AIR_BLIP, 1);
        decl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        decl->ops[0] = air_insn_register_operand_init(reg);
        pos = air_insn_insert_after(decl, pos);
    }

    for (size_t i = 0; i < sizeof(volatile_sse_registers) / sizeof(volatile_sse_registers[0]); ++i)
    {
        regid_t reg = volatile_sse_registers[i];
        air_insn_t* decl = air_insn_init(AIR_BLIP, 1);
        decl->ct = make_basic_type(CTC_DOUBLE);
        decl->ops[0] = air_insn_register_operand_init(reg);
        pos = air_insn_insert_after(decl, pos);
    }

    return pos;
}

void localize_x86_64_func_call_return(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (insn->ops[0]->type != AOP_REGISTER) report_return;

    // initialize the integer register return sequence
    regid_t integer_return_sequence[] = { X86R_RAX, X86R_RDX };
    size_t next_intretreg = 0;
    regid_t next_sseretreg = X86R_XMM0;

    // get the register where the result of the function call should end up in
    regid_t resreg = insn->ops[0]->content.reg;

    // remove it from the call instruction itself
    air_insn_operand_delete(insn->ops[0]);
    insn->ops[0] = air_insn_register_operand_init(INVALID_VREGID);

    // get the return type of the function call
    c_type_t* ct = insn->ct;
    if (insn->metadata.fcall_sret)
        ct = ct->derived_from;

    // get the ABI classes of the return type
    size_t ccount = 0;
    arg_class_t* classes = find_classes(ct, &ccount);
    if (!classes) report_return;

    air_insn_t* pos = blip_volatiles_after(insn);

    // if there is a single class and it's INTEGER,
    // then the return value is in %rax, so pull it into the result register

    // if the classes are MEMORY,
    // then the return value is a ptr that was passed in as %rdi initially
    // now that ptr's value is in %rax, so pull it into the resulting register
    if (classes[0] == ARG_MEMORY ||
        (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION && classes[0] == ARG_INTEGER))
    {
        // load the actual value
        air_insn_t* load = air_insn_init(AIR_LOAD, 2);
        load->ct = type_copy(insn->ct);
        load->ops[0] = air_insn_register_operand_init(resreg);
        load->ops[1] = air_insn_register_operand_init(X86R_RAX);
        pos = air_insn_insert_after(load, pos);
        free(classes);
        return;
    }

    // if there is a single class and it's SSE,
    // then the return value is in %xmm0, so pull it into the result register
    if (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION && classes[0] == ARG_SSE)
    {
        // load the actual value
        air_insn_t* load = air_insn_init(AIR_LOAD, 2);
        load->ct = type_copy(insn->ct);
        load->ops[0] = air_insn_register_operand_init(resreg);
        load->ops[1] = air_insn_register_operand_init(X86R_XMM0);
        pos = air_insn_insert_after(load, pos);
        free(classes);
        return;
    }
    // TODO: all the x87 stuff

    // beyond here handles struct/union returns that are <16 bytes in width
    if (ct->class != CTC_STRUCTURE && ct->class != CTC_UNION)
        report_return;
    
    long long size = type_size(ct);

    // bad, obviously
    if (size > 16)
        report_return;

    // create and declare a local variable to load the struct data into
    symbol_t* lv = symbol_table_add(SYMBOL_TABLE, "__anonymous_lv__", symbol_init(NULL));
    lv->type = type_copy(ct);

    air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
    decl->ops[0] = air_insn_symbol_operand_init(lv);

    pos = air_insn_insert_after(decl, pos);

    // load the address of the local variable we just created
    air_insn_t* loadaddr = air_insn_init(AIR_LOAD_ADDR, 2);
    loadaddr->ct = make_reference_type(ct);
    loadaddr->ops[0] = air_insn_register_operand_init(resreg);
    loadaddr->ops[1] = air_insn_symbol_operand_init(lv);
    pos = air_insn_insert_after(loadaddr, pos);
    
    // go thru all the classes/eightbytes of the return value
    for (size_t i = 0; i < ccount; ++i)
    {
        // keep track of how many bytes we got left
        long long total_remaining = size - (i * 8);

        arg_class_t class = classes[i];

        // if the class is INTEGER or SSE
        if (class == ARG_INTEGER || class == ARG_SSE)
        {
            // copy at most 8 bytes at a time
            long long to_be_copied = min(total_remaining, 8);
            for (long long copied = 0; copied < to_be_copied;)
            {
                long long remaining = to_be_copied - copied;
                regid_t reg = class == ARG_INTEGER ? integer_return_sequence[next_intretreg] : next_sseretreg;

                // keep on loading!
                air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
                assign->ct = make_basic_type(class == ARG_INTEGER ? largest_type_class_for_eightbyte(remaining) :
                    largest_sse_type_class_for_eightbyte(remaining));
                long long csize = type_size(assign->ct);
                assign->ops[0] = air_insn_indirect_register_operand_init(resreg, (i * 8) + copied, INVALID_VREGID, 1);
                assign->ops[1] = air_insn_register_operand_init(reg);
                pos = air_insn_insert_after(assign, pos);

                copied += csize;

                if (copied >= to_be_copied)
                    continue;

                air_insn_t* shr = air_insn_init(AIR_DIRECT_SHIFT_RIGHT, 2);
                shr->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shr->ops[0] = air_insn_register_operand_init(reg);
                shr->ops[1] = air_insn_integer_constant_operand_init(csize << 3);
                pos = air_insn_insert_after(shr, pos);
            }
            // move up the register
            if (class == ARG_INTEGER)
                ++next_intretreg;
            else if (class == ARG_SSE)
                ++next_sseretreg;
        }
        else
            // TODO: handle long doubles and complex numberss
            report_return;
    }
}

// inserts necessary System V ABI loads and stores around the call site
void localize_x86_64_func_call(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    localize_x86_64_func_call_return(insn, routine, air);
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

int %eax = _1;
int %edx = 0;
%eax = %edx:%eax / _2;
int _3 = %eax;

*/
void localize_x86_64_divide_modulo(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    try_extract_integer_constant(insn, air, 2, insn->ct);
    try_extract_integer_constant(insn, air, 1, insn->ct);

    // further localization only applies to integer division
    if (insn->type == AIR_DIVIDE && !type_is_integer(insn->ct))
        return;
    regid_t hresultreg = insn->type == AIR_DIVIDE ? X86R_RAX : X86R_RDX;
    if (insn->ops[0]->type != AOP_REGISTER) report_return;
    if (insn->ops[1]->type != AOP_REGISTER) report_return;
    air_insn_t* assign_top = air_insn_init(AIR_LOAD, 2);
    assign_top->ct = type_copy(insn->ct);
    assign_top->ops[0] = air_insn_register_operand_init(X86R_RAX);
    assign_top->ops[1] = air_insn_register_operand_init(insn->ops[1]->content.reg);
    air_insn_insert_before(assign_top, insn);
    air_insn_t* zero_rdx = air_insn_init(AIR_LOAD, 2);
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

/*

_1 %= _2;

becomes:

%eax = _1;
%edx = 0;
%edx:%eax /= _2;
_1 = %edx;

_1 /= _2;

becomes:

%eax = _1;
%edx = 0;
%edx:%eax /= _2;
_1 = %eax;

*/
void localize_x86_64_direct_divide_modulo(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    try_extract_integer_constant(insn, air, 1, insn->ct);
    try_extract_integer_constant(insn, air, 0, insn->ct);

    // further localization only applies to integer division
    if (insn->type == AIR_DIRECT_DIVIDE && !type_is_integer(insn->ct))
        return;
    regid_t hresultreg = insn->type == AIR_DIRECT_DIVIDE ? X86R_RAX : X86R_RDX;
    if (insn->ops[0]->type != AOP_REGISTER) report_return;
    if (insn->ops[1]->type != AOP_REGISTER) report_return;
    air_insn_t* assign_top = air_insn_init(AIR_LOAD, 2);
    assign_top->ct = type_copy(insn->ct);
    assign_top->ops[0] = air_insn_register_operand_init(X86R_RAX);
    assign_top->ops[1] = air_insn_register_operand_init(insn->ops[0]->content.reg);
    air_insn_insert_before(assign_top, insn);
    air_insn_t* zero_rdx = air_insn_init(AIR_LOAD, 2);
    zero_rdx->ct = type_copy(insn->ct);
    zero_rdx->ops[0] = air_insn_register_operand_init(X86R_RDX);
    zero_rdx->ops[1] = air_insn_integer_constant_operand_init(0);
    air_insn_insert_before(zero_rdx, insn);
    regid_t resultreg = insn->ops[1]->content.reg;
    insn->ops[0]->content.reg = INVALID_VREGID;
    air_insn_t* load_result = air_insn_init(AIR_LOAD, 2);
    load_result->ct = type_copy(insn->ct);
    load_result->ops[0] = air_insn_register_operand_init(resultreg);
    load_result->ops[1] = air_insn_register_operand_init(hresultreg);
    air_insn_insert_after(load_result, insn);
    insn->type = AIR_DIRECT_DIVIDE;
}

/*

unsigned _3 = _1 * _2;

becomes:

unsigned %eax = _1;
unsigned _3 = %eax * _2;
blip %rdx;

*/
void localize_x86_64_multiply(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (!type_is_unsigned_integer(insn->ct) && insn->ct->class != CTC_POINTER)
        return;

    try_extract_integer_constant(insn, air, 2, insn->ct);
    
    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = insn->ops[1]->ct ? type_copy(insn->ops[1]->ct) : type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(X86R_RAX);
    ld->ops[1] = air_insn_operand_copy(insn->ops[1]);
    air_insn_insert_before(ld, insn);

    air_insn_operand_delete(insn->ops[1]);
    insn->ops[1] = air_insn_register_operand_init(X86R_RAX);

    air_insn_t* blip = air_insn_init(AIR_BLIP, 1);
    blip->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    blip->ops[0] = air_insn_register_operand_init(X86R_RDX);
    air_insn_insert_after(blip, insn);
}

/*

_2 *= _1;

becomes:

unsigned %eax = _2;
%eax *= _1;
_2 = %eax;
blip %rdx;

*/
void localize_x86_64_direct_multiply(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (!type_is_unsigned_integer(insn->ct) && insn->ct->class != CTC_POINTER)
        return;

    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = insn->ops[0]->ct ? type_copy(insn->ops[0]->ct) : type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(X86R_RAX);
    ld->ops[1] = air_insn_operand_copy(insn->ops[0]);
    air_insn_insert_before(ld, insn);

    air_insn_operand_delete(insn->ops[0]);
    insn->ops[0] = air_insn_register_operand_init(X86R_RAX);

    air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
    assign->ct = type_copy(insn->ct);
    assign->ops[0] = air_insn_operand_copy(insn->ops[0]);
    assign->ops[1] = air_insn_register_operand_init(X86R_RAX);
    air_insn_insert_after(assign, insn);

    air_insn_t* blip = air_insn_init(AIR_BLIP, 1);
    blip->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    blip->ops[0] = air_insn_register_operand_init(X86R_RDX);
    air_insn_insert_after(blip, insn);
}

void localize_x86_64_add_subtract(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    try_extract_integer_constant(insn, air, 1, insn->ct);
}

/*

    int _1 = 5;
    return _1;

    becomes:

    int _1 = 5;
    int %eax = _1;
    return;

    float _1 = 5.0f;
    return _1;

    becomes:

    float _1 = 5.0f;
    float %xmm0 = _1;
    return;

    struct point{22}* _1;
    return *_1;

    becomes:

    struct point{22}* _1;
    unsigned long long int _2 = *_1;
    __anonymous_lv__ = _2;
    unsigned long long int _3 = *(_1 + 8);
    __anonymous_lv__ + 8 = _3;
    unsigned int _4 = *(_1 + 16);
    __anonymous_lv__ + 16 = _4;
    unsigned short _5 = *(_1 + 20);
    __anonymous_lv__ + 20 = _5;
    return;

    struct point{14}* _1;
    return *_1;

    becomes:

    *_1:  01 02 03 04 05 06 07 08 |       09 0A 0B 0C 0D 0E
    %rax: 01 02 03 04 05 06 07 08 | %rdx: 09 0A 0B 0C 0D 0E 00 00

    struct point{14}* _1;
    unsigned long long int %rax = *_1;
    unsigned int %edx = *(_1 + 10);
    %rdx <<= 16;
    %dx = *(_1 + 8);

*/

// handles return values w/ respect to the System V ABI
void localize_x86_64_return(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (insn->noops < 1)
        return;
    
    try_extract_integer_constant(insn, air, 0, insn->ct);
    
    regid_t integer_return_sequence[] = { X86R_RAX, X86R_RDX };
    size_t next_intretreg = 0;

    regid_t sse_return_sequence[] = { X86R_XMM0, X86R_XMM1 };
    size_t next_sseretreg = 0;

    // get the function's type
    c_type_t* ftype = routine->sy->type;

    // get its return type
    c_type_t* rettype = ftype->derived_from;
    long long rtsize = type_size(rettype);

    air_insn_operand_t* retop = insn->ops[0];
    regid_t retreg = INVALID_VREGID;
    if (retop->type == AOP_REGISTER)
        retreg = retop->content.reg;
    else if (retop->type == AOP_INDIRECT_REGISTER)
        retreg = retop->content.inreg.id;
    else
        report_return;
    
    air_insn_operand_delete(retop);
    insn->noops = 0;

    air_insn_t* pos = insn->prev;

    if (!pos)
        report_return;

    if ((rettype->class == CTC_STRUCTURE || rettype->class == CTC_UNION) && type_size(rettype) > 16)
    {
        for (long long copied = 0; copied < rtsize;)
        {
            long long remaining = rtsize - copied;
            c_type_t* copytype = make_basic_type(largest_type_class_for_eightbyte(remaining));

            regid_t tempreg = NEXT_VIRTUAL_REGISTER;

            air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
            ld->ct = copytype;
            ld->ops[0] = air_insn_register_operand_init(tempreg);
            ld->ops[1] = air_insn_indirect_register_operand_init(retreg, copied, INVALID_VREGID, 1);
            pos = air_insn_insert_after(ld, pos);

            air_insn_t* copy = air_insn_init(AIR_ASSIGN, 2);
            copy->ct = type_copy(copytype);
            copy->ops[0] = air_insn_indirect_symbol_operand_init(routine->retptr, copied);
            copy->ops[1] = air_insn_register_operand_init(tempreg);
            pos = air_insn_insert_after(copy, pos);

            copied += type_size(copytype);
        }
        return;
    }

    if (type_is_integer(rettype) || rettype->class == CTC_ARRAY || rettype->class == CTC_POINTER)
    {
        air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
        ld->ct = type_copy(rettype);
        ld->ops[0] = air_insn_register_operand_init(X86R_RAX);
        ld->ops[1] = air_insn_register_operand_init(retreg);
        pos = air_insn_insert_after(ld, pos);
        return;
    }

    if (type_is_sse_floating(rettype))
    {
        air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
        ld->ct = type_copy(rettype);
        ld->ops[0] = air_insn_register_operand_init(X86R_XMM0);
        ld->ops[1] = air_insn_register_operand_init(retreg);
        pos = air_insn_insert_after(ld, pos);
        return;
    }

    // TODO: handle the garbage

    if (rettype->class != CTC_STRUCTURE && rettype->class != CTC_UNION)
        report_return;

    size_t ccount = 0;
    arg_class_t* classes = find_classes(rettype, &ccount);
    if (!classes) report_return;

    for (size_t i = 0; i < ccount; ++i)
    {
        arg_class_t class = classes[i];

        if (class == ARG_INTEGER || class == ARG_SSE)
        {
            long long copy_size = min(rtsize - (i * 8), UNSIGNED_LONG_LONG_INT_WIDTH);

            for (long long copied = 0; copied < copy_size;)
            {
                long long remaining = copy_size - copied;
                c_type_t* copytype = make_basic_type(class == ARG_SSE ?
                    largest_sse_type_class_for_eightbyte(remaining) : largest_type_class_for_eightbyte(remaining));
                long long cpytsize = type_size(copytype);

                if (copied)
                {
                    air_insn_t* shl = air_insn_init(AIR_DIRECT_SHIFT_LEFT, 2);
                    shl->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                    shl->ops[0] = air_insn_register_operand_init(integer_return_sequence[next_intretreg]);
                    shl->ops[1] = air_insn_integer_constant_operand_init(cpytsize << 3);
                    pos = air_insn_insert_after(shl, pos);
                }

                air_insn_t* copy = air_insn_init(copied ? AIR_ASSIGN : AIR_LOAD, 2);
                copy->ct = copytype;
                copy->ops[0] = air_insn_register_operand_init(class == ARG_SSE ?
                    sse_return_sequence[next_sseretreg] : integer_return_sequence[next_intretreg]);
                copy->ops[1] = air_insn_indirect_register_operand_init(retreg, (i << 3) + remaining - cpytsize, INVALID_VREGID, 1);
                pos = air_insn_insert_after(copy, pos);

                copied += cpytsize;
            }

            if (class == ARG_INTEGER)
                ++next_intretreg;
            else if (class == ARG_SSE)
                ++next_sseretreg;
        }
        else
            report_return;
    }
    
    free(classes);
}

// assigns parameter locals
void localize_x86_64_routine_before(air_routine_t* routine, air_t* air)
{
    // get the function's type
    c_type_t* ftype = routine->sy->type;

    // get its return type
    c_type_t* rettype = ftype->derived_from;

    // initialize location references for pulling parameters
    regid_t nextintreg = X86R_RDI;
    regid_t nextssereg = X86R_XMM0;
    long long nexteightbyteoffset = 16;

    air_insn_t* inserting = routine->insns;

    // if the return type is a struct/union larger than two eightbytes,
    // we need to make a variable to hold it
    if ((rettype->class == CTC_STRUCTURE || rettype->class == CTC_UNION) && type_size(rettype) > 16)
    {
        /*
        struct point* __anonymous_lv__;
        __anonymous_lv__ = %rdi;
         */

        // create and declare a local variable to store the ptr for the return value
        symbol_t* sy = symbol_table_add(air->st, "__anonymous_lv__", symbol_init(NULL));
        sy->type = make_reference_type(rettype);

        air_insn_t* decl = air_insn_init(AIR_DECLARE, 1);
        decl->ops[0] = air_insn_symbol_operand_init(sy);

        // assign %rdi to the local variable
        air_insn_t* assign = air_insn_init(AIR_ASSIGN, 2);
        assign->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
        assign->ops[0] = air_insn_symbol_operand_init(sy);
        assign->ops[1] = air_insn_register_operand_init(nextintreg++);

        inserting = air_insn_insert_after(assign, inserting);
        inserting = air_insn_insert_after(decl, inserting);

        // keep track of the local variable so we can return it l8r
        routine->retptr = sy;
    }

    syntax_component_t* fdeclr = syntax_get_function_declarator(routine->sy->declarer);

    if (!fdeclr) report_return;

    if (!fdeclr->fdeclr_parameter_declarations)
    {
        syntax_component_t* id = syntax_get_declarator_identifier(fdeclr);
        if (id)
            printf("no parameter declarations found for function: %s\n", id->id);
        // TODO: need to handle K&R functions (maybe?)
        report_return;
    }

    // loop thru all parameters of the function
    VECTOR_FOR(syntax_component_t*, pdecl, fdeclr->fdeclr_parameter_declarations)
    {
        syntax_component_t* pid = syntax_get_declarator_identifier(pdecl->pdecl_declr);
        if (!pid) continue;

        // get the symbol and type associated with the current parameter
        symbol_t* psy = symbol_table_get_syn_id(air->st, pid);
        c_type_t* pt = vector_get(ftype->function.param_types, i);

        long long ptsize = type_size(pt);
        long long total_copied = 0;

        // get the argument classes associated with this parameter's type
        size_t ccount = 0;
        arg_class_t* classes = find_classes(pt, &ccount);

        for (size_t i = 0; i < ccount; ++i)
        {
            arg_class_t class = classes[i];

            regid_t reg = INVALID_VREGID;
            if (class == ARG_INTEGER && nextintreg <= X86R_R9)
            {
                reg = nextintreg++;
                air_insn_t* insn = air_insn_init(AIR_DECLARE_REGISTER, 1);
                insn->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                insn->ops[0] = air_insn_register_operand_init(reg);
                inserting = air_insn_insert_after(insn, inserting);
            }
            else if (class == ARG_SSE && nextssereg <= X86R_XMM7)
            {
                reg = nextssereg++;
                air_insn_t* insn = air_insn_init(AIR_DECLARE_REGISTER, 1);
                insn->ct = make_basic_type(CTC_DOUBLE);
                insn->ops[0] = air_insn_register_operand_init(reg);
                inserting = air_insn_insert_after(insn, inserting);
            }
            else if (class == ARG_MEMORY || class == ARG_INTEGER || class == ARG_SSE)
            {
                air_insn_t* insn = air_insn_init(AIR_LOAD, 2);
                insn->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                insn->ops[0] = air_insn_register_operand_init(reg = NEXT_VIRTUAL_REGISTER);
                insn->ops[1] = air_insn_indirect_register_operand_init(X86R_RBP, nexteightbyteoffset, INVALID_VREGID, 1);
                nexteightbyteoffset += 8;
                inserting = air_insn_insert_after(insn, inserting);
            }

            long long to_be_copied = min(ptsize - total_copied, UNSIGNED_LONG_LONG_INT_WIDTH);
            for (long long copied = 0; copied < to_be_copied;)
            {
                long long remaining = to_be_copied - copied;
                c_type_t* tt = make_basic_type(class == ARG_SSE ? 
                    largest_sse_type_class_for_eightbyte(remaining) : largest_type_class_for_eightbyte(remaining));
                long long ttsize = type_size(tt);

                air_insn_t* insn = air_insn_init(AIR_ASSIGN, 2);
                insn->ct = tt;
                insn->ops[0] = air_insn_indirect_symbol_operand_init(psy, total_copied);
                insn->ops[1] = air_insn_register_operand_init(reg);

                inserting = air_insn_insert_after(insn, inserting);

                copied += ttsize;
                total_copied += ttsize;

                if (copied >= to_be_copied)
                    continue;
                
                air_insn_t* shr = air_insn_init(AIR_DIRECT_SHIFT_RIGHT, 2);
                shr->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
                shr->ops[0] = air_insn_register_operand_init(reg);
                shr->ops[1] = air_insn_integer_constant_operand_init(ttsize << 3);

                inserting = air_insn_insert_after(shr, inserting);
            }
        }
    }
}

/*
... va_start(_1);

becomes:

unsigned long long int _2 = %rbp;
_2 -= 176;
*(_1 + 8) = _2;
_2 += 128;
*_1 = _2;
_2 += 64;
*(_1 + 16) = _2;

*/
air_insn_t* localize_x86_64_va_start(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    symbol_t* fsy = routine->sy;
    syntax_component_t* fdeclr = syntax_get_function_declarator(fsy->declarer);
    if (!fdeclr || fdeclr->type != SC_FUNCTION_DECLARATOR) report_return_value(NULL);
    vector_t* pdecls = fdeclr->fdeclr_parameter_declarations;
    if (!pdecls) report_return_value(NULL);

    long long intoffset = -48;
    long long sseoffset = -176;
    long long stackoffset = 16;

    VECTOR_FOR(syntax_component_t*, pdecl, pdecls)
    {
        syntax_component_t* id = syntax_get_declarator_identifier(pdecl->pdecl_declr);
        if (!id) report_return_value(NULL);
        symbol_t* psy = symbol_table_get_syn_id(SYMBOL_TABLE, id);
        if (!psy) report_return_value(NULL);
        c_type_t* pt = psy->type;
        size_t ccount = 0;
        arg_class_t* classes = find_classes(pt, &ccount);
        if (!classes) report_return_value(NULL);
        for (size_t j = 0; j < ccount; ++j)
        {
            arg_class_t class = classes[i];
            switch (class)
            {
                case ARG_INTEGER:
                    if (intoffset == 0)
                        stackoffset += UNSIGNED_LONG_LONG_INT_WIDTH;
                    else
                        intoffset += UNSIGNED_LONG_LONG_INT_WIDTH;
                    break;
                case ARG_SSE:
                    if (sseoffset == -48)
                        stackoffset += UNSIGNED_LONG_LONG_INT_WIDTH;
                    else
                        sseoffset += 16;
                    break;
                case ARG_MEMORY:
                    stackoffset += UNSIGNED_LONG_LONG_INT_WIDTH;
                    break;
                case ARG_NO_CLASS:
                case ARG_COMPLEX_X87:
                case ARG_SSEUP:
                case ARG_X87:
                case ARG_X87UP:
                    break;
            }
        }
        free(classes);
    }

    regid_t reg = NEXT_VIRTUAL_REGISTER;

    regid_t valist_reg = insn->ops[1]->content.reg;

    air_insn_t* ld_rbp = air_insn_init(AIR_LOAD, 2);
    ld_rbp->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    ld_rbp->ops[0] = air_insn_register_operand_init(reg);
    ld_rbp->ops[1] = air_insn_register_operand_init(X86R_RBP);
    air_insn_insert_before(ld_rbp, insn);

    air_insn_t* sub = air_insn_init(AIR_DIRECT_SUBTRACT, 2);
    sub->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    sub->ops[0] = air_insn_register_operand_init(reg);
    sub->ops[1] = air_insn_integer_constant_operand_init(abs(sseoffset));
    air_insn_insert_before(sub, insn);

    air_insn_t* ld_ssepos = air_insn_init(AIR_ASSIGN, 2);
    ld_ssepos->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    ld_ssepos->ops[0] = air_insn_indirect_register_operand_init(valist_reg, 8, INVALID_VREGID, 1);
    ld_ssepos->ops[1] = air_insn_register_operand_init(reg);
    air_insn_insert_before(ld_ssepos, insn);

    air_insn_t* add1 = air_insn_init(AIR_DIRECT_ADD, 2);
    add1->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    add1->ops[0] = air_insn_register_operand_init(reg);
    add1->ops[1] = air_insn_integer_constant_operand_init(intoffset - sseoffset);
    air_insn_insert_before(add1, insn);

    air_insn_t* ld_intpos = air_insn_init(AIR_ASSIGN, 2);
    ld_intpos->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    ld_intpos->ops[0] = air_insn_indirect_register_operand_init(valist_reg, 0, INVALID_VREGID, 1);
    ld_intpos->ops[1] = air_insn_register_operand_init(reg);
    air_insn_insert_before(ld_intpos, insn);

    air_insn_t* add2 = air_insn_init(AIR_DIRECT_ADD, 2);
    add2->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    add2->ops[0] = air_insn_register_operand_init(reg);
    add2->ops[1] = air_insn_integer_constant_operand_init(stackoffset - intoffset);
    air_insn_insert_before(add2, insn);

    air_insn_t* ld_stackpos = air_insn_init(AIR_ASSIGN, 2);
    ld_stackpos->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    ld_stackpos->ops[0] = air_insn_indirect_register_operand_init(valist_reg, 16, INVALID_VREGID, 1);
    ld_stackpos->ops[1] = air_insn_register_operand_init(reg);
    air_insn_insert_before(ld_stackpos, insn);

    air_insn_remove(insn);

    return ld_stackpos;
}

/*
type _2 = va_arg(_1)

becomes:
unsigned long long int _3 = *(_1 + offset of pos for type);
type _2 = *_3;
*(_1 + offset of pos for type) += (pos increment for type);

*/
air_insn_t* localize_x86_64_va_arg(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    c_type_t* ct = insn->ct;

    // TODO: do actual ABI class analysis
    size_t offset = 0;
    size_t increment = 0;
    if (type_is_integer(ct) || ct->class == CTC_POINTER)
    {
        offset = 0;
        increment = 8;
    }
    else if (ct->class == CTC_FLOAT || ct->class == CTC_DOUBLE)
    {
        offset = 8;
        increment = 16;
    }
    else
    {
        offset = 16;
        increment = 8;
    }

    regid_t reg = insn->ops[0]->content.reg;
    regid_t valist_reg = insn->ops[1]->content.reg;

    regid_t posreg = NEXT_VIRTUAL_REGISTER;

    air_insn_t* ld_pos = air_insn_init(AIR_LOAD, 2);
    ld_pos->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    ld_pos->ops[0] = air_insn_register_operand_init(posreg);
    ld_pos->ops[1] = air_insn_indirect_register_operand_init(valist_reg, offset, INVALID_VREGID, 1);
    air_insn_insert_before(ld_pos, insn);

    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(reg);
    ld->ops[1] = air_insn_indirect_register_operand_init(posreg, 0, INVALID_VREGID, 1);
    air_insn_insert_before(ld, insn);

    air_insn_t* add = air_insn_init(AIR_DIRECT_ADD, 2);
    add->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    add->ops[0] = air_insn_indirect_register_operand_init(valist_reg, offset, INVALID_VREGID, 1);
    add->ops[1] = air_insn_integer_constant_operand_init(increment);
    air_insn_insert_before(add, insn);

    air_insn_remove(insn);

    return add;
}

air_insn_t* localize_x86_64_va_end(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    return air_insn_remove(insn);
}

/*

for comparison operators:

int _3 = _1 == _2;

becomes:

int _3 = _1 == _2;
_3 &= 1;

*/
void localize_x86_64_setcc_comparison(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    regid_t reg = insn->ops[0]->content.reg;

    air_insn_t* and = air_insn_init(AIR_DIRECT_AND, 2);
    and->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    and->ops[0] = air_insn_register_operand_init(reg);
    and->ops[1] = air_insn_integer_constant_operand_init(1);
    air_insn_insert_after(and, insn);
}

void localize_x86_64_shift(air_insn_t* insn, air_routine_t* routine, air_t* air, size_t index)
{
    if (insn->ops[index]->type == AOP_INTEGER_CONSTANT &&
        insn->ops[index]->content.ic >= 0 &&
        insn->ops[index]->content.ic <= 0xFF)
        return;
    
    if (insn->ops[index]->type != AOP_REGISTER)
        report_return;

    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(X86R_RCX);
    ld->ops[1] = air_insn_register_operand_init(insn->ops[index]->content.reg);
    air_insn_insert_before(ld, insn);

    insn->ops[index]->content.reg = X86R_RCX;
}

/*

sse _2 = -_1;

becomes:

sse _3 = __fcX; (__fcX = 0x80000000 for floats, __fcX = 0x8000000000000000 for doubles)
sse _2 = _1 ^ _3;

*/
air_insn_t* localize_x86_64_sse_negate(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    bool is_float = insn->ct->class == CTC_FLOAT;
    symbol_t* negater = is_float ? air->sse32_negater : air->sse64_negater;
    bool to_define = !negater;
    if (!negater && is_float)
    {
        negater = air->sse32_negater = symbol_table_add(SYMBOL_TABLE, "__sse32_negater", symbol_init(NULL));
        negater->name = strdup("__sse32_negater");
        negater->type = make_basic_type(CTC_FLOAT);
        negater->sd = SD_STATIC;
    }
    if (!negater && !is_float)
    {
        negater = air->sse64_negater = symbol_table_add(SYMBOL_TABLE, "__sse64_negater", symbol_init(NULL));
        negater->name = strdup("__sse64_negater");
        negater->type = make_basic_type(CTC_DOUBLE);
        negater->sd = SD_STATIC;
    }

    if (to_define)
    {
        air_data_t* data = calloc(1, sizeof *data);
        data->readonly = true;
        data->sy = negater;
        if (is_float)
        {
            data->data = malloc(FLOAT_WIDTH);
            *((unsigned*) (data->data)) = 0x80000000;
        }
        else
        {
            data->data = malloc(DOUBLE_WIDTH);
            *((unsigned long long*) (data->data)) = 0x8000000000000000;
        }
        vector_add(air->rodata, data);
    }

    regid_t negater_reg = NEXT_VIRTUAL_REGISTER;

    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(negater_reg);
    ld->ops[1] = air_insn_symbol_operand_init(negater);
    air_insn_insert_before(ld, insn);

    air_insn_t* xor = air_insn_init(AIR_XOR, 3);
    xor->ct = type_copy(insn->ct);
    xor->ops[0] = air_insn_operand_copy(insn->ops[0]);
    xor->ops[1] = air_insn_operand_copy(insn->ops[1]);
    xor->ops[2] = air_insn_register_operand_init(negater_reg);
    air_insn_insert_before(xor, insn);

    air_insn_remove(insn);

    return xor;
}

air_insn_t* localize_x86_64_negate(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    if (type_is_sse_floating(insn->ct))
        return localize_x86_64_sse_negate(insn, routine, air);
    return insn;
}

void localize_x86_64_memset(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    air_insn_operand_t* op1 = insn->ops[0];
    air_insn_operand_t* op2 = insn->ops[1];
    air_insn_operand_t* op3 = insn->ops[2];

    air_insn_t* ldv = air_insn_init(AIR_LOAD, 2);
    ldv->ct = make_basic_type(CTC_UNSIGNED_CHAR);
    ldv->ops[0] = air_insn_register_operand_init(X86R_RAX);
    ldv->ops[1] = air_insn_operand_copy(op1);
    air_insn_insert_before(ldv, insn);

    air_insn_t* ldptr = air_insn_init(AIR_LOAD_ADDR, 2);
    ldptr->ct = type_copy(insn->ct);
    ldptr->ops[0] = air_insn_register_operand_init(X86R_RDI);
    ldptr->ops[1] = air_insn_operand_copy(op2);
    air_insn_insert_before(ldptr, insn);

    air_insn_t* ldc = air_insn_init(AIR_LOAD, 2);
    ldc->ct = make_basic_type(C_TYPE_SIZE_T);
    ldc->ops[0] = air_insn_register_operand_init(X86R_RCX);
    ldc->ops[1] = air_insn_operand_copy(op3);
    air_insn_insert_before(ldc, insn);

    air_insn_operand_delete(op1);
    air_insn_operand_delete(op2);
    air_insn_operand_delete(op3);

    insn->ops[0] = air_insn_register_operand_init(X86R_RAX);
    insn->ops[0]->ct = make_basic_type(CTC_UNSIGNED_CHAR);
    insn->ops[1] = air_insn_register_operand_init(X86R_RDI);
    insn->ops[2] = air_insn_register_operand_init(X86R_RCX);
}

void localize_x86_64_assign_imm64_to_m64(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    air_insn_operand_t* op2 = insn->ops[1];

    if (op2->type != AOP_INTEGER_CONSTANT) return;
    if (type_size(insn->ct) != UNSIGNED_LONG_LONG_INT_WIDTH) return;

    regid_t reg = NEXT_VIRTUAL_REGISTER;

    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(reg);
    ld->ops[1] = air_insn_operand_copy(op2);
    air_insn_insert_before(ld, insn);

    air_insn_operand_delete(op2);
    insn->ops[1] = air_insn_register_operand_init(reg);
}

void localize_x86_64_assign(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    localize_x86_64_assign_imm64_to_m64(insn, routine, air);
}

/*

lsyscall(id, _1, _2, _3, _4, _5, _6);

becomes:

int %rax = id;
type %rdi = _1; 
type %rsi = _2; 
type %rdx = _3;
type %r10 = _4;
type %r8 = _5;
type %r9 = _6;
lsyscall;
blip volatiles;

*/

void localize_x86_64_lsyscall(air_insn_t* insn, air_routine_t* routine, air_t* air)
{
    static regid_t sequence[] = {
        X86R_RDI,
        X86R_RSI,
        X86R_RDX,
        X86R_R10,
        X86R_R8,
        X86R_R9
    };
    size_t nextreg = 0;
    for (size_t i = 2; i < insn->noops; ++i)
    {
        air_insn_operand_t* op = insn->ops[i];
        if (!op || op->type != AOP_REGISTER) report_return;

        air_insn_t* def = air_insn_find_temporary_definition_from_insn(op->content.reg, insn);
        if (!def) report_return;

        air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
        ld->ct = type_copy(def->ct);
        ld->ops[0] = air_insn_register_operand_init(sequence[nextreg++]);
        ld->ops[1] = air_insn_operand_copy(op);
        air_insn_insert_before(ld, insn);

        op->content.reg = INVALID_VREGID;
    }

    air_insn_operand_t* id_op = insn->ops[1];

    if (!id_op || id_op->type != AOP_INTEGER_CONSTANT) report_return;

    air_insn_t* id_load = air_insn_init(AIR_LOAD, 2);
    id_load->ct = make_basic_type(CTC_UNSIGNED_LONG_LONG_INT);
    id_load->ops[0] = air_insn_register_operand_init(X86R_RAX);
    id_load->ops[1] = air_insn_integer_constant_operand_init(id_op->content.ic);
    air_insn_insert_before(id_load, insn);
    id_op->type = AOP_REGISTER;
    id_op->content.reg = INVALID_VREGID;

    air_insn_t* pos = blip_volatiles_after(insn);

    air_insn_operand_t* rop = insn->ops[0];

    if (!rop || rop->type != AOP_REGISTER) report_return;

    regid_t rreg = rop->content.reg;

    air_insn_t* ld = air_insn_init(AIR_LOAD, 2);
    ld->ct = type_copy(insn->ct);
    ld->ops[0] = air_insn_register_operand_init(rreg);
    ld->ops[1] = air_insn_register_operand_init(X86R_RAX);
    pos = air_insn_insert_after(ld, pos);

    rop->content.reg = INVALID_VREGID;
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
                case AIR_DIRECT_DIVIDE:
                case AIR_DIRECT_MODULO:
                    localize_x86_64_direct_divide_modulo(insn, routine, air);
                    break;
                case AIR_ADD:
                case AIR_SUBTRACT:
                    localize_x86_64_add_subtract(insn, routine, air);
                    break;
                case AIR_MULTIPLY:
                    localize_x86_64_multiply(insn, routine, air);
                    break;
                case AIR_DIRECT_MULTIPLY:
                    localize_x86_64_direct_multiply(insn, routine, air);
                    break;
                case AIR_VA_START:
                    insn = localize_x86_64_va_start(insn, routine, air);
                    break;
                case AIR_VA_ARG:
                    insn = localize_x86_64_va_arg(insn, routine, air);
                    break;
                case AIR_VA_END:
                    insn = localize_x86_64_va_end(insn, routine, air);
                    break;
                case AIR_SHIFT_LEFT:
                case AIR_SHIFT_RIGHT:
                case AIR_SIGNED_SHIFT_RIGHT:
                    localize_x86_64_shift(insn, routine, air, 2);
                    break;
                case AIR_DIRECT_SHIFT_LEFT:
                case AIR_DIRECT_SHIFT_RIGHT:
                case AIR_DIRECT_SIGNED_SHIFT_RIGHT:
                    localize_x86_64_shift(insn, routine, air, 1);
                    break;
                case AIR_LESS_EQUAL:
                case AIR_LESS:
                case AIR_GREATER_EQUAL:
                case AIR_GREATER:
                case AIR_EQUAL:
                case AIR_INEQUAL:
                case AIR_NOT:
                    localize_x86_64_setcc_comparison(insn, routine, air);
                    break;
                case AIR_NEGATE:
                    insn = localize_x86_64_negate(insn, routine, air);
                    break;
                case AIR_MEMSET:
                    localize_x86_64_memset(insn, routine, air);
                    break;
                case AIR_ASSIGN:
                    localize_x86_64_assign(insn, routine, air);
                    break;
                case AIR_LSYSCALL:
                    localize_x86_64_lsyscall(insn, routine, air);
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

/*

LOCALIZATION RULES:
    - the live ranges of a physical register being used for two separate calculations may not overlap

*/
