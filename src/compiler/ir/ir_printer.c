/**
 * @file ir_printer.c
 * @brief IR pretty printer
 */

#include "compiler/ir.h"
#include <stdio.h>

void nova_ir_print_module(nova_ir_module_t *module)
{
    if (!module) return;
    
    printf("; Nova IR Module\n");
    printf("; Functions: %zu\n\n", module->num_functions);
    
    for (size_t i = 0; i < module->num_functions; i++) {
        nova_ir_function_t *func = &module->functions[i];
        printf("define %s(", func->name);
        
        // Print parameters
        for (size_t j = 0; j < func->num_params; j++) {
            printf("%%arg%zu", j);
            if (j < func->num_params - 1) printf(", ");
        }
        printf(") {\n");
        
        // Print basic blocks
        for (size_t j = 0; j < func->num_blocks; j++) {
            nova_ir_block_t *block = &func->blocks[j];
            printf("bb%zu:\n", j);
            
            // Print instructions
            for (size_t k = 0; k < block->num_instrs; k++) {
                nova_ir_instr_t *instr = &block->instrs[k];
                printf("  ");
                nova_ir_print_instruction(instr);
            }
            printf("\n");
        }
        
        printf("}\n\n");
    }
}

void nova_ir_print_instruction(nova_ir_instr_t *instr)
{
    if (!instr) return;
    
    switch (instr->opcode) {
        case IR_NOP:
            printf("nop\n");
            break;
        case IR_LOAD:
            printf("%%r%u = load %%r%u\n", instr->dest, instr->src1);
            break;
        case IR_STORE:
            printf("store %%r%u, %%r%u\n", instr->src1, instr->dest);
            break;
        case IR_ADD:
            printf("%%r%u = add %%r%u, %%r%u\n", instr->dest, instr->src1, instr->src2);
            break;
        case IR_SUB:
            printf("%%r%u = sub %%r%u, %%r%u\n", instr->dest, instr->src1, instr->src2);
            break;
        case IR_MUL:
            printf("%%r%u = mul %%r%u, %%r%u\n", instr->dest, instr->src1, instr->src2);
            break;
        case IR_DIV:
            printf("%%r%u = div %%r%u, %%r%u\n", instr->dest, instr->src1, instr->src2);
            break;
        case IR_CALL:
            printf("%%r%u = call @func%u(%%r%u)\n", instr->dest, instr->src1, instr->src2);
            break;
        case IR_RET:
            printf("ret %%r%u\n", instr->src1);
            break;
        case IR_BR:
            printf("br bb%u\n", instr->src1);
            break;
        case IR_BR_COND:
            printf("br %%r%u, bb%u, bb%u\n", instr->dest, instr->src1, instr->src2);
            break;
        default:
            printf("<unknown opcode %d>\n", instr->opcode);
    }
}

void nova_ir_print(nova_ir_module_t *module)
{
    nova_ir_print_module(module);
}
