/**
 * nova_llvm_ad.c — Automatic Differentiation Implementation
 *
 * Forward mode  : each SSA value v gets a "tangent" v' = dv/d(input).
 *                 Implemented by cloning the primal function and replacing
 *                 every instruction with its dual counterpart.
 *
 * Reverse mode  : two-pass.
 *   Pass 1 (primal)  : execute the original function, record a tape.
 *   Pass 2 (adjoint) : walk the tape backwards, accumulate adjoints.
 *
 *   For purely static IR (no dynamic control flow on the differentiated
 *   path) the tape is unrolled at compile time into a straight-line adjoint
 *   function — zero runtime overhead.
 */

#include "nova_llvm_ad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LLVM
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

// ═══════════════════════════════════════════════════════════════════════════
// Tape Helpers
// ═══════════════════════════════════════════════════════════════════════════

GradientTape *nova_ad_tape_create(void) {
  GradientTape *t = (GradientTape *)calloc(1, sizeof(GradientTape));
  return t;
}

void nova_ad_tape_destroy(GradientTape *tape) {
  if (!tape) return;
  free(tape->entries);
  free(tape->primal_values);
  free(tape->adjoint_values);
  free(tape);
}

void nova_ad_tape_push(GradientTape *tape, TapeEntry entry) {
  if (tape->count >= tape->capacity) {
    int nc = tape->capacity == 0 ? 32 : tape->capacity * 2;
    tape->entries = (TapeEntry *)realloc(tape->entries,
                                         sizeof(TapeEntry) * (size_t)nc);
    tape->capacity = nc;
  }
  tape->entries[tape->count++] = entry;
}

/* Get (or create zero) adjoint for a primal value */
static LLVMValueRef tape_get_adjoint(GradientTape *tape, LLVMValueRef primal,
                                     LLVMBuilderRef builder,
                                     LLVMContextRef lctx) {
  for (int i = 0; i < tape->adj_count; i++) {
    if (tape->primal_values[i] == primal)
      return tape->adjoint_values[i];
  }
  /* Not found — return 0.0 */
  (void)builder;
  return LLVMConstReal(LLVMDoubleTypeInContext(lctx), 0.0);
}

static void tape_acc_adjoint(GradientTape *tape, LLVMValueRef primal,
                             LLVMValueRef delta,
                             LLVMBuilderRef builder,
                             LLVMContextRef lctx) {
  for (int i = 0; i < tape->adj_count; i++) {
    if (tape->primal_values[i] == primal) {
      tape->adjoint_values[i] =
          LLVMBuildFAdd(builder, tape->adjoint_values[i], delta, "adj_acc");
      return;
    }
  }
  /* New entry */
  if (tape->adj_count >= tape->adj_capacity) {
    int nc = tape->adj_capacity == 0 ? 32 : tape->adj_capacity * 2;
    tape->primal_values  = (LLVMValueRef *)realloc(tape->primal_values,
                                                    sizeof(LLVMValueRef) * (size_t)nc);
    tape->adjoint_values = (LLVMValueRef *)realloc(tape->adjoint_values,
                                                    sizeof(LLVMValueRef) * (size_t)nc);
    tape->adj_capacity = nc;
  }
  (void)lctx;
  tape->primal_values [tape->adj_count] = primal;
  tape->adjoint_values[tape->adj_count] = delta;
  tape->adj_count++;
}

// ═══════════════════════════════════════════════════════════════════════════
// Forward Mode — Dual Numbers
// ═══════════════════════════════════════════════════════════════════════════

/*
 * Build a function  `<name>_fwd_<param_idx>`  that returns the tangent
 * (derivative) of the scalar output with respect to parameter `wrt`.
 *
 * Strategy: walk every instruction in the primal entry block.
 * For each instruction emit both the primal and the tangent computation
 * into the new function's entry block.
 *
 * Tangent rules (f64 arithmetic):
 *   fadd  t = a + b   →  t' = a' + b'
 *   fsub  t = a - b   →  t' = a' - b'
 *   fmul  t = a * b   →  t' = a'*b + a*b'       (product rule)
 *   fdiv  t = a / b   →  t' = (a'*b - a*b')/b²  (quotient rule)
 */
