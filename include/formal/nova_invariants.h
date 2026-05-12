/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA IR INVARIANTS - P0 Week 1
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * MANDATORY INVARIANTS (Cannot be violated):
 * 1. SSA form - Each IRValueId defined once
 * 2. Dominance - Value dominates its uses
 * 3. Type soundness - Operations have correct types
 * 4. Effect safety - PURE blocks contain no IO/MEMORY
 * 5. Proof boundary - VERIFIED cannot depend on HEURISTIC
 * 6. Single return - Function has single return block
 * 7. No uninitialized use - Every value defined before use
 *
 * ❌ IR invariant MUST pass before proceeding
 */

#ifndef NOVA_INVARIANTS_H
#define NOVA_INVARIANTS_H

#include "../compiler/nova_ir.h"

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  IR_INV_SSA_FORM,       // Each value defined once
  IR_INV_DOMINANCE,      // Value dominates uses
  IR_INV_TYPE_SOUNDNESS, // Correct types
  IR_INV_EFFECT_SAFETY,  // PURE cannot contain effects
  IR_INV_PROOF_BOUNDARY, // VERIFIED isolated from HEURISTIC
  IR_INV_SINGLE_RETURN,  // Single return block
  IR_INV_NO_UNINIT_USE,  // No uninitialized values
} IRInvariant;

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT VIOLATION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  IRInvariant invariant;
  const char *message;
  IRBlockId block_id;
  uint32_t instruction_index;
} IRInvariantViolation;

typedef struct {
  IRInvariantViolation *violations;
  uint32_t violation_count;
  uint32_t violation_capacity;
} IRInvariantReport;

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT CHECKING API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Check SSA form
 * Ensures each IRValueId is defined exactly once
 */
bool ir_check_ssa(const IRFunction *fn, IRInvariantReport *report);

/**
 * Check dominance
 * Ensures every value dominates its uses
 */
bool ir_check_dominance(const IRFunction *fn, IRInvariantReport *report);

/**
 * Check type soundness
 * Ensures operations have correct operand types
 */
bool ir_check_types(const IRFunction *fn, IRInvariantReport *report);

/**
 * Check effect safety
 * Ensures PURE code contains no IO/MEMORY effects
 */
bool ir_check_effects(const IRFunction *fn, IRInvariantReport *report);

/**
 * Check proof boundaries (CRITICAL - Gödel enforcement)
 * Ensures VERIFIED code cannot depend on HEURISTIC code
 *
 * RULE: VERIFIED node cannot use value from HEURISTIC node
 */
bool ir_check_proof_boundaries(const IRFunction *fn, IRInvariantReport *report);

/**
 * Check single return
 * Ensures function has exactly one return block
 */
bool ir_check_single_return(const IRFunction *fn, IRInvariantReport *report);

/**
 * Check no uninitialized use
 * Ensures all values are defined before use
 */
bool ir_check_no_uninit(const IRFunction *fn, IRInvariantReport *report);

/**
 * Verify ALL invariants
 *
 * Returns true if ALL invariants pass
 * ❌ If this returns false, IR is INVALID and cannot proceed
 */
bool ir_verify_all(const IRFunction *fn, IRInvariantReport *report);

// ═══════════════════════════════════════════════════════════════════════════
// VERIFICATION POINTS (WHERE TO CHECK)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * MANDATORY verification points:
 *
 * 1. After AST → IR lowering
 * 2. After each optimization/rewrite
 * 3. At heuristic zone exit
 * 4. Before target lowering (LLVM/WASM)
 * 5. Before deterministic build hash
 *
 * ❌ IR invariant MUST pass at ALL these points
 */

// ═══════════════════════════════════════════════════════════════════════════
// REPORT API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create invariant report
 */
IRInvariantReport *ir_report_create(void);

/**
 * Destroy report
 */
void ir_report_destroy(IRInvariantReport *report);

/**
 * Add violation to report
 */
void ir_report_add_violation(IRInvariantReport *report, IRInvariant invariant,
                             const char *message, IRBlockId block_id,
                             uint32_t instruction_index);

/**
 * Print report
 */
void ir_report_print(const IRInvariantReport *report);

/**
 * Get violation count
 */
uint32_t ir_report_violation_count(const IRInvariantReport *report);

/**
 * Check if report has violations
 */
bool ir_report_has_violations(const IRInvariantReport *report);

// ═══════════════════════════════════════════════════════════════════════════
// INVARIANT NAMES
// ═══════════════════════════════════════════════════════════════════════════

const char *ir_invariant_name(IRInvariant inv);

#endif // NOVA_INVARIANTS_H
