/**
 * nova_llvm_ad.h — Automatic Differentiation at IR Level
 *
 * Gradient Tape is a first-class IR data structure in Nova.
 * Both forward-mode (dual numbers) and reverse-mode (tape) AD
 * are implemented as LLVM IR transformations — not runtime objects.
 *
 * Design:
 *   Forward mode  → dual number pairs (value, tangent) in SSA registers
 *   Reverse mode  → adjoint functions generated alongside primal functions
 *   Fusion        → grad + primal share the same SSA graph (no extra mem traffic)
 */

#ifndef NOVA_LLVM_AD_H
#define NOVA_LLVM_AD_H

#include "nova_llvm_backend.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>

// ═══════════════════════════════════════════════════════════════════════════
// AD Mode
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  NOVA_AD_FORWARD,   /* dual-number forward mode  — cheap for scalar inputs  */
  NOVA_AD_REVERSE,   /* adjoint reverse mode       — cheap for scalar outputs */
} NovaADMode;

// ═══════════════════════════════════════════════════════════════════════════
// Gradient Tape  (IR-level, not runtime)
// ═══════════════════════════════════════════════════════════════════════════

/*
 * A TapeEntry records one differentiable operation performed on the primal
 * pass.  During the adjoint pass each entry is visited in reverse order and
 * the adjoint (∂output/∂input) is accumulated into the correct LLVM value.
 */
typedef struct {
  LLVMValueRef  output;        /* SSA value produced by this op          */
  LLVMValueRef  inputs[2];     /* operands (up to 2 for binary ops)      */
  int           input_count;
  enum {
    TAPE_ADD, TAPE_SUB, TAPE_MUL, TAPE_DIV,
    TAPE_NEG, TAPE_EXP, TAPE_LOG, TAPE_CALL,
  }             op;
  LLVMValueRef  saved_operands[2]; /* values needed for grad of non-linear ops */
} TapeEntry;

typedef struct {
  TapeEntry *entries;
  int        count;
  int        capacity;
  /* Per-value adjoint accumulator map (parallel arrays) */
  LLVMValueRef *primal_values;
  LLVMValueRef *adjoint_values;
  int           adj_count;
  int           adj_capacity;
} GradientTape;

// ═══════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * nova_llvm_ad_differentiate
 *
 * Given a primal LLVM function `primal_fn`, generate a new function
 * `<name>_grad` that returns the gradient of the scalar output with
 * respect to the `wrt_param_index`-th parameter.
 *
 * mode = NOVA_AD_FORWARD  → forward-mode (dual numbers, one pass)
 * mode = NOVA_AD_REVERSE  → reverse-mode (adjoint, efficient for f: Rⁿ→R)
 *
 * Returns the new gradient function, or NULL on failure.
 */
LLVMValueRef nova_llvm_ad_differentiate(NovaLLVMContext *ctx,
                                        LLVMValueRef     primal_fn,
                                        int              wrt_param_index,
                                        NovaADMode       mode);

/**
 * nova_llvm_ad_grad_call
 *
 * Emit an IR call to the gradient of `func_name` at the call site pointed to
 * by the builder inside `ctx`.  If the gradient function does not yet exist,
 * it is generated automatically (reverse mode by default).
 *
 * This is what codegen.c uses when it encounters a `grad(f, x)` expression.
 */
LLVMValueRef nova_llvm_ad_grad_call(NovaLLVMContext *ctx,
                                    const char      *func_name,
                                    LLVMValueRef    *args,
                                    int              arg_count);

/* Low-level tape helpers (used internally and by fusion pass) */
GradientTape *nova_ad_tape_create(void);
void          nova_ad_tape_destroy(GradientTape *tape);
void          nova_ad_tape_push(GradientTape *tape, TapeEntry entry);

#endif /* HAVE_LLVM */
#endif /* NOVA_LLVM_AD_H */
