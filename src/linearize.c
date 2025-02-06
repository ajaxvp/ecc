#include <stdlib.h>
#include <string.h>

#include "cc.h"

typedef struct linearizing_syntax_traverser
{
    syntax_traverser_t base;
    regid_t next_vregid;
    unsigned long long next_llabel;
} linearizing_syntax_traverser_t;

locator_t* locator_copy(locator_t* loc)
{
    if (!loc) return NULL;
    locator_t* n = calloc(1, sizeof *loc);
    n->type = loc->type;
    switch (loc->type)
    {
        case L_OFFSET:
            n->stack_offset = loc->stack_offset;
            break;
        case L_LABEL:
            n->label = strdup(loc->label);
            break;
        case L_ARRAY:
            n->array.base_reg = loc->array.base_reg;
            n->array.offset_reg = loc->array.offset_reg;
            n->array.scale = loc->array.scale;
            break;
    }
    return n;
}

void locator_delete(locator_t* loc)
{
    if (!loc) return;
    switch (loc->type)
    {
        case L_LABEL:
            free(loc->label);
            break;
        default:
            break;
    }
    free(loc);
}

void locator_print(locator_t* loc, int (*printer)(const char* fmt, ...))
{
    if (!loc)
    {
        printer("(null)");
        return;
    }
    switch (loc->type)
    {
        case L_OFFSET:
            printer("local[%lld]", loc->stack_offset);
            break;
        case L_LABEL:
            printer("%s", loc->label);
            break;
        case L_ARRAY:
            printer("array[_%lld, _%lld, %d]", loc->array.base_reg, loc->array.offset_reg, loc->array.scale);
            break;
    }
}

/*

II_LOAD_OBJECT:
    type _r = obj;
where
    type is the type of the object
    r is the next available virtual register
    obj is the identifier of the object to load
notes:
    after the instruction, r contains the value of obj.
    after the instruction, r has an address associated with the value stored in it.
        if the object is stored on the stack, the address is:
            offset(obj)(%rbp)
        where off is the offset of the object on the stack
        if the object is stored globally, the address is:
            obj(%rip)
        where obj is the identifier of the object loaded
rough x86 equivalent:
    movX address(obj), %r
    leaX address(obj), %r (for arrays, structs)

II_LOAD_ICONST:
    type _r = con;
where
    type is the type of the constant
    r is the next available virtual register
    con is an integer constant
notes:
    after the instruction, r contains the value of con.
rough x86 equivalent:
    movX $con, %r

II_LOAD_SUBSCRIPT:
    type _r = _rA[_rB];
where
    type is the type of the result of the subscript
    r is the next available virtual register
    rA is the register that contains the array/pointer to subscript
    rB is the register that contains the index
notes:
    after the instruction, r contains the value of the subscription of the pointer in rA with the index in rB
    after the instruction, r has an address associated with the value stored in it.
        the address is: %rA, %rB, sizeof(type)
rough x86 equivalent:
    movX (%rA, %rB, sizeof(type)), %r

II_LOAD_DEREFERENCE:
    type _r = *_rA;
notes:
    after the instruction, r has an address associated with the value stored in it.
        the address is: %rA

II_LOAD_MEMBER:
    type _r = _rA._rB;
notes:
    after the instruction, r has an address associated with the value stored in it.
        the address is: offset(%rB)(%rA)

II_LOAD_STRING_LITERAL:
    type _r = str;
rough x86 equivalent:
    leaq str(%rip), %r
    after the instruction, r has an address associated with the value stored in it.
        the address is: str(%rip)

II_LOAD_REFERENCE:
    type _r = &_rA;
where
    type is pointer to T if the type of rA has type T
    r is the next available virtual register
    rA is the register that contains the object to reference
notes:
    rA must have an address associated with it in order to reference
    after the instruction, r contains the address to the object in rA
    for clarification: r does NOT have an address associated with it
rough x86 equivalent:
    movX (address), %r

II_STORE_ADDRESS
    type _r := _rA = _rB;
where
    type is the type of the object represented by rA
    rA is the register representing the left hand object
    rB is the register with the value to be copied to the object on the left hand side
notes:
    rA must have an address associated with it
    after the instruction, the object represented by rA contains the value stored in rB
rough x86 equivalent:
    movX %rB, (address(%rA))

int main(void)
{
    int x[3];
    int *p = &x[0];
    *p = 3;
    return 0;
}

int main(void)
{
    int *_1 := p;       [II_LOAD_OBJECT]            -> movq -20(%rbp), %r1
    int _2[3] := x;     [II_LOAD_OBJECT]            -> leaq -4(%rbp), %r2
    int _3 := 0;        [II_LOAD_ICONST]            -> movl $0, %e3
    int _4 := _2[_3];   [II_LOAD_SUBSCRIPT]         -> movl (%r2, %r3, 4), %e4
    int *_5 := &_4;     [II_LOAD_REFERENCE]         -> leaq (%r2, %r3, 4), %r5
    int _6 := _1 = _5;  [II_STORE_ADDRESS]          -> movq %r5, -20(%rbp); movq %r5, %r6
    int *_7 := p;       [II_LOAD_OBJECT]            -> movq -20(%rbp), %r7
    int _8 := *_7;      [II_LOAD_DEREFERENCE]       -> movl (%r7), %e8
    int _9 := 3;        [II_LOAD_ICONST]            -> movl $3, %e9
    int _10 := _8 = _9; [II_STORE_ADDRESS]          -> movl %e9, (%r7); movl %e9, %e10
    int _11 := 0;       [II_LOAD_ICONST]            -> movl $0, %e11
    return _11;         [II_RETURN]                 -> jmp .Rmain
}

int main(void)
{
    struct point
    {
        struct j
        {
            int x;
        } fart;
        int y;
    } p;
    p.fart.x = 3;
}

main:                               [II_LABEL]              -> main:
    struct point _1 := local[0];    [II_LOAD_OBJECT]        -> leaq (%rbp), %r1
    struct j _2 := _1.fart;         [II_LOAD_MEMBER]        -> leaq (%r1), %r2
    int _3 = _2.x;                  [II_LOAD_MEMBER]        -> leaq (%r2), %r3
    int _4 := 3;                    [II_LOAD_ICONST]        -> movl $3, %e4
    int _5 := _2 = _4;              [II_STORE_ADDRESS]      -> movl %e4, (%r2); movl %e4, %e5

int main(void)
{
    return 2;
}

main:               [II_LABEL]          ->
    int _1 := 2;    [II_LOAD_ICONST]    -> 
    return _1;      [II_RETURN]

*/

