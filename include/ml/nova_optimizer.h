/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA LLVM OPTIMIZER - Optimized Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_OPTIMIZER_H
#define NOVA_OPTIMIZER_H

#include "nova_autotune.h"
#if defined(HAVE_LLVM) && __has_include(<llvm-c/Core.h>)
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Transforms/PassBuilder.h>
#else
// Forward declarations for LLVM types
typedef struct LLVMOpaquePassBuilderOptions *LLVMPassBuilderOptionsRef;
typedef struct LLVMOpaqueModule *LLVMModuleRef;
typedef struct LLVMOpaqueValue *LLVMValueRef;
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// OPTIMIZATION LEVELS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  OPT_NONE = 0,      // No optimization
  OPT_BASIC = 1,     // Basic optimizations (O1)
  OPT_FULL = 2,      // Full optimizations (O2)
  OPT_AGGRESSIVE = 3 // Aggressive optimizations (O3)
} OptLevel;

// ═══════════════════════════════════════════════════════════════════════════
// OPTIMIZER STRUCTURE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct NovaLLVMOptimizer {
  LLVMPassBuilderOptionsRef pbo;
  OptLevel level;

  // Custom passes
  bool enable_tail_call_opt;
  bool enable_scalar_evolution;
  bool enable_loop_idiom;
  bool enable_auto_vectorization;
  bool enable_slp_vectorization;
  bool enable_aggressive_inst_combine;

  // Inlining configuration
  int inline_threshold;      // Default: 225
  int inline_hint_threshold; // Default: 325

  // Unrolling configuration
  int unroll_threshold;
  int unroll_count;
  bool unroll_allow_partial;
  bool unroll_allow_runtime;

  // Profiling
  const char *profile_data_path;
  bool use_pgo;

  // Statistics
  NovaAutotuner *autotuner;
  size_t functions_inlined;
  size_t loops_unrolled;
  size_t instructions_eliminated;
  double optimization_time_ms;
} NovaLLVMOptimizer;

// ═══════════════════════════════════════════════════════════════════════════
// CREATION & DESTRUCTION
// ═══════════════════════════════════════════════════════════════════════════

NovaLLVMOptimizer *nova_llvm_optimizer_create(OptLevel level);
void nova_llvm_optimizer_destroy(NovaLLVMOptimizer *opt);

// ═══════════════════════════════════════════════════════════════════════════
// MAIN OPTIMIZATION ENTRY POINT
// ═══════════════════════════════════════════════════════════════════════════

void nova_llvm_optimize_module(NovaLLVMOptimizer *opt,
                                 LLVMModuleRef module);
const char *nova_build_pass_pipeline(NovaLLVMOptimizer *opt);

// ═══════════════════════════════════════════════════════════════════════════
// CUSTOM NOVA PASSES
// ═══════════════════════════════════════════════════════════════════════════

void nova_optimize_tail_calls(NovaLLVMOptimizer *opt, LLVMModuleRef module);
void nova_aggressive_inst_combine(NovaLLVMOptimizer *opt,
                                    LLVMModuleRef module);
bool nova_is_recursive(LLVMValueRef func);

// ═══════════════════════════════════════════════════════════════════════════
// PROFILE-GUIDED OPTIMIZATION
// ═══════════════════════════════════════════════════════════════════════════

void nova_enable_pgo(NovaLLVMOptimizer *opt, const char *profile_path);
bool nova_generate_profile_instrumentation(NovaLLVMOptimizer *opt,
                                             LLVMModuleRef module);

// ═══════════════════════════════════════════════════════════════════════════
// PRO OPTIMIZER (PHASE 2)
// ═══════════════════════════════════════════════════════════════════════════

#include "nova_optimizer_pro.h"

#endif // NOVA_OPTIMIZER_H
