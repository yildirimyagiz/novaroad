#ifndef NOVA_DECISION_SYSTEM_H
#define NOVA_DECISION_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include "../ml/nova_cost_model.h"
#include "../compiler/nova_ir.h"
#include "../optimizer/nova_kernel_fusion.h"
#include "../formal/nova_proof.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA DECISION SYSTEM
 * ═══════════════════════════════════════════════════════════════════════════
 * The 'Brain' of the Nova Compiler.
 * Orchestrates fusion, lowering, and hardware mapping by balancing
 * performance goals and formal safety/precision proofs.
 */

typedef enum {
  DECISION_GOAL_LATENCY,    // Minimize time (Aggressive fusion, FP16)
  DECISION_GOAL_ACCURACY,   // Maximize precision (No FP16, bit-exact checks)
  DECISION_GOAL_THROUGHPUT, // Maximize total work (Multi-threading prioritized)
  DECISION_GOAL_DETERMINISTIC // Strictly deterministic (No async/parallel that
                              // reorders)
} NovaDecisionGoal;

typedef enum {
  DETERMINISM_NONE,
  DETERMINISM_REPRODUCIBLE, // Same output on same hardware
  DETERMINISM_BIT_EXACT // Identical output across different hardware (strict
                        // IEEE)
} NovaDeterminismLevel;

typedef enum {
  SYNTHESIS_NONE,
  SYNTHESIS_PROCEDURAL,  // RNG, Positional Encodings
  SYNTHESIS_RECONSTRUCT, // Low-rank (LoRA) or Sparse regeneration
  SYNTHESIS_RECURRENT    // Iterative recurrence (FMA-only)
} NovaSynthesisPolicy;

typedef struct {
  NovaDecisionGoal goal;
  bool allow_fp16;
  bool allow_fusion;
  bool allow_reassoc;   // (a+b)+c != a+(b+c)
  bool allow_contract;  // Allow FMA
  bool use_fast_math;   // bit-trick exp, approx reciprocal
  bool allow_synthesis; // Allow 'compute-over-memory' synthesis
  NovaDeterminismLevel determinism;
  NovaSynthesisPolicy synthesis_policy;
  ProofLevel min_proof_level;
  uint32_t max_parallel_heads;
} NovaDecisionContext;

// Initialize the decision engine
NovaDecisionContext *nova_decision_create(NovaDecisionGoal goal);

// Perform systematic optimization on an IR Graph
// This is the main entry point for 'Systemic Optimization Intelligence'
void nova_decision_optimize_graph(NovaIRGraph *graph,
                                    NovaDecisionContext *ctx,
                                    const NovaHardwareProfile *hw);

// Analyze a specific node and decide which kernel to use
NovaFusedOp nova_decision_select_kernel(NovaIRNode *node,
                                            NovaDecisionContext *ctx,
                                            const NovaHardwareProfile *hw);

#ifdef __cplusplus
}
#endif

#endif // NOVA_DECISION_SYSTEM_H