ir_insn_t* insn_copy(ir_insn_t* insn)
{
    ir_insn_t* n = calloc(1, sizeof *insn);
    n->type = insn->type;
    n->prev = insn->prev;
    n->next = insn->next;
    n->result = insn->result;
    n->loc = locator_copy(insn->loc);
    n->ctype = type_copy(insn->ctype);
    switch (insn->type)
    {
        case II_LABEL:
            n->label = strdup(insn->label);
            break;
        case II_LOAD_ICONST:
            n->load_iconst_value = insn->load_iconst_value;
            break;
        case II_RETURN:
            n->return_reg = insn->return_reg;
            break;
        case II_STORE_ADDRESS:
            n->store_address.dest = locator_copy(insn->store_address.dest);
            n->store_address.src = insn->store_address.src;
            break;
        case II_ADDITION:
        case II_SUBSCRIPT:
            n->bexpr.lhs = insn->bexpr.lhs;
            n->bexpr.rhs = insn->bexpr.rhs;
            break;
        case II_JUMP_IF_ZERO:
            n->cjmp.condition = insn->cjmp.condition;
            n->cjmp.label = strdup(insn->cjmp.label);
            break;
        case II_JUMP:
            n->jmp_label = strdup(insn->jmp_label);
            break;
        case II_FUNCTION_CALL:
            n->function_call.calling = insn->function_call.calling;
            n->function_call.noargs = insn->function_call.noargs;
            n->function_call.regargs = malloc(insn->function_call.noargs * sizeof(regid_t));
            memcpy(n->function_call.regargs, insn->function_call.regargs, insn->function_call.noargs * sizeof(regid_t));
            break;
        case II_ENTER:
            n->enter_stackalloc = insn->enter_stackalloc;
            break;
        default:
            break;
    }
    return n;
}

