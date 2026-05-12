/**
 * 🚀 NOVA PRO OPTIMIZER - Advanced Backend Optimizations (Phase 2)
 *
 * Implements:
 * - Polyhedral Loop Optimization (Tiling, Fusion)
 * - Enhanced SLP Vectorization (Pattern Matching)
 * - Whole-Program Dead Code Elimination
 */

#ifndef NOVA_OPTIMIZER_PRO_H
#define NOVA_OPTIMIZER_PRO_H

#include "nova_autotune.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>

// Forward declaration
typedef struct NovaLLVMOptimizer NovaLLVMOptimizer;

// ═══════════════════════════════════════════════════════════════════════════
// POLYHEDRAL LOOP OPTIMIZER
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaHardwareProfile *hw_profile;
  size_t tile_size_l1;
  size_t tile_size_l2;
  bool enable_fusion;
  bool enable_interchange;
} PolyhedralOptimizer;

PolyhedralOptimizer *polyhedral_optimizer_create(NovaHardwareProfile *hw);
void polyhedral_optimizer_destroy(PolyhedralOptimizer *opt);
void nova_optimize_loops_polyhedral(NovaLLVMOptimizer *opt,
                                      LLVMModuleRef module);

// ═══════════════════════════════════════════════════════════════════════════
// ENHANCED SLP VECTORIZER
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaHardwareProfile *hw;
  int max_vector_width;
  bool enable_fma;
  size_t patterns_matched;
} EnhancedSLPVectorizer;

EnhancedSLPVectorizer *slp_vectorizer_create(NovaHardwareProfile *hw);
void slp_vectorizer_destroy(EnhancedSLPVectorizer *slp);
void nova_enhance_slp_vectorization(NovaLLVMOptimizer *opt,
                                      LLVMModuleRef module);

// ═══════════════════════════════════════════════════════════════════════════
// WHOLE-PROGRAM OPTIMIZATION
// ═══════════════════════════════════════════════════════════════════════════

void nova_whole_program_dce(LLVMModuleRef module);

#endif // NOVA_OPTIMIZER_PRO_H
