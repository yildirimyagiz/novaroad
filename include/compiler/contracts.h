// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║  NOVA FORMAL VERIFICATION SYSTEM v2.0                                   ║
// ║  SMT-Based Correctness Checking and Proof Generation                    ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_CONTRACTS_H
#define NOVA_CONTRACTS_H

#include "ast.h"
#include "diagnostics.h"
#include <stdbool.h>

// ══════════════════════════════════════════════════════════════════════════════
// FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════════════════════

typedef struct formal_verification_context formal_verification_context_t;
typedef struct verification_result verification_result_t;
typedef struct proof_result proof_result_t;
typedef struct verification_summary verification_summary_t;

// ══════════════════════════════════════════════════════════════════════════════
// PUBLIC API
// ══════════════════════════════════════════════════════════════════════════════

/// Create formal verification context
formal_verification_context_t *
formal_verification_create(nova_diag_collector_t *diag, int level);

/// Destroy formal verification context
void formal_verification_destroy(formal_verification_context_t *ctx);

/// Verify function contracts (require/ensure)
verification_result_t
formal_verify_function_contracts(formal_verification_context_t *ctx,
                                 nova_stmt_t *fn_def);

/// Verify loop invariants
verification_result_t
formal_verify_loop_invariants(formal_verification_context_t *ctx,
                              nova_stmt_t *loop_stmt);

/// Verify assertions
verification_result_t
formal_verify_assertions(formal_verification_context_t *ctx,
                         nova_expr_t *assert_expr);

/// Generate formal proof for a property
proof_result_t formal_generate_proof(formal_verification_context_t *ctx,
                                     nova_expr_t *property,
                                     const char *property_name);

/// Run formal verification on entire program
verification_summary_t formal_verify_program(formal_verification_context_t *ctx,
                                             nova_program_t *program);

// ══════════════════════════════════════════════════════════════════════════════
// VERIFICATION RESULT TYPES
// ══════════════════════════════════════════════════════════════════════════════

typedef enum verification_status {
  VERIFICATION_PASSED,
  VERIFICATION_FAILED,
  VERIFICATION_UNKNOWN,
  VERIFICATION_SKIPPED
} verification_status_t;

struct verification_result {
  verification_status_t status;
  const char *error_message;
};

typedef enum proof_status {
  PROOF_VALID,
  PROOF_INVALID,
  PROOF_UNKNOWN,
  PROOF_SKIPPED
} proof_status_t;

struct proof_result {
  proof_status_t status;
  const char *proof;
  const char *counterexample;
};

struct verification_summary {
  int total_functions;
  int passed_functions;
  int failed_functions;
  int unknown_functions;
};

// ══════════════════════════════════════════════════════════════════════════════
// SMT SOLVER INTERFACE
// ══════════════════════════════════════════════════════════════════════════════

typedef enum smt_solver_type {
  SMT_Z3,
  SMT_CVC5,
  SMT_YICES,
  SMT_ALT_ERGO
} smt_solver_type_t;

/// Initialize SMT solver
bool smt_solver_initialize(smt_solver_type_t type);

/// Shutdown SMT solver
void smt_solver_shutdown(void);

/// Check satisfiability of SMT formula
int smt_check_sat(const char *formula);

/// Get model from SMT solver
char *smt_get_model(void);

#endif /* NOVA_CONTRACTS_H */