void insn_delete(ir_insn_t* insn)
{
    if (!insn) return;
    switch (insn->type)
    {
        case II_LABEL:
            free(insn->label);
            break;
        case II_STORE_ADDRESS:
            locator_delete(insn->store_address.dest);
            break;
        case II_JUMP_IF_ZERO:
            free(insn->cjmp.label);
            break;
        case II_JUMP:
            free(insn->jmp_label);
            break;
        case II_FUNCTION_CALL:
            free(insn->function_call.regargs);
            break;
        default:
            break;
    }
    locator_delete(insn->loc);
    type_delete(insn->ctype);
    free(insn);
}

void insn_delete_all(ir_insn_t* insn)
{
    if (!insn) return;
    insn_delete_all(insn->next);
    insn_delete(insn);
}

void insn_clike_print(ir_insn_t* insn, int (*printer)(const char* fmt, ...))
{
    if (!insn)
    {
        printer("(null)");
        return;
    }
    #define INDENT_START printer("    ")
    #define SEMICOLON_END printer(";")
    switch (insn->type)
    {
        case II_LOAD_OBJECT:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            printer(" _%llu := ", insn->result);
            locator_print(insn->loc, printer);
            SEMICOLON_END;
            break;
        case II_LOAD_ICONST:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            printer(" _%llu := %llu", insn->result, insn->load_iconst_value);
            SEMICOLON_END;
            break;
        case II_STORE_ADDRESS:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            printer(" _%llu := ", insn->result);
            locator_print(insn->store_address.dest, printer);
            printer(" = _%llu", insn->store_address.src);
            SEMICOLON_END;
            break;
        case II_LABEL:
            printer("%s:", insn->label);
            break;
        case II_RETURN:
            INDENT_START;
            printer("return _%llu", insn->return_reg);
            SEMICOLON_END;
            break;
        case II_ADDITION:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            printer(" _%llu := _%llu + _%llu", insn->result, insn->bexpr.lhs, insn->bexpr.rhs);
            SEMICOLON_END;
            break;
        case II_SUBSCRIPT:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            printer(" _%llu := _%llu[_%llu]", insn->result, insn->bexpr.lhs, insn->bexpr.rhs);
            SEMICOLON_END;
            break;
        case II_JUMP_IF_ZERO:
            INDENT_START;
            printer("jz by _%llu to %s", insn->cjmp.condition, insn->cjmp.label);
            SEMICOLON_END;
            break;
        case II_JUMP:
            INDENT_START;
            printer("jmp %s", insn->jmp_label);
            SEMICOLON_END;
            break;
        case II_FUNCTION_CALL:
            INDENT_START;
            type_humanized_print(insn->ctype, printer);
            printer(" _%llu := _%llu(", insn->result, insn->function_call.calling);
            for (size_t i = 0; i < insn->function_call.noargs; ++i)
            {
                if (i != 0)
                    printer(", ");
                printer("_%llu", insn->function_call.regargs[i]);
            }
            printer(")");
            SEMICOLON_END;
            break;
        case II_RETAIN:
            INDENT_START;
            printer("retain _%llu", insn->retain_reg);
            SEMICOLON_END;
            break;
        case II_RESTORE:
            INDENT_START;
            printer("restore _%llu", insn->restore_reg);
            SEMICOLON_END;
            break;
        case II_ENTER:
            INDENT_START;
            printer("enter %lld", insn->enter_stackalloc);
            SEMICOLON_END;
            break;
        case II_LEAVE:
            INDENT_START;
            printer("leave");
            SEMICOLON_END;
            break;
        default:
            break;
    }
    #undef INDENT_START
    #undef SEMICOLON_END
}

void insn_clike_print_all(ir_insn_t* insn, int (*printer)(const char* fmt, ...))
{
    for (; insn; insn = insn->next)
    {
        insn_clike_print(insn, printer);
        printer("\n");
    }
}

ir_insn_t* copy_code_impl(syntax_component_t* dest, syntax_component_t* src, ir_insn_t* start)
{
    if (!src || !dest || !start)
        return NULL;
    if (!src->code)
        return start;
    for (ir_insn_t* insn = src->code; insn; insn = insn->next)
        start->next = insn_copy(insn), start->next->prev = start, start = start->next;
    return start;
}

ir_insn_t* make_basic_insn(ir_insn_type_t type)
{
    ir_insn_t* insn = calloc(1, sizeof *insn);
    insn->type = type;
    insn->ctype = NULL;
    insn->result = INVALID_VREGID;
    return insn;
}

ir_insn_t* make_label_insn(char* label)
{
    ir_insn_t* insn = make_basic_insn(II_LABEL);
    insn->label = strdup(label);
    return insn;
}