static LLVMValueRef forward_mode_diff(NovaLLVMContext *ctx,
                                      LLVMValueRef     primal_fn,
                                      int              wrt) {
  LLVMContextRef lctx    = nova_llvm_get_llvm_context(ctx);
  LLVMModuleRef  module  = nova_llvm_get_module(ctx);
  LLVMBuilderRef builder = nova_llvm_get_builder(ctx);

  /* --- Build gradient function signature --- */
  unsigned      nparams   = LLVMCountParams(primal_fn);
  LLVMTypeRef  *ptypes    = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * nparams);
  if (!ptypes) return NULL;

  LLVMTypeRef primal_type = LLVMGlobalGetValueType(primal_fn);
  LLVMGetParamTypes(primal_type, ptypes);

  /* Grad function has the same signature as the primal */
  LLVMTypeRef  grad_ft = LLVMFunctionType(
      LLVMDoubleTypeInContext(lctx), ptypes, nparams, 0);

  /* Name: <primal>_fwd_<wrt> */
  const char *pname = LLVMGetValueName(primal_fn);
  char        gname[256];
  snprintf(gname, sizeof(gname), "%s_fwd_%d", pname, wrt);

  LLVMValueRef grad_fn = LLVMAddFunction(module, gname, grad_ft);
  free(ptypes);

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(lctx, grad_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  /*
   * Map: primal SSA value → {primal_clone, tangent}
   * We keep two parallel arrays for simplicity.
   */
  int           map_cap  = 64;
  LLVMValueRef *p_vals   = (LLVMValueRef *)calloc((size_t)map_cap, sizeof(LLVMValueRef));
  LLVMValueRef *t_vals   = (LLVMValueRef *)calloc((size_t)map_cap, sizeof(LLVMValueRef));
  int           map_count = 0;

  LLVMTypeRef f64 = LLVMDoubleTypeInContext(lctx);
  LLVMValueRef zero = LLVMConstReal(f64, 0.0);
  LLVMValueRef one  = LLVMConstReal(f64, 1.0);

  /* Seed parameters: tangent of wrt-th param = 1, others = 0 */
  for (unsigned i = 0; i < nparams; i++) {
    LLVMValueRef primal_param = LLVMGetParam(primal_fn, i);
    LLVMValueRef grad_param   = LLVMGetParam(grad_fn,   i);
    if (map_count < map_cap) {
      p_vals[map_count] = primal_param;
      t_vals[map_count] = ((int)i == wrt) ? one : zero;
      map_count++;
      /* Also map grad param to itself for the primal clone */
      (void)grad_param;
    }
  }

  /* Helper: look up tangent for a primal value */
#define TANGENT_OF(v) ({                                          \
    LLVMValueRef _t = zero;                                       \
    for (int _i = 0; _i < map_count; _i++)                        \
      if (p_vals[_i] == (v)) { _t = t_vals[_i]; break; }         \
    _t;                                                           \
  })

  /* Walk instructions in the primal entry block */
  LLVMBasicBlockRef primal_bb = LLVMGetFirstBasicBlock(primal_fn);
  LLVMValueRef      inst      = LLVMGetFirstInstruction(primal_bb);
  LLVMValueRef      last_val  = NULL;

  while (inst) {
    LLVMOpcode op = LLVMGetInstructionOpcode(inst);

    /* Map operands from primal space to grad function params */
    LLVMValueRef a = (LLVMGetNumOperands(inst) > 0)
                         ? LLVMGetOperand(inst, 0) : NULL;
    LLVMValueRef b = (LLVMGetNumOperands(inst) > 1)
                         ? LLVMGetOperand(inst, 1) : NULL;

    /* Remap to grad-function equivalents */
    for (unsigned i = 0; i < nparams; i++) {
      if (a == LLVMGetParam(primal_fn, i)) a = LLVMGetParam(grad_fn, i);
      if (b == LLVMGetParam(primal_fn, i)) b = LLVMGetParam(grad_fn, i);
    }

    LLVMValueRef primal_clone = NULL;
    LLVMValueRef tangent      = zero;

    switch (op) {
    case LLVMFAdd:
      primal_clone = LLVMBuildFAdd(builder, a, b, "p");
      tangent      = LLVMBuildFAdd(builder, TANGENT_OF(LLVMGetOperand(inst,0)),
                                            TANGENT_OF(LLVMGetOperand(inst,1)), "t");
      break;
    case LLVMFSub:
      primal_clone = LLVMBuildFSub(builder, a, b, "p");
      tangent      = LLVMBuildFSub(builder, TANGENT_OF(LLVMGetOperand(inst,0)),
                                            TANGENT_OF(LLVMGetOperand(inst,1)), "t");
      break;
    case LLVMFMul: {
      /* product rule: (a*b)' = a'*b + a*b' */
      LLVMValueRef da = TANGENT_OF(LLVMGetOperand(inst,0));
      LLVMValueRef db = TANGENT_OF(LLVMGetOperand(inst,1));
      primal_clone    = LLVMBuildFMul(builder, a, b, "p");
      LLVMValueRef t1 = LLVMBuildFMul(builder, da, b, "t1");
      LLVMValueRef t2 = LLVMBuildFMul(builder, a, db, "t2");
      tangent         = LLVMBuildFAdd(builder, t1, t2, "t");
      break;
    }
    case LLVMFDiv: {
      /* quotient rule: (a/b)' = (a'*b - a*b') / b² */
      LLVMValueRef da  = TANGENT_OF(LLVMGetOperand(inst,0));
      LLVMValueRef db  = TANGENT_OF(LLVMGetOperand(inst,1));
      primal_clone     = LLVMBuildFDiv(builder, a, b, "p");
      LLVMValueRef num = LLVMBuildFSub(builder,
                             LLVMBuildFMul(builder, da, b, ""),
                             LLVMBuildFMul(builder, a, db, ""), "num");
      LLVMValueRef b2  = LLVMBuildFMul(builder, b, b, "b2");
      tangent          = LLVMBuildFDiv(builder, num, b2, "t");
      break;
    }
    case LLVMRet:
      /* Return the tangent of the returned value */
      if (a) {
        LLVMValueRef ret_tangent = TANGENT_OF(LLVMGetOperand(inst,0));
        LLVMBuildRet(builder, ret_tangent);
      } else {
        LLVMBuildRetVoid(builder);
      }
      goto done;
    default:
      /* Non-differentiable instruction: clone as-is, tangent = 0 */
      primal_clone = NULL;
      tangent      = zero;
      break;
    }

    if (primal_clone && map_count < map_cap) {
      p_vals[map_count] = inst;
      t_vals[map_count] = tangent;
      map_count++;
      last_val = tangent;
    }

    inst = LLVMGetNextInstruction(inst);
  }

  /* Fallback: no explicit return found */
  if (last_val)
    LLVMBuildRet(builder, last_val);
  else
    LLVMBuildRet(builder, zero);

