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

#include "compiler.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef HAVE_LLVM
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Core Types
// ═══════════════════════════════════════════════════════════════════════════

/*
 * FIX #2: Opaque forward declaration.
 * The full struct lives only in nova_llvm_backend.c — eliminates the
 * anonymous-struct vs named-struct mismatch across translation units.
 * All access goes through the accessor helpers below.
 */
typedef struct NovaLLVMContext NovaLLVMContext;

// ═══════════════════════════════════════════════════════════════════════════
// API
// ═══════════════════════════════════════════════════════════════════════════

// Initialization & Cleanup
NovaLLVMContext *nova_llvm_create_context(const char *module_name);
void             nova_llvm_destroy_context(NovaLLVMContext *ctx);

// Target Configuration
bool nova_llvm_configure_target(NovaLLVMContext *ctx, NovaTargetArch arch);
void nova_llvm_enable_optimizations(NovaLLVMContext *ctx, NovaOptLevel level);

// Code Generation & Execution
bool    nova_llvm_verify_module(NovaLLVMContext *ctx);
void    nova_llvm_dump_ir(NovaLLVMContext *ctx);
bool    nova_llvm_compile_jit(NovaLLVMContext *ctx);
int64_t nova_llvm_run_function(NovaLLVMContext *ctx, const char *func_name,
                               int64_t *args, int arg_count);

// Object/Assembly Emission
bool nova_llvm_emit_object(NovaLLVMContext *ctx, const char *filename);
bool nova_llvm_emit_assembly(NovaLLVMContext *ctx, const char *filename);
void nova_llvm_set_opt_level(NovaLLVMContext *ctx, int level);

// IR Generation Helpers
#ifdef HAVE_LLVM
LLVMValueRef nova_llvm_build_function(NovaLLVMContext *ctx, const char *name,
                                      LLVMTypeRef ret_type,
                                      LLVMTypeRef *param_types,
                                      int param_count);
LLVMValueRef nova_llvm_const_int(NovaLLVMContext *ctx, int64_t val);

/*
 * Accessor helpers — required now that the struct is opaque.
 * codegen.c uses these instead of ctx->llvm_ctx->context etc.
 */
LLVMContextRef nova_llvm_get_llvm_context(NovaLLVMContext *ctx);
LLVMModuleRef  nova_llvm_get_module(NovaLLVMContext *ctx);
LLVMBuilderRef nova_llvm_get_builder(NovaLLVMContext *ctx);
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Code Generation (AST → LLVM IR)
// ═══════════════════════════════════════════════════════════════════════════

#include "ast.h"

bool nova_llvm_codegen_program(NovaLLVMContext *ctx, nova_program_t *program);

#endif // NOVA_LLVM_BACKEND_H