ir_insn_t* make_return_insn(c_type_t* ctype, regid_t reg)
{
    ir_insn_t* insn = make_basic_insn(II_RETURN);
    insn->ctype = type_copy(ctype);
    insn->return_reg = reg;
    return insn;
}

ir_insn_t* make_load_iconst_insn(c_type_t* ctype, regid_t reg, unsigned long long value)
{
    ir_insn_t* insn = make_basic_insn(II_LOAD_ICONST);
    insn->ctype = type_copy(ctype);
    insn->result = reg;
    insn->load_iconst_value = value;
    return insn;
}

ir_insn_t* make_load_object_insn(c_type_t* ctype, regid_t reg, locator_t* loc)
{
    ir_insn_t* insn = make_basic_insn(II_LOAD_OBJECT);
    insn->ctype = type_copy(ctype);
    insn->result = reg;
    insn->loc = locator_copy(loc);
    return insn;
}

ir_insn_t* make_store_address_insn(c_type_t* ctype, regid_t reg, locator_t* dest, regid_t src)
{
    ir_insn_t* insn = make_basic_insn(II_STORE_ADDRESS);
    insn->ctype = type_copy(ctype);
    insn->result = reg;
    insn->store_address.src = src;
    insn->store_address.dest = locator_copy(dest);
    return insn;
}

ir_insn_t* make_subscript_insn(c_type_t* ctype, locator_t* loc, regid_t reg, regid_t lhs, regid_t rhs)
{
    ir_insn_t* insn = make_basic_insn(II_SUBSCRIPT);
    insn->ctype = type_copy(ctype);
    insn->loc = loc;
    insn->result = reg;
    insn->bexpr.lhs = lhs;
    insn->bexpr.rhs = rhs;
    return insn;
}

ir_insn_t* make_binary_expression_insn(c_type_t* ctype, ir_insn_type_t type, regid_t reg, regid_t lhs, regid_t rhs)
{
    ir_insn_t* insn = make_basic_insn(type);
    insn->ctype = type_copy(ctype);
    insn->result = reg;
    insn->bexpr.lhs = lhs;
    insn->bexpr.rhs = rhs;
    return insn;
}

ir_insn_t* make_condition_jump_insn(ir_insn_type_t type, regid_t condition, char* label)
{
    ir_insn_t* insn = make_basic_insn(type);
    insn->cjmp.condition = condition;
    insn->cjmp.label = strdup(label);
    return insn;
}

ir_insn_t* make_jump_insn(char* label)
{
    ir_insn_t* insn = make_basic_insn(II_JUMP);
    insn->jmp_label = strdup(label);
    return insn;
}

ir_insn_t* make_function_call_insn(c_type_t* ctype, regid_t reg, regid_t calling, regid_t* regargs, size_t noargs)
{
    ir_insn_t* insn = make_basic_insn(II_FUNCTION_CALL);
    insn->ctype = type_copy(ctype);
    insn->result = reg;
    insn->function_call.calling = calling;
    insn->function_call.regargs = regargs;
    insn->function_call.noargs = noargs;
    return insn;
}

ir_insn_t* make_enter_insn(long long stackalloc)
{
    ir_insn_t* insn = make_basic_insn(II_ENTER);
    insn->enter_stackalloc = stackalloc;
    return insn;
}

#define SETUP_LINEARIZE \
    ir_insn_t* dummy = calloc(1, sizeof *dummy); \
    dummy->type = II_UNKNOWN; \
    ir_insn_t* code = dummy;

#define ADD_CODE(t, ...) (code->next = t(__VA_ARGS__), code->next->prev = code, code = code->next)
#define COPY_CODE(s) code = copy_code_impl(syn, s, code)

#define FINALIZE_LINEARIZE \
    syn->code = dummy->next; \
    if (syn->code) syn->code->prev = NULL; \
    insn_delete(dummy);

#define LINEARIZING_TRAVERSER ((linearizing_syntax_traverser_t*) trav)
#define SYMBOL_TABLE ((syntax_get_translation_unit(syn))->tlu_st)

#define MAKE_REGISTER ++(LINEARIZING_TRAVERSER->next_vregid)

char* make_local_label(syntax_traverser_t* trav)
{
    char label[MAX_STRINGIFIED_INTEGER_LENGTH + 3];
    snprintf(label, MAX_STRINGIFIED_INTEGER_LENGTH + 3, ".L%llu", ++(LINEARIZING_TRAVERSER->next_llabel));
    return strdup(label);
}