done:
  free(p_vals);
  free(t_vals);
#undef TANGENT_OF
  return grad_fn;
}

// ═══════════════════════════════════════════════════════════════════════════
// Reverse Mode — Adjoint Generation
// ═══════════════════════════════════════════════════════════════════════════

/*
 * Build `<name>_rev_<wrt>` using a compile-time-unrolled adjoint.
 *
 * For purely straight-line primal functions (no branches on differentiated
 * path) the tape is unrolled statically: every primal instruction gets an
 * adjoint instruction emitted in reverse order in the new function.
 *
 * Adjoint rules (∂L/∂x  notation, seed = 1.0 on output):
 *   y  = a + b   →  ā += ȳ,  b̄ += ȳ
 *   y  = a - b   →  ā += ȳ,  b̄ -= ȳ
 *   y  = a * b   →  ā += ȳ*b, b̄ += ȳ*a
 *   y  = a / b   →  ā += ȳ/b, b̄ -= ȳ*a/b²
 */
static LLVMValueRef reverse_mode_diff(NovaLLVMContext *ctx,
                                      LLVMValueRef     primal_fn,
                                      int              wrt) {
  LLVMContextRef lctx    = nova_llvm_get_llvm_context(ctx);
  LLVMModuleRef  module  = nova_llvm_get_module(ctx);
  LLVMBuilderRef builder = nova_llvm_get_builder(ctx);

  unsigned     nparams = LLVMCountParams(primal_fn);
  LLVMTypeRef *ptypes  = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * nparams);
  if (!ptypes) return NULL;
  LLVMTypeRef primal_type = LLVMGlobalGetValueType(primal_fn);
  LLVMGetParamTypes(primal_type, ptypes);

  LLVMTypeRef f64 = LLVMDoubleTypeInContext(lctx);
  LLVMTypeRef grad_ft = LLVMFunctionType(f64, ptypes, nparams, 0);

  const char *pname = LLVMGetValueName(primal_fn);
  char        gname[256];
  snprintf(gname, sizeof(gname), "%s_rev_%d", pname, wrt);

  LLVMValueRef grad_fn = LLVMAddFunction(module, gname, grad_ft);
  free(ptypes);

  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(lctx, grad_fn, "entry");
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef zero = LLVMConstReal(f64, 0.0);
  LLVMValueRef one  = LLVMConstReal(f64, 1.0);

  /* --- Pass 1: replay the primal into the new function, collect tape --- */
  GradientTape *tape = nova_ad_tape_create();
  if (!tape) { return NULL; }

  /* Maps primal instruction → replayed value in grad_fn */
  int           cap      = 128;
  LLVMValueRef *p_orig   = (LLVMValueRef *)calloc((size_t)cap, sizeof(LLVMValueRef));
  LLVMValueRef *p_replay = (LLVMValueRef *)calloc((size_t)cap, sizeof(LLVMValueRef));
  int           n_replay = 0;

  /* Seed parameters */
  for (unsigned i = 0; i < nparams; i++) {
    if (n_replay < cap) {
      p_orig  [n_replay] = LLVMGetParam(primal_fn, i);
      p_replay[n_replay] = LLVMGetParam(grad_fn,   i);
      n_replay++;
    }
  }

