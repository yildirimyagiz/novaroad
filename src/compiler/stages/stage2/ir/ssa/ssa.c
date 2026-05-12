#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../../../include/compiler/nova_ir.h"

/**
 * Stage 2 SSA Optimization & IR Visualization
 */

void ir_print_function(const IRFunction *f) {
    if (!f) return;
    printf("  [IR Viz] Function: %s\n", f->name ? f->name : "anonymous");
    
    for (uint32_t i = 0; i < f->block_count; i++) {
        IRBlock *bb = &f->blocks[i];
        printf("    Block %u:\n", bb->id);
        for (uint32_t j = 0; j < bb->instr_count; j++) {
            printf("      ");
            ir_print_instruction(&bb->instrs[j]);
            printf("\n");
        }
    }
}

void ir_print_instruction(const IRInstruction *instr) {
    if (!instr) return;
    printf("%s", ir_opcode_name(instr->op));
}

const char *ir_opcode_name(IROpcode op) {
    switch (op) {
        case IR_NOP: return "nop";
        case IR_CONST_INT: return "const_int";
        case IR_CONST_FLOAT: return "const_float";
        case IR_PARAM: return "param";
        case IR_ADD: return "add";
        case IR_SUB: return "sub";
        case IR_MUL: return "mul";
        case IR_DIV: return "div";
        case IR_ALLOCA: return "alloca";
        case IR_LOAD: return "load";
        case IR_STORE: return "store";
        case IR_JUMP: return "jump";
        case IR_BRANCH: return "branch";
        case IR_RETURN: return "return";
        case IR_CALL: return "call";
        default: return "unknown";
    }
}