void linearize_translation_unit_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, edecl, syn->tlu_external_declarations)
        COPY_CODE(edecl);
    FINALIZE_LINEARIZE;
}

void linearize_function_definition_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    ADD_CODE(make_label_insn, syntax_get_declarator_identifier(syn->fdef_declarator)->id);
    ADD_CODE(make_enter_insn, syn->fdef_stackframe_size);
    COPY_CODE(syn->fdef_body);
    ADD_CODE(make_basic_insn, II_LEAVE);
    FINALIZE_LINEARIZE;
}

void linearize_compound_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, stmt, syn->cstmt_block_items)
        COPY_CODE(stmt);
    FINALIZE_LINEARIZE;
}

void linearize_return_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    regid_t reg = INVALID_VREGID;
    if (syn->retstmt_expression)
    {
        COPY_CODE(syn->retstmt_expression);
        reg = syn->retstmt_expression->result_register;
        ADD_CODE(make_return_insn, syn->retstmt_expression->ctype, reg);
    }
    ADD_CODE(make_basic_insn, II_LEAVE);
    FINALIZE_LINEARIZE;
}

void linearize_expression_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->estmt_expression);
    FINALIZE_LINEARIZE;
}

void linearize_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, expr, syn->expr_expressions)
        COPY_CODE(expr);
    if (syn->expr_expressions)
    {
        syntax_component_t* last = vector_get(syn->expr_expressions, syn->expr_expressions->size - 1);
        syn->result_register = last->result_register;
    }
    FINALIZE_LINEARIZE;
}

void linearize_integer_constant_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_load_iconst_insn, syn->ctype, syn->result_register, syn->intc);
    FINALIZE_LINEARIZE;
}

void linearize_identifier_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = MAKE_REGISTER;
    symbol_t* sy = symbol_table_lookup(SYMBOL_TABLE, syn, syntax_get_namespace(syn));
    if (!sy) report_return;
    ADD_CODE(make_load_object_insn, syn->ctype, syn->result_register, sy->loc);
    FINALIZE_LINEARIZE;
}

void linearize_array_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = syn->adeclr_direct->result_register;
    COPY_CODE(syn->adeclr_direct);
    // TODO: probably need to alloca for VLAs
    FINALIZE_LINEARIZE;
}

void linearize_init_declarator_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syn->result_register = MAKE_REGISTER;
    ir_insn_t* insn = syn->ideclr_declarator->code;
    locator_t* loc = insn->loc;
    for (; insn->next; insn = insn->next, loc = insn->loc);
    COPY_CODE(syn->ideclr_initializer);
    if (syn->ideclr_initializer)
        ADD_CODE(make_store_address_insn, syn->ideclr_declarator->ctype, syn->result_register, loc, syn->ideclr_initializer->result_register);
    FINALIZE_LINEARIZE;
}

void linearize_assignment_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_rhs);
    syn->result_register = MAKE_REGISTER;
    ir_insn_t* insn = syn->bexpr_lhs->code;
    locator_t* loc = insn->loc;
    for (; insn->next; insn = insn->next, loc = insn->loc);
    ADD_CODE(make_store_address_insn, syn->ctype, syn->result_register, loc, syn->bexpr_rhs->result_register);
    FINALIZE_LINEARIZE;
}

void linearize_declaration_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    VECTOR_FOR(syntax_component_t*, ideclr, syn->decl_init_declarators)
        COPY_CODE(ideclr);
    FINALIZE_LINEARIZE;
}

static ir_insn_type_t binary_expression_type_map(syntax_component_type_t type)
{
    switch (type)
    {
        case SC_ADDITION_EXPRESSION:
            return II_ADDITION;
        default:
            report_return_value(II_UNKNOWN);
    }
}

void linearize_binary_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->bexpr_lhs);
    COPY_CODE(syn->bexpr_rhs);
    syn->result_register = MAKE_REGISTER;
    ADD_CODE(make_binary_expression_insn, syn->ctype, binary_expression_type_map(syn->type), syn->result_register, syn->bexpr_lhs->result_register, syn->bexpr_rhs->result_register);
    FINALIZE_LINEARIZE;
}

