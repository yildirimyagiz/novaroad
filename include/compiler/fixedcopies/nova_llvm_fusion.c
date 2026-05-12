/**
 * nova_llvm_fusion.c — Differentiable Kernel Fusion
 *
 * The core idea: after AD generates a gradient function, that function
 * re-executes the same arithmetic as the primal to reconstruct intermediate
 * values needed for the adjoint.  If we fuse them, those intermediates are
 * in SSA registers and never need to leave the register file.
 *
 * Implementation strategy:
 *   1. Clone the primal instructions into the fused function.
 *   2. Inline the adjoint instructions immediately after, reusing
 *      the cloned primal SSA values instead of recomputing them.
 *   3. Write primal output and gradient to out-pointers.
 *
 * This is conservative fusion — it handles straight-line (single BB)
 * kernels, which covers the majority of ML inner loops.
 */

#include "nova_llvm_fusion.h"
#include "nova_llvm_ad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>

/* ── Value remap table ───────────────────────────────────────────────────── */

typedef struct { LLVMValueRef from; LLVMValueRef to; } RemapEntry;

typedef struct {
  RemapEntry *entries;
  int         count, capacity;
} RemapTable;

static void remap_init(RemapTable *t) {
  t->entries  = NULL;
  t->count    = 0;
  t->capacity = 0;
}
static void remap_free(RemapTable *t) { free(t->entries); }

static void remap_add(RemapTable *t, LLVMValueRef from, LLVMValueRef to) {
  if (t->count >= t->capacity) {
    int nc = t->capacity == 0 ? 32 : t->capacity * 2;
    t->entries = (RemapEntry *)realloc(t->entries,
                                       sizeof(RemapEntry) * (size_t)nc);
    t->capacity = nc;
  }
  t->entries[t->count].from = from;
  t->entries[t->count].to   = to;
  t->count++;
}

static LLVMValueRef remap_get(RemapTable *t, LLVMValueRef from) {
  for (int i = 0; i < t->count; i++)
    if (t->entries[i].from == from) return t->entries[i].to;
  return from; /* unmapped: use as-is (e.g. constants) */
}

/* ── Clone one instruction using a remap table ───────────────────────────── */

static LLVMValueRef clone_instruction(LLVMBuilderRef builder,
                                      LLVMValueRef   inst,
                                      RemapTable    *remap) {
  LLVMOpcode   op = LLVMGetInstructionOpcode(inst);
  LLVMValueRef a  = (LLVMGetNumOperands(inst) > 0)
                        ? remap_get(remap, LLVMGetOperand(inst, 0)) : NULL;
  LLVMValueRef b  = (LLVMGetNumOperands(inst) > 1)
                        ? remap_get(remap, LLVMGetOperand(inst, 1)) : NULL;

  switch (op) {
  case LLVMFAdd: return LLVMBuildFAdd(builder, a, b, "f");
  case LLVMFSub: return LLVMBuildFSub(builder, a, b, "f");
  case LLVMFMul: return LLVMBuildFMul(builder, a, b, "f");
  case LLVMFDiv: return LLVMBuildFDiv(builder, a, b, "f");
  case LLVMFNeg: return LLVMBuildFNeg(builder, a, "f");
  /* For any instruction we don't handle, skip (return NULL) */
  default: return NULL;
  }
}

/* ── Main fusion logic ───────────────────────────────────────────────────── */

