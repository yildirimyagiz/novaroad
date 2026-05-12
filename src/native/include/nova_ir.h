/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA CORE IR - P0 Week 1
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Design Principles (IMMUTABLE):
 * - IR is SSA-based
 * - IR is minimal but sufficient
 * - Every node knows its side effects
 * - Every node carries proof zone
 * - IR is target-agnostic (LLVM/WASM/native abstraction)
 */

#pragma once

#ifndef NOVA_IR_H
#define NOVA_IR_H

#include "nova_arena.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// IR IDENTIFIERS
// ═══════════════════════════════════════════════════════════════════════════

typedef uint32_t IRValueId;
typedef uint32_t IRBlockId;
typedef uint32_t IRFuncId;

// Forward declarations
typedef struct IRModule IRModule;
typedef struct IRFunction IRFunction;
typedef struct IRBlock IRBlock;
typedef struct IRInstruction IRInstruction;

#define IR_INVALID_VALUE 0xFFFFFFFF
#define IR_INVALID_BLOCK 0xFFFFFFFF
#define IR_INVALID_FUNC 0xFFFFFFFF

// ═══════════════════════════════════════════════════════════════════════════
// IR OPCODE (P0 CORE SET)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  IR_NOP,

  // Constants & values
  IR_CONST_INT,
  IR_CONST_FLOAT,
  IR_PARAM,

  // Arithmetic
  IR_ADD,
  IR_SUB,
  IR_MUL,
  IR_DIV,

  // Memory
  IR_ALLOCA,
  IR_LOAD,
  IR_STORE,

  // Control flow
  IR_JUMP,
  IR_BRANCH,
  IR_RETURN,

  // Calls
  IR_CALL,

} IROpcode;

// ═══════════════════════════════════════════════════════════════════════════
// IR TYPE SYSTEM (MINIMAL)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  IR_TYPE_I32,
  IR_TYPE_I64,
  IR_TYPE_F32,
  IR_TYPE_F64,
  IR_TYPE_PTR,
  IR_TYPE_VOID,
} IRType;

// ═══════════════════════════════════════════════════════════════════════════
// EFFECT & PROOF BINDING (CRITICAL)
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  IR_EFFECT_PURE = 0,
  IR_EFFECT_IO = 1 << 0,
  IR_EFFECT_MEMORY = 1 << 1,
  IR_EFFECT_UNSAFE = 1 << 2,
} IREffect;

typedef enum {
  IR_PROOF_VERIFIED,  // SMT proven
  IR_PROOF_TRUSTED,   // Assumed correct
  IR_PROOF_HEURISTIC, // Optimization zone
  IR_PROOF_UNKNOWN    // Beyond Gödel boundary
} IRProofLevel;

// ═══════════════════════════════════════════════════════════════════════════
// IR INSTRUCTION
// ═══════════════════════════════════════════════════════════════════════════

struct IRInstruction {
  IROpcode op;
  IRType type;

  IRValueId lhs;
  IRValueId rhs;
  IRValueId result;

  IREffect effects;
  IRProofLevel proof;
};

// ═══════════════════════════════════════════════════════════════════════════
// BASIC BLOCK
// ═══════════════════════════════════════════════════════════════════════════

struct IRBlock {
  IRBlockId id;
  IRInstruction *instrs;
  uint32_t instr_count;
  uint32_t instr_capacity;
  IRFunction *function; // Access to module/arena
};

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION IR
// ═══════════════════════════════════════════════════════════════════════════

struct IRFunction {
  IRFuncId id;
  const char *name;
  IRType return_type;

  IRBlock *blocks;
  uint32_t block_count;
  uint32_t block_capacity;

  bool is_runtime_free;
  IREffect combined_effects;
  IRProofLevel min_proof_level;
  IRModule *module; // Access to global arena
};

// ═══════════════════════════════════════════════════════════════════════════
// IR MODULE
// ═══════════════════════════════════════════════════════════════════════════

struct IRModule {
  Arena *arena; // Arena for all IR allocations
  IRFunction *functions;
  uint32_t function_count;
  uint32_t function_capacity;
};

// ═══════════════════════════════════════════════════════════════════════════
// IR CONSTRUCTION API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create IR module
 */
IRModule *ir_module_create(Arena *arena);

/**
 * Destroy IR module
 */
void ir_module_destroy(IRModule *mod);

/**
 * Create function in module
 */
IRFunction *ir_function_create(IRModule *mod, const char *name,
                               IRType return_type);

/**
 * Create basic block in function
 */
IRBlock *ir_block_create(IRFunction *fn);

/**
 * Append instruction to block
 */
void ir_block_append(IRBlock *block, IRInstruction instr);

// ═══════════════════════════════════════════════════════════════════════════
// IR INSTRUCTION BUILDERS
// ═══════════════════════════════════════════════════════════════════════════

IRInstruction ir_make_const_int(IRValueId result, int64_t value, IRType type);
IRInstruction ir_make_const_float(IRValueId result, double value, IRType type);
IRInstruction ir_make_add(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type);
IRInstruction ir_make_sub(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type);
IRInstruction ir_make_mul(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type);
IRInstruction ir_make_div(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type);
IRInstruction ir_make_load(IRValueId result, IRValueId ptr, IRType type);
IRInstruction ir_make_store(IRValueId ptr, IRValueId value);
IRInstruction ir_make_return(IRValueId value);
IRInstruction ir_make_call(IRValueId result, IRFuncId func, IRValueId *args,
                           uint32_t arg_count);

// ═══════════════════════════════════════════════════════════════════════════
// IR QUERY API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Get opcode name (for debugging)
 */
const char *ir_opcode_name(IROpcode op);

/**
 * Get type name
 */
const char *ir_type_name(IRType type);

/**
 * Get effect name
 */
const char *ir_effect_name(IREffect effect);

/**
 * Get proof level name
 */
const char *ir_proof_level_name(IRProofLevel level);

/**
 * Check if instruction has side effects
 */
bool ir_has_side_effects(const IRInstruction *instr);

/**
 * Check if instruction is pure
 */
bool ir_is_pure(const IRInstruction *instr);

// ═══════════════════════════════════════════════════════════════════════════
// IR PRINTING (DEBUG)
// ═══════════════════════════════════════════════════════════════════════════

void ir_print_instruction(const IRInstruction *instr);
void ir_print_block(const IRBlock *block);
void ir_print_function(const IRFunction *fn);
void ir_print_module(const IRModule *mod);

#endif // NOVA_IR_H
