#ifndef NOVA_FORMAL_H
#define NOVA_FORMAL_H

typedef struct NovaTensor NovaTensor;

#include <stdbool.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA FORMAL LAYER (GÖDEL) - Reasoning & Verification Engine
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  NOVA_FORMAL_STRICT,   // Full formal checks (Performance impact)
  NOVA_FORMAL_ADAPTIVE, // Smart verification (Default)
  NOVA_FORMAL_FAST,     // Minimal invariants
  NOVA_FORMAL_RESEARCH  // Symbolic heavy analysis
} NovaFormalMode;

typedef struct {
  NovaFormalMode mode;
  bool enable_symbolic;
  bool enable_kernel_veri;
  bool enable_optimizer_veri;
  double timeout_ms;
} NovaFormalConfig;

// Initialize Formal System
int nova_formal_init(NovaFormalConfig config);
void nova_formal_shutdown(void);

/* ═══════════════════════════════════════════════════════════════════════════
 * TENSOR & KERNEL VERIFICATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  bool shape_valid;
  bool bounds_valid;
  bool type_valid;
  bool determinism_guaranteed;
  const char *violation_msg;
} NovaFormalKernelReport;

NovaFormalKernelReport nova_formal_kernel_validate(const char *kernel_name,
                                                   NovaTensor **inputs,
                                                   int num_inputs,
                                                   NovaTensor **outputs,
                                                   int num_outputs);

/* Forward declaration */
#include "../compute/nova_graph_structs.h"

/**
 * Validate a full compute graph against formal policies.
 * Used by AOT Compiler and JIT.
 */
NovaFormalKernelReport nova_formal_graph_validate(const NovaIRGraph *graph);

/* ═══════════════════════════════════════════════════════════════════════════
 * COMPILER HOOKS
 * ═══════════════════════════════════════════════════════════════════════════
 */

/**
 * Check if a function is allowed to be compiled by the active policy.
 * Returns true if allowed, false if denied.
 */
bool nova_formal_compile_check(const char *func_name);

/* ═══════════════════════════════════════════════════════════════════════════
 * OPTIMIZER VERIFICATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  bool is_stable;
  bool no_numerical_drift;
  bool strategy_consistent;
  float stability_score;
} NovaFormalOptimizerReport;

NovaFormalOptimizerReport
nova_formal_optimizer_validate(const char *decision_id, void *optimizer_state);

/* ═══════════════════════════════════════════════════════════════════════════
 * INVARIANT VALIDATION
 * ═══════════════════════════════════════════════════════════════════════════
 */

bool nova_formal_check_invariant(const char *invariant_name, void *data);

/* ═══════════════════════════════════════════════════════════════════════════
 * SYMBOLIC ANALYSIS
 * ═══════════════════════════════════════════════════════════════════════════
 */

void nova_formal_symbolic_analyze(NovaTensor *t);

#endif // NOVA_FORMAL_H