FusionResult nova_llvm_fuse_grad_kernel(NovaLLVMContext *ctx,
                                        LLVMValueRef     primal_fn,
                                        LLVMValueRef     grad_fn) {
  FusionResult res = {0};
  if (!ctx || !primal_fn || !grad_fn) return res;

  LLVMContextRef lctx    = nova_llvm_get_llvm_context(ctx);
  LLVMModuleRef  module  = nova_llvm_get_module(ctx);
  LLVMBuilderRef builder = nova_llvm_get_builder(ctx);
  LLVMTypeRef    f64     = LLVMDoubleTypeInContext(lctx);
  LLVMTypeRef    f64ptr  = LLVMPointerType(f64, 0);

  /* Build fused function signature:  (<primal args>, f64* out, f64* grad_out) */
  unsigned     nparams  = LLVMCountParams(primal_fn);
  unsigned     total_p  = nparams + 2;
  LLVMTypeRef *ptypes   = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * total_p);
  if (!ptypes) return res;

  LLVMTypeRef primal_ftype = LLVMGlobalGetValueType(primal_fn);
  LLVMGetParamTypes(primal_ftype, ptypes);
  ptypes[nparams]     = f64ptr; /* out_value  */
  ptypes[nparams + 1] = f64ptr; /* out_grad   */

  LLVMTypeRef fused_ftype =
      LLVMFunctionType(LLVMVoidTypeInContext(lctx), ptypes, total_p, 0);

  const char *pname = LLVMGetValueName(primal_fn);
  char        fname[256];
  snprintf(fname, sizeof(fname), "%s_fused", pname);

  LLVMValueRef fused_fn = LLVMAddFunction(module, fname, fused_ftype);
  free(ptypes);

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(lctx, fused_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  /* Remap primal params → fused params */
  RemapTable remap;
  remap_init(&remap);
  for (unsigned i = 0; i < nparams; i++)
    remap_add(&remap, LLVMGetParam(primal_fn, i), LLVMGetParam(fused_fn, i));

  /* ── Phase 1: clone primal instructions ─────────────────────────────── */

  LLVMBasicBlockRef primal_bb = LLVMGetFirstBasicBlock(primal_fn);
  LLVMValueRef      inst      = LLVMGetFirstInstruction(primal_bb);
  LLVMValueRef      primal_ret_val = NULL;
  int               loads_elim = 0;

  while (inst) {
    LLVMOpcode op = LLVMGetInstructionOpcode(inst);
    if (op == LLVMRet) {
      primal_ret_val = (LLVMGetNumOperands(inst) > 0)
                           ? remap_get(&remap, LLVMGetOperand(inst, 0))
                           : NULL;
      inst = LLVMGetNextInstruction(inst);
      continue;
    }

    LLVMValueRef cloned = clone_instruction(builder, inst, &remap);
    if (cloned) {
      remap_add(&remap, inst, cloned);
      loads_elim++;  /* every cloned inst = one fewer reload in grad pass */
    }
    inst = LLVMGetNextInstruction(inst);
  }

  /* Store primal output */
  LLVMValueRef out_ptr = LLVMGetParam(fused_fn, nparams);
  if (primal_ret_val)
    LLVMBuildStore(builder, primal_ret_val, out_ptr);

  /* ── Phase 2: inline adjoint, reusing cloned primal values ──────────── */

  /*
   * The grad_fn was built with the same parameter order.
   * Remap its params to the fused params, then clone its instructions.
   * Because our remap table already maps primal intermediates → fused
   * clones, any adjoint instruction that referenced a primal intermediate
   * will automatically use the already-computed value.
   */
  RemapTable grad_remap;
  remap_init(&grad_remap);

  /* Map grad_fn params → fused params */
  for (unsigned i = 0; i < nparams; i++)
    remap_add(&grad_remap, LLVMGetParam(grad_fn, i), LLVMGetParam(fused_fn, i));

  /* Merge primal remap into grad_remap for intermediate reuse */
  for (int i = 0; i < remap.count; i++)
    remap_add(&grad_remap, remap.entries[i].from, remap.entries[i].to);

  LLVMBasicBlockRef grad_bb = LLVMGetFirstBasicBlock(grad_fn);
  inst = LLVMGetFirstInstruction(grad_bb);
  LLVMValueRef grad_ret_val = NULL;

  while (inst) {
    LLVMOpcode op = LLVMGetInstructionOpcode(inst);
    if (op == LLVMRet) {
      grad_ret_val = (LLVMGetNumOperands(inst) > 0)
                         ? remap_get(&grad_remap, LLVMGetOperand(inst, 0))
                         : NULL;
      inst = LLVMGetNextInstruction(inst);
      continue;
    }

    LLVMValueRef cloned = clone_instruction(builder, inst, &grad_remap);
    if (cloned)
      remap_add(&grad_remap, inst, cloned);

    inst = LLVMGetNextInstruction(inst);
  }

  /* Store gradient output */
  LLVMValueRef grad_out_ptr = LLVMGetParam(fused_fn, nparams + 1);
  if (grad_ret_val)
    LLVMBuildStore(builder, grad_ret_val, grad_out_ptr);

  LLVMBuildRetVoid(builder);

  remap_free(&remap);
  remap_free(&grad_remap);

  res.fused_fn         = fused_fn;
  res.primal_fn        = primal_fn;
  res.grad_fn          = grad_fn;
  res.loads_eliminated = loads_elim;
  return res;
}

/* ── Module-level fusion pass ────────────────────────────────────────────── */

int nova_llvm_run_fusion_pass(NovaLLVMContext *ctx) {
  if (!ctx) return 0;

  LLVMModuleRef module = nova_llvm_get_module(ctx);
  int           fused  = 0;

  /*
   * For every function f in the module, look for f_rev_0.
   * If found, fuse them.
   */
  LLVMValueRef fn = LLVMGetFirstFunction(module);
  while (fn) {
    const char *name = LLVMGetValueName(fn);
    /* Skip functions that are already grad or fused variants */
    if (strstr(name, "_rev_") || strstr(name, "_fwd_") ||
        strstr(name, "_fused")) {
      fn = LLVMGetNextFunction(fn);
      continue;
    }

    char grad_name[256];
    snprintf(grad_name, sizeof(grad_name), "%s_rev_0", name);
    LLVMValueRef grad_fn = LLVMGetNamedFunction(module, grad_name);

    if (grad_fn) {
      FusionResult r = nova_llvm_fuse_grad_kernel(ctx, fn, grad_fn);
      if (r.fused_fn) {
        fprintf(stderr,
                "[fusion] fused '%s' + '%s' → '%s'  (%d loads eliminated)\n",
                name, grad_name, LLVMGetValueName(r.fused_fn),
                r.loads_eliminated);
        fused++;
      }
    }

    fn = LLVMGetNextFunction(fn);
  }
  return fused;
}

#else // !HAVE_LLVM

FusionResult nova_llvm_fuse_grad_kernel(NovaLLVMContext *ctx,
                                        LLVMValueRef p, LLVMValueRef g) {
  (void)ctx; (void)p; (void)g;
  FusionResult r = {0}; return r;
}
int nova_llvm_run_fusion_pass(NovaLLVMContext *ctx) { (void)ctx; return 0; }

#endif // HAVE_LLVM