#define REPLAY(v) ({                                              \
    LLVMValueRef _r = (v);                                        \
    for (int _i = 0; _i < n_replay; _i++)                         \
      if (p_orig[_i] == (v)) { _r = p_replay[_i]; break; }       \
    _r;                                                           \
  })

  LLVMBasicBlockRef primal_bb = LLVMGetFirstBasicBlock(primal_fn);
  LLVMValueRef      inst      = LLVMGetFirstInstruction(primal_bb);
  LLVMValueRef      output_val = NULL;

  while (inst) {
    LLVMOpcode   op = LLVMGetInstructionOpcode(inst);
    LLVMValueRef a  = (LLVMGetNumOperands(inst) > 0) ? REPLAY(LLVMGetOperand(inst,0)) : NULL;
    LLVMValueRef b  = (LLVMGetNumOperands(inst) > 1) ? REPLAY(LLVMGetOperand(inst,1)) : NULL;
    LLVMValueRef replayed = NULL;

    switch (op) {
    case LLVMFAdd: replayed = LLVMBuildFAdd(builder, a, b, "r"); break;
    case LLVMFSub: replayed = LLVMBuildFSub(builder, a, b, "r"); break;
    case LLVMFMul: replayed = LLVMBuildFMul(builder, a, b, "r"); break;
    case LLVMFDiv: replayed = LLVMBuildFDiv(builder, a, b, "r"); break;
    case LLVMRet:
      output_val = a; /* the returned primal value */
      goto adjoint_pass;
    default:
      break;
    }

    if (replayed) {
      TapeEntry te = {0};
      te.output       = inst;     /* key: original SSA value     */
      te.inputs[0]    = (LLVMGetNumOperands(inst) > 0) ? LLVMGetOperand(inst,0) : NULL;
      te.inputs[1]    = (LLVMGetNumOperands(inst) > 1) ? LLVMGetOperand(inst,1) : NULL;
      te.input_count  = (op == LLVMFAdd || op == LLVMFSub ||
                         op == LLVMFMul || op == LLVMFDiv) ? 2 : 1;
      te.saved_operands[0] = a;
      te.saved_operands[1] = b;
      switch (op) {
      case LLVMFAdd: te.op = TAPE_ADD; break;
      case LLVMFSub: te.op = TAPE_SUB; break;
      case LLVMFMul: te.op = TAPE_MUL; break;
      case LLVMFDiv: te.op = TAPE_DIV; break;
      default:       te.op = TAPE_ADD; break;
      }
      nova_ad_tape_push(tape, te);

      if (n_replay < cap) {
        p_orig  [n_replay] = inst;
        p_replay[n_replay] = replayed;
        n_replay++;
      }
    }
    inst = LLVMGetNextInstruction(inst);
  }

