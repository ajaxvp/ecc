#include <stdlib.h>

#include "cc.h"

#define emitf(fmt, ...) fprintf(d->out, (fmt), ## __VA_ARGS__)
#define emitif(fmt, ...) fprintf(d->out, "\t" fmt, ## __VA_ARGS__)

// emit function prototype:
// bool emit_[syntax component name](syntax_component_t* s, syntax_component_t* parent, emission_details_t* d)
// s: the component to emit
// parent: the component which needed to call this emit function to complete itself
// d: the details about this translation unit's emission

// bool emit_statement(syntax_component_t* s, syntax_component_t* parent, emission_details_t* d)
// {
//     if (s->type != SYNTAX_COMPONENT_STATEMENT) return false;
//     // TODO
//     return true;
// }

// bool emit_declaration(syntax_component_t* s, syntax_component_t* parent, emission_details_t* d)
// {
//     if (s->type != SYNTAX_COMPONENT_DECLARATION) return false;
//     bool stat = syntax_has_specifier_qualifier(s->sc1_specifiers_qualifiers, SPECIFIER_QUALIFIER_STORAGE_CLASS, STORAGE_CLASS_STATIC);
//     bool labeled = parent->type == SYNTAX_COMPONENT_TRANSLATION_UNIT || stat;
//     //unsigned size = get_declaration_size(s);
//     // TODO
//     return true;
// }

// bool emit_function_definition(syntax_component_t* s, syntax_component_t* parent, emission_details_t* d)
// {
//     if (s->type != SYNTAX_COMPONENT_FUNCTION_DEFINITION) return false;
//     if (s->sc9_function_declarator->sc3_type != DECLARATOR_IDENTIFIER) return false;
//     char* function_name = s->sc9_function_declarator->sc3_identifier;
//     if (!syntax_has_specifier_qualifier(s->sc9_function_specifiers_qualifiers, SPECIFIER_QUALIFIER_STORAGE_CLASS, STORAGE_CLASS_STATIC))
//         emitf(".global %s\n", function_name);
//     emitf("%s:\n", function_name);
//     emitif("pushq %%rbp\n");
//     emitif("movq %%rsp, %%rbp\n");
//     // TODO: stack allocation
//     // unsigned stackframe_size = 32; // shadow space
//     // for (unsigned i = 0; i < s->sc9_function_body->sc10_compound_block_items->size; ++i)
//     // {
//     //     syntax_component_t* block_item = vector_get(s->sc9_function_body->sc10_compound_block_items, i);
//     //     if (block_item->type != SYNTAX_COMPONENT_DECLARATION)
//     //         continue;
//     //     stackframe_size += get_declaration_size(block_item);
//     // }
//     // stackframe_size += 16 - (stackframe_size % 16); // align to 16 bytes
//     // emitif("subq $%d, %%rsp\n", stackframe_size);
//     for (unsigned i = 0; i < s->sc9_function_body->sc10_compound_block_items->size; ++i)
//     {
//         syntax_component_t* block_item = vector_get(s->sc9_function_body->sc10_compound_block_items, i);
//         if (block_item->type == SYNTAX_COMPONENT_DECLARATION)
//         {
//             if (!emit_declaration(block_item, s, d))
//                 return false;
//         }
//         else if (block_item->type == SYNTAX_COMPONENT_STATEMENT)
//         {
//             if (!emit_statement(block_item, s, d))
//                 return false;
//         }
//         else
//             return false;
//     }
//     // TODO: stack allocation
//     //emitif("addq $%d, %%rsp\n", stackframe_size);
//     emitif("popq %%rbp\n");
//     emitif("ret\n");
//     return true;
// }

// bool emit(syntax_component_t* unit, FILE* out)
// {
//     if (unit->type != SYNTAX_COMPONENT_TRANSLATION_UNIT) return false;
//     emission_details_t* d = calloc(1, sizeof *d);
//     d->unit = unit;
//     d->out = out;
//     for (unsigned i = 0; i < d->unit->sc0_external_declarations->size; ++i)
//     {
//         syntax_component_t* decl = vector_get(d->unit->sc0_external_declarations, i);
//         if (decl->type == SYNTAX_COMPONENT_DECLARATION)
//         {
//             if (!emit_declaration(decl, unit, d))
//                 return false;
//         }
//         else if (decl->type == SYNTAX_COMPONENT_FUNCTION_DEFINITION)
//         {
//             if (!emit_function_definition(decl, unit, d))
//                 return false;
//         }
//         else
//             return false;
//     }
//     free(d);
//     return true;
// }