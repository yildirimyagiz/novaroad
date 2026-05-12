/**
 * @file jit_aarch64.c
 * @brief ARM64/AArch64 JIT code generation
 */

#include "jit.h"
#include "../../../../include/compiler/nova_ir.h"

/* ARM64 register encoding */
#define X0  0
#define X1  1
#define X2  2
#define X3  3
#define X29 29  /* FP */
#define X30 30  /* LR */
#define SP  31

/* Emit MOV Xd, #imm16 */
static void emit_mov_imm16(nova_jit_compiler_t *jit, uint8_t reg, uint16_t imm)
{
    /* MOVZ Xd, #imm16 */
    uint32_t instr = 0xD2800000 | (imm << 5) | reg;
    
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
}

/* Emit ADD Xd, Xn, Xm */
static void emit_add(nova_jit_compiler_t *jit, uint8_t rd, uint8_t rn, uint8_t rm)
{
    /* ADD Xd, Xn, Xm */
    uint32_t instr = 0x8B000000 | (rm << 16) | (rn << 5) | rd;
    
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
}

/* Emit SUB Xd, Xn, Xm */
static void emit_sub(nova_jit_compiler_t *jit, uint8_t rd, uint8_t rn, uint8_t rm)
{
    /* SUB Xd, Xn, Xm */
    uint32_t instr = 0xCB000000 | (rm << 16) | (rn << 5) | rd;
    
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
}

/* Emit RET */
static void emit_ret(nova_jit_compiler_t *jit)
{
    /* RET (return via X30/LR) */
    uint32_t instr = 0xD65F03C0;
    
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
}

/* Emit function prologue */
static void emit_prologue(nova_jit_compiler_t *jit)
{
    /* stp x29, x30, [sp, #-16]! */
    uint32_t instr = 0xA9BF7BFD;
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
    
    /* mov x29, sp */
    instr = 0x910003FD;
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
}

/* Emit function epilogue */
static void emit_epilogue(nova_jit_compiler_t *jit)
{
    /* mov sp, x29 */
    uint32_t instr = 0x910003BF;
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
    
    /* ldp x29, x30, [sp], #16 */
    instr = 0xA8C17BFD;
    nova_jit_emit_byte(jit, instr & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 8) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 16) & 0xFF);
    nova_jit_emit_byte(jit, (instr >> 24) & 0xFF);
    
    /* ret */
    emit_ret(jit);
}

/* Compile IR to ARM64 */
int nova_jit_compile_aarch64(nova_jit_compiler_t *jit, IRModule *ir)
{
    if (!jit || !ir) return -1;
    
    /* Emit function prologue */
    emit_prologue(jit);
    
    /* Simple example: return 42 */
    emit_mov_imm16(jit, X0, 42);
    
    /* Emit function epilogue */
    emit_epilogue(jit);
    
    return 0;
}
