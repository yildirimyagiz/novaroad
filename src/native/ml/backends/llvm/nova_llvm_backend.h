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

#include <stdbool.h>
#include <stdint.h>

// LLVM C API Headers
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
// PassManagerBuilder removed for compatibility with LLVM 17+

// ═══════════════════════════════════════════════════════════════════════════
// Core Types
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  LLVMContextRef context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMExecutionEngineRef engine;
  LLVMPassManagerRef mpm; // Module Pass Manager (Legacy)
  char *target_triple;
  char *cpu_name;
  char *features;
} NovaLLVMContext;

typedef enum {
  NOVA_OPT_NONE = 0,
  NOVA_OPT_O1 = 1,
  NOVA_OPT_O2 = 2,
  NOVA_OPT_O3 = 3,
  NOVA_OPT_Oz = 4
} NovaOptLevel;

typedef enum {
  NOVA_TARGET_NATIVE = 0,
  NOVA_TARGET_WASM = 1,
  NOVA_TARGET_RISCV = 2,
  NOVA_TARGET_ARM64 = 3,
  NOVA_TARGET_X86_64 = 4
} NovaTargetArch;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

// Initialization & Cleanup
NovaLLVMContext *nova_llvm_create_context(const char *module_name);
void nova_llvm_destroy_context(NovaLLVMContext *ctx);

// Target Configuration
bool nova_llvm_configure_target(NovaLLVMContext *ctx,
                                  NovaTargetArch arch);
void nova_llvm_enable_optimizations(NovaLLVMContext *ctx,
                                      NovaOptLevel level);

// Code Generation & Execution
bool nova_llvm_verify_module(NovaLLVMContext *ctx);
void nova_llvm_dump_ir(NovaLLVMContext *ctx);
bool nova_llvm_compile_jit(NovaLLVMContext *ctx);
int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *func_name,
                                 int64_t *args, int arg_count);

// IR Generation Helpers (High-Level)
LLVMValueRef nova_llvm_build_function(NovaLLVMContext *ctx,
                                        const char *name, LLVMTypeRef ret_type,
                                        LLVMTypeRef *param_types,
                                        int param_count);
LLVMValueRef nova_llvm_const_int(NovaLLVMContext *ctx, int64_t val);

#endif // NOVA_LLVM_BACKEND_H
