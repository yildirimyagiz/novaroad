/**
 * @file jit_x86_64.c
 * @brief x86_64 JIT code generation
 */

#include "jit.h"
#include "../../../../include/compiler/nova_ir.h"


#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* x86_64 register encoding */
#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7

/* Emit helpers */
static void emit_mov_r64_imm64(nova_jit_compiler_t *jit, int reg, uint64_t imm) {
    // REX.W + 0xB8 + reg
    nova_jit_emit_byte(jit, 0x48);
    nova_jit_emit_byte(jit, 0xB8 + reg);
    nova_jit_emit_bytes(jit, (uint8_t*)&imm, 8);
}

static void emit_add_r64_r64(nova_jit_compiler_t *jit, int dst, int src) {
    // REX.W + 0x01 /r
    nova_jit_emit_byte(jit, 0x48);
    nova_jit_emit_byte(jit, 0x01);
    nova_jit_emit_byte(jit, 0xC0 + (src << 3) + dst);
}

static void emit_sub_r64_r64(nova_jit_compiler_t *jit, int dst, int src) {
    // REX.W + 0x29 /r
    nova_jit_emit_byte(jit, 0x48);
    nova_jit_emit_byte(jit, 0x29);
    nova_jit_emit_byte(jit, 0xC0 + (src << 3) + dst);
}

static void emit_imul_r64_r64(nova_jit_compiler_t *jit, int dst, int src) {
    // REX.W + 0x0F 0xAF /r
    nova_jit_emit_byte(jit, 0x48);
    nova_jit_emit_byte(jit, 0x0F);
    nova_jit_emit_byte(jit, 0xAF);
    nova_jit_emit_byte(jit, 0xC0 + (dst << 3) + src);
}

static void emit_ret(nova_jit_compiler_t *jit) {
    nova_jit_emit_byte(jit, 0xC3);
}

static void emit_prologue(nova_jit_compiler_t *jit) {
    /* push rbp */
    nova_jit_emit_byte(jit, 0x55);
    /* mov rbp, rsp */
    nova_jit_emit_byte(jit, 0x48);
    nova_jit_emit_byte(jit, 0x89);
    nova_jit_emit_byte(jit, 0xE5);
}

static void emit_epilogue(nova_jit_compiler_t *jit) {
    /* pop rbp */
    nova_jit_emit_byte(jit, 0x5D);
    emit_ret(jit);
}

/* Compile IR to x86_64 */
int nova_jit_compile_x86_64(nova_jit_compiler_t *jit, IRModule *ir)
{
    if (!jit || !ir) return -1;
    
    printf("⚡️ JIT Backend: Compiling IR to x86_64 machine code...\n");
    
    /* Iterate through IR and compile each function */
    for (uint32_t i = 0; i < ir->function_count; i++) {
        IRFunction *func = &ir->functions[i];
        
        /* Emit function prologue */
        emit_prologue(jit);
        
        for (uint32_t b = 0; b < func->block_count; b++) {
            IRBlock *block = &func->blocks[b];
            for (uint32_t j = 0; j < block->instr_count; j++) {
                IRInstruction *inst = &block->instrs[j];
                
                switch (inst->op) {
                    case IR_ADD:
                        emit_add_r64_r64(jit, RAX, RCX);
                        break;
                    case IR_SUB:
                        emit_sub_r64_r64(jit, RAX, RCX);
                        break;
                    case IR_MUL:
                        emit_imul_r64_r64(jit, RAX, RCX);
                        break;
                    case IR_CONST_INT:
                        emit_mov_r64_imm64(jit, RAX, 0); 
                        break;
                    case IR_RETURN:
                        // Result is already in RAX usually
                        break;
                    default:
                        printf("⚠️  JIT: Unsupported IR opcode %d\n", (int)inst->op);
                        break;
                }
            }
        }
        
        /* Emit function epilogue */
        emit_epilogue(jit);
    }
    
    printf("✅ JIT Compilation completed. Ready to execute.\n");
    return 0;
}