adjoint_pass:
  /* --- Pass 2: emit adjoints in reverse tape order --- */

  /* Seed: adjoint of the output = 1.0 */
  if (output_val)
    tape_acc_adjoint(tape, output_val, one, builder, lctx);

  for (int i = tape->count - 1; i >= 0; i--) {
    TapeEntry   *te   = &tape->entries[i];
    LLVMValueRef adj_y = tape_get_adjoint(tape, te->output, builder, lctx);
    LLVMValueRef sa   = te->saved_operands[0];
    LLVMValueRef sb   = te->saved_operands[1];

    switch (te->op) {
    case TAPE_ADD:
      /* ā += adj_y,  b̄ += adj_y */
      tape_acc_adjoint(tape, te->inputs[0], adj_y, builder, lctx);
      tape_acc_adjoint(tape, te->inputs[1], adj_y, builder, lctx);
      break;
    case TAPE_SUB:
      /* ā += adj_y,  b̄ -= adj_y */
      tape_acc_adjoint(tape, te->inputs[0], adj_y, builder, lctx);
      tape_acc_adjoint(tape, te->inputs[1],
                       LLVMBuildFNeg(builder, adj_y, "neg"), builder, lctx);
      break;
    case TAPE_MUL:
      /* ā += adj_y * b,  b̄ += adj_y * a */
      if (sa && sb) {
        tape_acc_adjoint(tape, te->inputs[0],
                         LLVMBuildFMul(builder, adj_y, sb, "ga"), builder, lctx);
        tape_acc_adjoint(tape, te->inputs[1],
                         LLVMBuildFMul(builder, adj_y, sa, "gb"), builder, lctx);
      }
      break;
    case TAPE_DIV:
      /* ā += adj_y / b,  b̄ -= adj_y * a / b² */
      if (sa && sb) {
        tape_acc_adjoint(tape, te->inputs[0],
                         LLVMBuildFDiv(builder, adj_y, sb, "ga"), builder, lctx);
        LLVMValueRef b2  = LLVMBuildFMul(builder, sb, sb, "b2");
        LLVMValueRef num = LLVMBuildFMul(builder, adj_y, sa, "");
        LLVMValueRef gb  = LLVMBuildFNeg(builder,
                               LLVMBuildFDiv(builder, num, b2, ""), "gb");
        tape_acc_adjoint(tape, te->inputs[1], gb, builder, lctx);
      }
      break;
    default:
      break;
    }
  }

  /* Return the adjoint accumulated for the requested parameter */
  LLVMValueRef wrt_param = LLVMGetParam(primal_fn, (unsigned)wrt);
  LLVMValueRef result    = tape_get_adjoint(tape, wrt_param, builder, lctx);
  LLVMBuildRet(builder, result);

  nova_ad_tape_destroy(tape);
  free(p_orig);
  free(p_replay);
