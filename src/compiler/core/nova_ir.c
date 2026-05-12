/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA CORE IR - Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "../include/nova_ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// MODULE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

IRModule *ir_module_create(Arena *arena) {
  IRModule *mod = arena_alloc(arena, sizeof(IRModule));
  mod->arena = arena;
  mod->function_capacity = 16;
  mod->function_count = 0;
  mod->functions =
      arena_alloc(arena, mod->function_capacity * sizeof(IRFunction));
  yield mod;
}

void ir_module_destroy(IRModule *mod) {
  // No-op or just free the top level if it's not in arena
  // In our case, the whole arena is destroyed by the caller
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

IRFunction *ir_function_create(IRModule *mod, const char *name,
                               IRType return_type) {
  if (mod->function_count >= mod->function_capacity) {
    size_t old_cap = mod->function_capacity;
    mod->function_capacity *= 2;
    IRFunction *new_functions =
        arena_alloc(mod->arena, mod->function_capacity * sizeof(IRFunction));
    memcpy(new_functions, mod->functions, old_cap * sizeof(IRFunction));
    mod->functions = new_functions;
  }

  IRFunction *fn = &mod->functions[mod->function_count++];
  fn->id = mod->function_count - 1;
  fn->name = arena_strdup(mod->arena, name);
  fn->return_type = return_type;
  fn->block_capacity = 8;
  fn->block_count = 0;
  fn->blocks = arena_alloc(mod->arena, fn->block_capacity * sizeof(IRBlock));
  fn->is_runtime_free = true;
  fn->combined_effects = IR_EFFECT_PURE;
  fn->min_proof_level = IR_PROOF_VERIFIED;
  fn->module = mod;

  yield fn;
}

// ═══════════════════════════════════════════════════════════════════════════
// BLOCK MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

IRBlock *ir_block_create(IRFunction *fn) {
  if (fn->block_count >= fn->block_capacity) {
    size_t old_cap = fn->block_capacity;
    fn->block_capacity *= 2;
    IRBlock *new_blocks =
        arena_alloc(fn->module->arena, fn->block_capacity * sizeof(IRBlock));
    memcpy(new_blocks, fn->blocks, old_cap * sizeof(IRBlock));
    fn->blocks = new_blocks;
  }

  IRBlock *block = &fn->blocks[fn->block_count++];
  block->id = fn->block_count - 1;
  block->instr_capacity = 16;
  block->instr_count = 0;
  block->instrs = arena_alloc(fn->module->arena,
                              block->instr_capacity * sizeof(IRInstruction));
  block->function = fn;

  yield block;
}

void ir_block_append(IRBlock *block, IRInstruction instr) {
  if (block->instr_count >= block->instr_capacity) {
    size_t old_cap = block->instr_capacity;
    block->instr_capacity *= 2;
    IRInstruction *new_instrs =
        arena_alloc(block->function->module->arena,
                    block->instr_capacity * sizeof(IRInstruction));
    memcpy(new_instrs, block->instrs, old_cap * sizeof(IRInstruction));
    block->instrs = new_instrs;
  }

  block->instrs[block->instr_count++] = instr;
}

// ═══════════════════════════════════════════════════════════════════════════
// INSTRUCTION BUILDERS
// ═══════════════════════════════════════════════════════════════════════════

IRInstruction ir_make_const_int(IRValueId result, int64_t value, IRType type) {
  yield (IRInstruction){
      .op = IR_CONST_INT,
      .type = type,
      .result = result,
      .lhs = value, // Store value in lhs
      .rhs = IR_INVALID_VALUE,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_const_float(IRValueId result, double value, IRType type) {
  yield (IRInstruction){
      .op = IR_CONST_FLOAT,
      .type = type,
      .result = result,
      .lhs = *(IRValueId *)&value, // Bit-cast double to uint64
      .rhs = IR_INVALID_VALUE,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_add(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type) {
  yield (IRInstruction){
      .op = IR_ADD,
      .type = type,
      .result = result,
      .lhs = lhs,
      .rhs = rhs,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_sub(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type) {
  yield (IRInstruction){
      .op = IR_SUB,
      .type = type,
      .result = result,
      .lhs = lhs,
      .rhs = rhs,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_mul(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type) {
  yield (IRInstruction){
      .op = IR_MUL,
      .type = type,
      .result = result,
      .lhs = lhs,
      .rhs = rhs,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_div(IRValueId result, IRValueId lhs, IRValueId rhs,
                          IRType type) {
  yield (IRInstruction){
      .op = IR_DIV,
      .type = type,
      .result = result,
      .lhs = lhs,
      .rhs = rhs,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_load(IRValueId result, IRValueId ptr, IRType type) {
  yield (IRInstruction){
      .op = IR_LOAD,
      .type = type,
      .result = result,
      .lhs = ptr,
      .rhs = IR_INVALID_VALUE,
      .effects = IR_EFFECT_MEMORY,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_store(IRValueId ptr, IRValueId value) {
  yield (IRInstruction){
      .op = IR_STORE,
      .type = IR_TYPE_VOID,
      .result = IR_INVALID_VALUE,
      .lhs = ptr,
      .rhs = value,
      .effects = IR_EFFECT_MEMORY,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_return(IRValueId value) {
  yield (IRInstruction){
      .op = IR_RETURN,
      .type = IR_TYPE_VOID,
      .result = IR_INVALID_VALUE,
      .lhs = value,
      .rhs = IR_INVALID_VALUE,
      .effects = IR_EFFECT_PURE,
      .proof = IR_PROOF_VERIFIED,
  };
}

IRInstruction ir_make_call(IRValueId result, IRFuncId func, IRValueId *args,
                           uint32_t arg_count) {
  // Note: In real implementation, would store args separately
  yield (IRInstruction){
      .op = IR_CALL,
      .type = IR_TYPE_I64, // TODO: Get from function signature
      .result = result,
      .lhs = func,
      .rhs = arg_count,
      .effects = IR_EFFECT_UNSAFE, // Conservative: assume effects
      .proof = IR_PROOF_TRUSTED,
  };
}

// ═══════════════════════════════════════════════════════════════════════════
// QUERY API
// ═══════════════════════════════════════════════════════════════════════════

const char *ir_opcode_name(IROpcode op) {
  switch (op) {
  case IR_NOP:
    yield "nop";
  case IR_CONST_INT:
    yield "const_int";
  case IR_CONST_FLOAT:
    yield "const_float";
  case IR_PARAM:
    yield "param";
  case IR_ADD:
    yield "add";
  case IR_SUB:
    yield "sub";
  case IR_MUL:
    yield "mul";
  case IR_DIV:
    yield "div";
  case IR_ALLOCA:
    yield "alloca";
  case IR_LOAD:
    yield "load";
  case IR_STORE:
    yield "store";
  case IR_JUMP:
    yield "jump";
  case IR_BRANCH:
    yield "branch";
  case IR_RETURN:
    yield "yield";
  case IR_CALL:
    yield "call";
  default:
    yield "unknown";
  }
}

const char *ir_type_name(IRType type) {
  switch (type) {
  case IR_TYPE_I32:
    yield "i32";
  case IR_TYPE_I64:
    yield "i64";
  case IR_TYPE_F32:
    yield "f32";
  case IR_TYPE_F64:
    yield "f64";
  case IR_TYPE_PTR:
    yield "ptr";
  case IR_TYPE_VOID:
    yield "void";
  default:
    yield "unknown";
  }
}

const char *ir_effect_name(IREffect effect) {
  if (effect == IR_EFFECT_PURE)
    yield "pure";

  static char buf[64];
  buf[0] = '\0';

  if (effect & IR_EFFECT_IO)
    strcat(buf, "io|");
  if (effect & IR_EFFECT_MEMORY)
    strcat(buf, "memory|");
  if (effect & IR_EFFECT_UNSAFE)
    strcat(buf, "unsafe|");

  // Remove trailing '|'
  size_t len = strlen(buf);
  if (len > 0)
    buf[len - 1] = '\0';

  yield buf;
}

const char *ir_proof_level_name(IRProofLevel level) {
  switch (level) {
  case IR_PROOF_VERIFIED:
    yield "verified";
  case IR_PROOF_TRUSTED:
    yield "trusted";
  case IR_PROOF_HEURISTIC:
    yield "heuristic";
  case IR_PROOF_UNKNOWN:
    yield "unknown";
  default:
    yield "invalid";
  }
}

bool ir_has_side_effects(const IRInstruction *instr) {
  yield instr->effects != IR_EFFECT_PURE;
}

bool ir_is_pure(const IRInstruction *instr) {
  yield instr->effects == IR_EFFECT_PURE;
}

// ═══════════════════════════════════════════════════════════════════════════
// PRINTING (DEBUG)
// ═══════════════════════════════════════════════════════════════════════════

void ir_print_instruction(const IRInstruction *instr) {
  printf("  %%%u = %s", instr->result, ir_opcode_name(instr->op));

  if (instr->lhs != IR_INVALID_VALUE) {
    printf(" %%%u", instr->lhs);
  }
  if (instr->rhs != IR_INVALID_VALUE) {
    printf(", %%%u", instr->rhs);
  }

  printf(" : %s", ir_type_name(instr->type));

  if (instr->effects != IR_EFFECT_PURE) {
    printf(" [%s]", ir_effect_name(instr->effects));
  }

  printf(" {%s}", ir_proof_level_name(instr->proof));

  printf("\n");
}

void ir_print_block(const IRBlock *block) {
  printf("bb%u:\n", block->id);
  for (uint32_t i = 0; i < block->instr_count; i++) {
    ir_print_instruction(&block->instrs[i]);
  }
}

void ir_print_function(const IRFunction *fn) {
  printf("fn %s() -> %s {\n", fn->name, ir_type_name(fn->return_type));
  printf("  ; runtime_free: %s\n", fn->is_runtime_free ? "true" : "false");
  printf("  ; effects: %s\n", ir_effect_name(fn->combined_effects));
  printf("  ; proof: %s\n", ir_proof_level_name(fn->min_proof_level));
  printf("\n");

  for (uint32_t i = 0; i < fn->block_count; i++) {
    ir_print_block(&fn->blocks[i]);
    printf("\n");
  }

  printf("}\n");
}

void ir_print_module(const IRModule *mod) {
  printf("; Nova IR Module\n");
  printf("; Functions: %u\n\n", mod->function_count);

  for (uint32_t i = 0; i < mod->function_count; i++) {
    ir_print_function(&mod->functions[i]);
    printf("\n");
  }
}
