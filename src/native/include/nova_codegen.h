/**
 * Nova LLVM Code Generator Header
 * Handles translation of AST to LLVM IR
 */

#ifndef NOVA_CODEGEN_H
#define NOVA_CODEGEN_H

#include "compiler/nova_ast.h"
#include "nova_optimizer.h"
#include "nova_policy.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

// CodeGen Context
typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef engine;

  // Symbol table for LLVM values (variables)
  // Keys: Variable names
  // Values: LLVMValueRef (as void*)
  void *named_values;

  // Current function being generated
  LLVMValueRef current_function;

  // Optimizer
  NovaLLVMOptimizer *optimizer;

  // Error tracking
  bool had_error;
  char error_message[256];

  // Formal Verification status
  // If true, we can skip runtime checks and use fast-math
  bool is_verified_context;

  // Per-Graph Optimization Policy
  NovaOptimizationConfig opt_config;
} CodeGen;

// API
/**
 * Create a new code generator
 */
CodeGen *codegen_create(const char *module_name, int opt_level);
void codegen_destroy(CodeGen *cg);

// Generation functions
bool codegen_generate(CodeGen *cg, ASTNode *app);
LLVMValueRef codegen_node(CodeGen *cg, ASTNode *node);

// Utility
void codegen_dump_ir(CodeGen *cg);
bool codegen_emit_object(CodeGen *cg, const char *filename);
bool codegen_emit_bitcode(CodeGen *cg, const char *filename);

#endif // NOVA_CODEGEN_H