#undef REPLAY
  return grad_fn;
}

// ═══════════════════════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════════════════════

LLVMValueRef nova_llvm_ad_differentiate(NovaLLVMContext *ctx,
                                        LLVMValueRef     primal_fn,
                                        int              wrt_param_index,
                                        NovaADMode       mode) {
  if (!ctx || !primal_fn) return NULL;

  if (mode == NOVA_AD_FORWARD)
    return forward_mode_diff(ctx, primal_fn, wrt_param_index);
  else
    return reverse_mode_diff(ctx, primal_fn, wrt_param_index);
}

LLVMValueRef nova_llvm_ad_grad_call(NovaLLVMContext *ctx,
                                    const char      *func_name,
                                    LLVMValueRef    *args,
                                    int              arg_count) {
  if (!ctx || !func_name || arg_count < 1) return NULL;

  LLVMModuleRef  module  = nova_llvm_get_module(ctx);
  LLVMBuilderRef builder = nova_llvm_get_builder(ctx);
  LLVMContextRef lctx    = nova_llvm_get_llvm_context(ctx);

  /* Find primal function */
  LLVMValueRef primal = LLVMGetNamedFunction(module, func_name);
  if (!primal) {
    fprintf(stderr, "grad(): primal function '%s' not found\n", func_name);
    return NULL;
  }

  /*
   * Convention: grad(f, x) differentiates f w.r.t. the first parameter
   * whose name matches the variable in x, or w.r.t. parameter 0 by default.
   * For now: differentiate w.r.t. parameter 0, reverse mode.
   */
  int wrt = 0;

  /* Check whether gradient function was already generated */
  char gname[256];
  snprintf(gname, sizeof(gname), "%s_rev_%d", func_name, wrt);

  LLVMValueRef grad_fn = LLVMGetNamedFunction(module, gname);
  if (!grad_fn) {
    grad_fn = nova_llvm_ad_differentiate(ctx, primal, wrt, NOVA_AD_REVERSE);
    if (!grad_fn) {
      fprintf(stderr, "grad(): failed to differentiate '%s'\n", func_name);
      return NULL;
    }
  }

  /* Emit call to the gradient function */
  LLVMTypeRef grad_type = LLVMGlobalGetValueType(grad_fn);

  /* Pass the same arguments as the original call */
  LLVMValueRef *call_args = NULL;
  unsigned      nparams   = LLVMCountParams(grad_fn);
  if (nparams > 0) {
    call_args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * nparams);
    if (!call_args) return NULL;
    for (unsigned i = 0; i < nparams && (int)i < arg_count; i++)
      call_args[i] = args[i];
    /* If fewer args supplied, zero-fill */
    for (unsigned i = (unsigned)arg_count; i < nparams; i++)
      call_args[i] = LLVMConstReal(LLVMDoubleTypeInContext(lctx), 0.0);
  }

  LLVMValueRef result = LLVMBuildCall2(builder, grad_type, grad_fn,
                                       call_args, nparams, "grad_result");
  free(call_args);
  return result;
}

#else // !HAVE_LLVM

GradientTape *nova_ad_tape_create(void)              { return NULL; }
void          nova_ad_tape_destroy(GradientTape *t)  { (void)t; }
void          nova_ad_tape_push(GradientTape *t, TapeEntry e) { (void)t; (void)e; }

#endif // HAVE_LLVM