void linearize_subscript_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    syntax_component_t* lhs = syn->subsexpr_expression;
    syntax_component_t* rhs = syn->subsexpr_index_expression;
    COPY_CODE(lhs);
    COPY_CODE(rhs);
    syn->result_register = MAKE_REGISTER;
    locator_t* loc = calloc(1, sizeof *loc);
    loc->type = L_ARRAY;
    if (lhs->ctype->class == CTC_ARRAY)
    {
        loc->array.scale = type_size(lhs->ctype->derived_from);
        loc->array.offset_reg = rhs->result_register;
        loc->array.base_reg = lhs->result_register;
    }
    else
    {
        loc->array.scale = type_size(rhs->ctype->derived_from);
        loc->array.offset_reg = lhs->result_register;
        loc->array.base_reg = rhs->result_register;
    }
    ADD_CODE(make_subscript_insn, syn->ctype, loc, syn->result_register, lhs->result_register, rhs->result_register);
    FINALIZE_LINEARIZE;
}

void linearize_if_statement_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    // compare
    COPY_CODE(syn->ifstmt_condition);
    // jump if zero to else
    char* else_label = make_local_label(trav);
    char* end_label = make_local_label(trav);
    ADD_CODE(make_condition_jump_insn, II_JUMP_IF_ZERO, syn->ifstmt_condition->result_register, else_label);
    // if body
    COPY_CODE(syn->ifstmt_body);
    if (syn->ifstmt_else)
    {
        // unconditional jump to after
        ADD_CODE(make_jump_insn, end_label);
    }
    // label to start else body
    ADD_CODE(make_label_insn, else_label);
    if (syn->ifstmt_else)
    {
        // else body
        COPY_CODE(syn->ifstmt_else);
        // label for end of if statement
        ADD_CODE(make_label_insn, end_label);
    }
    free(else_label);
    free(end_label);
    FINALIZE_LINEARIZE;
}

void linearize_function_call_expression_after(syntax_traverser_t* trav, syntax_component_t* syn)
{
    SETUP_LINEARIZE;
    COPY_CODE(syn->fcallexpr_expression);
    long long noargs = syn->fcallexpr_args->size;
    regid_t* regargs = calloc(noargs, sizeof(regid_t));
    for (long long i = noargs - 1; i >= 0; --i)
    {
        syntax_component_t* arg = vector_get(syn->fcallexpr_args, i);
        COPY_CODE(arg);
        regargs[i] = arg->result_register;
    }
    ADD_CODE(make_function_call_insn, syn->ctype, syn->result_register = MAKE_REGISTER, syn->fcallexpr_expression->result_register, regargs, noargs);
    FINALIZE_LINEARIZE;
}

ir_insn_t* linearize(syntax_component_t* tlu)
{
    if (!tlu) return NULL;
    syntax_traverser_t* trav = traverse_init(tlu, sizeof(linearizing_syntax_traverser_t));

    trav->after[SC_TRANSLATION_UNIT] = linearize_translation_unit_after;
    trav->after[SC_FUNCTION_DEFINITION] = linearize_function_definition_after;
    trav->after[SC_COMPOUND_STATEMENT] = linearize_compound_statement_after;
    trav->after[SC_RETURN_STATEMENT] = linearize_return_statement_after;
    trav->after[SC_EXPRESSION_STATEMENT] = linearize_expression_statement_after;
    trav->after[SC_EXPRESSION] = linearize_expression_after;
    trav->after[SC_INTEGER_CONSTANT] = linearize_integer_constant_after;
    trav->after[SC_IDENTIFIER] = linearize_identifier_after;
    trav->after[SC_INIT_DECLARATOR] = linearize_init_declarator_after;
    trav->after[SC_ARRAY_DECLARATOR] = linearize_array_declarator_after;
    trav->after[SC_DECLARATION] = linearize_declaration_after;
    trav->after[SC_ADDITION_EXPRESSION] = linearize_binary_expression_after;
    trav->after[SC_ASSIGNMENT_EXPRESSION] = linearize_assignment_expression_after;
    trav->after[SC_SUBSCRIPT_EXPRESSION] = linearize_subscript_expression_after;
    trav->after[SC_IF_STATEMENT] = linearize_if_statement_after;
    trav->after[SC_FUNCTION_CALL_EXPRESSION] = linearize_function_call_expression_after;

    traverse(trav);
    traverse_delete(trav);
    return tlu->code;
}
