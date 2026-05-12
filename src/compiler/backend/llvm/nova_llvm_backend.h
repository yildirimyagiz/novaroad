/**
 * nova_llvm_backend.h - The Core LLVM Engine for Nova Compiler
 *
 * "The Reality Engine" 🌌
 *
 * Features:
 * - High-Performance JIT Compilation (MCJIT / OrcJIT)
 * - Ahead-Of-Time (AOT) Compilation to Native Machine Code
 * - Multi-Target Support (x86_64, AArch64, RISC-V, WASM)
 *
 * Uses: LLVM C API (Modern)
 */

#ifndef NOVA_LLVM_BACKEND_H
#define NOVA_LLVM_BACKEND_H

#include "../../../../include/compiler/compiler.h"

#include <stdbool.h>
#include <stdint.h>

// LLVM C API Headers (only if available)
#ifdef HAVE_LLVM
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
// PassManagerBuilder removed for compatibility with LLVM 17+
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Core Types
// ═══════════════════════════════════════════════════════════════════════════

// Enums — guarded to avoid conflicts with compiler.h
// Redundant enums removed, now handled by compiler.h

typedef struct NovaLLVMContext NovaLLVMContext;

#ifdef HAVE_LLVM
struct NovaLLVMContext {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef engine;
  LLVMPassManagerRef mpm;
  char *target_triple;
  char *cpu_name;
  char *features;
  NovaOptLevel opt_level;
  NovaTargetArch target_arch;
};
#else
struct NovaLLVMContext {
  int dummy;
};
#endif

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

// Initialization & Cleanup
NovaLLVMContext *nova_llvm_create_context(const char *module_name);
void nova_llvm_destroy_context(NovaLLVMContext *ctx);

// Target Configuration
bool nova_llvm_configure_target(NovaLLVMContext *ctx, NovaTargetArch arch);
void nova_llvm_enable_optimizations(NovaLLVMContext *ctx, NovaOptLevel level);

// Code Generation & Execution
bool nova_llvm_verify_module(NovaLLVMContext *ctx);
void nova_llvm_dump_ir(NovaLLVMContext *ctx);
bool nova_llvm_compile_jit(NovaLLVMContext *ctx);
int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *func_name,
                               int64_t *args, int arg_count);

// Object/Assembly Emission
bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename);
bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename);
void nova_llvm_set_opt_level(NovaLLVMContext *ctx, int level);

// IR Generation Helpers (High-Level)
#ifdef HAVE_LLVM
LLVMValueRef nova_llvm_build_function(NovaLLVMContext *ctx, const char *name,
                                      LLVMTypeRef ret_type,
                                      LLVMTypeRef *param_types,
                                      int param_count);
LLVMValueRef nova_llvm_const_int(NovaLLVMContext *ctx, int64_t val);
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Code Generation (AST → LLVM IR)
// ═══════════════════════════════════════════════════════════════════════════

#include "../../../../include/compiler/ast.h"


// Generate LLVM IR from Nova program
bool nova_llvm_codegen_program(NovaLLVMContext *ctx, nova_program_t *program);

#endif // NOVA_LLVM_BACKEND_H
