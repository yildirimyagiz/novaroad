/**
 * nova_llvm_fusion.h — Differentiable Kernel Fusion Pass
 *
 * Fuses a user kernel and its gradient into a single LLVM function,
 * eliminating redundant memory loads between the primal and adjoint passes.
 *
 * This is the IR-level mechanism that delivers the memory-traffic reduction
 * promised by fusing grad computation into the SSA graph.
 */

#ifndef NOVA_LLVM_FUSION_H
#define NOVA_LLVM_FUSION_H

#include "nova_llvm_backend.h"
#include <stdbool.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>

typedef struct {
  LLVMValueRef fused_fn;       /* the new fused function                  */
  LLVMValueRef primal_fn;      /* original primal (preserved, not deleted) */
  LLVMValueRef grad_fn;        /* original gradient function               */
  int          loads_eliminated; /* diagnostic count                       */
} FusionResult;

/**
 * nova_llvm_fuse_grad_kernel
 *
 * Given a primal kernel and its pre-generated gradient function, produce a
 * single fused function `<name>_fused` that:
 *   1. Computes the primal output.
 *   2. Reuses all intermediate SSA values for the adjoint pass — no reloads.
 *   3. Returns a {f64 value, f64 grad} struct (as two f64 out-params).
 *
 * The fused function signature:
 *   void <n>_fused(<args...>, f64 *out_value, f64 *out_grad)
 */
FusionResult nova_llvm_fuse_grad_kernel(NovaLLVMContext *ctx,
                                        LLVMValueRef     primal_fn,
                                        LLVMValueRef     grad_fn);

/**
 * nova_llvm_run_fusion_pass
 *
 * Scan the entire module for pairs (f, f_rev_0) and fuse each pair.
 * Returns the number of kernels fused.
 */
int nova_llvm_run_fusion_pass(NovaLLVMContext *ctx);

#endif /* HAVE_LLVM */
#endif /* NOVA_LLVM_FUSION_H */
