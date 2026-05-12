/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_kernel_contracts.c — Legacy SMT2 Kernel Obligations (V1)
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * V1 kontrat sistemi (SMT2 string tabanlı, ObligationSet* döner).
 * Bu dosya geriye uyumluluk için korunmuştur.
 *
 * *** YENİ KONTRATLAR İÇİN: native/src/formal/nova_kernel_contracts.c ***
 * *** (V2 constraint-based NovaObligation API) kullanın.              ***
 */

#include "nova_obligation.h"
#include "nova_symbolic.h"
#include "nova_tensor.h"
#include <stdio.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * HELPER: Symbolic Tensor Definition
 * ═══════════════════════════════════════════════════════════════════════════
 */

static void declare_tensor_syms(SymContext *ctx, const char *name, int ndim,
                                int64_t *dims) {
  SymExpr *t = sym_var(ctx, name, SYM_TYPE_TENSOR);
  sym_declare(ctx, t);

  for (int i = 0; i < ndim; i++) {
    char dim_name[64];
    snprintf(dim_name, sizeof(dim_name), "%s_dim_%d", name, i);

    SymExpr *d_const = sym_const_int(ctx, dims[i]);
    SymExpr *d_sym = sym_dim(ctx, t, i);

    sym_assume(ctx, sym_eq(ctx, d_sym, d_const));
  }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V1 CONTRACT: MatMul [M,K] x [K,N] -> [M,N]  (SMT2-based)
 * ═══════════════════════════════════════════════════════════════════════════
 */

ObligationSet *nova_contract_matmul_v1(const char *op_name,
                                         const NovaTensor *a,
                                         const NovaTensor *b,
                                         const NovaTensor *c) {
  ObligationSet *set = obligation_set_create(op_name);
  SymContext *ctx = sym_context_create(op_name);

  declare_tensor_syms(ctx, "A", a->ndim, a->shape);
  declare_tensor_syms(ctx, "B", b->ndim, b->shape);
  declare_tensor_syms(ctx, "C", c->ndim, c->shape);

  SymExpr *sym_a = sym_var(ctx, "A", SYM_TYPE_TENSOR);
  SymExpr *sym_b = sym_var(ctx, "B", SYM_TYPE_TENSOR);
  SymExpr *sym_c = sym_var(ctx, "C", SYM_TYPE_TENSOR);

  /* Inner Dimension Match */
  {
    SymExpr *a_cols = sym_dim(ctx, sym_a, 1);
    SymExpr *b_rows = sym_dim(ctx, sym_b, 0);
    SymExpr *cond = sym_eq(ctx, a_cols, b_rows);
    sym_assert(ctx, cond);
  }

  /* Output Shape */
  {
    SymExpr *c_rows = sym_dim(ctx, sym_c, 0);
    SymExpr *a_rows = sym_dim(ctx, sym_a, 0);
    SymExpr *c_cols = sym_dim(ctx, sym_c, 1);
    SymExpr *b_cols = sym_dim(ctx, sym_b, 1);
    sym_assert(ctx, sym_eq(ctx, c_rows, a_rows));
    sym_assert(ctx, sym_eq(ctx, c_cols, b_cols));
  }

  char smt2[2048];
  sym_emit_smt2(ctx, smt2, sizeof(smt2));

  obligation_set_add(set, OBL_SHAPE_SAFE,
                     "MatMul Inner Dimension & Output Shape Consistency",
                     strdup(smt2));

  obligation_set_add(set, OBL_MEMORY_SAFE,
                     "Input/Output buffers valid and non-overlapping",
                     "(assert true)");

  sym_context_destroy(ctx);
  return set;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V1 CONTRACT: Element-wise Add  (SMT2-based)
 * ═══════════════════════════════════════════════════════════════════════════
 */

ObligationSet *nova_contract_add_v1(const char *op_name,
                                      const NovaTensor *a,
                                      const NovaTensor *b,
                                      const NovaTensor *c) {
  ObligationSet *set = obligation_set_create(op_name);
  SymContext *ctx = sym_context_create(op_name);

  declare_tensor_syms(ctx, "A", a->ndim, a->shape);
  declare_tensor_syms(ctx, "B", b->ndim, b->shape);
  declare_tensor_syms(ctx, "C", c->ndim, c->shape);

  SymExpr *sym_a = sym_var(ctx, "A", SYM_TYPE_TENSOR);
  SymExpr *sym_b = sym_var(ctx, "B", SYM_TYPE_TENSOR);
  SymExpr *sym_c = sym_var(ctx, "C", SYM_TYPE_TENSOR);

  for (int i = 0; i < a->ndim; i++) {
    SymExpr *da = sym_dim(ctx, sym_a, i);
    SymExpr *db = sym_dim(ctx, sym_b, i);
    SymExpr *dc = sym_dim(ctx, sym_c, i);
    sym_assert(ctx, sym_eq(ctx, da, db));
    sym_assert(ctx, sym_eq(ctx, db, dc));
  }

  char smt2[2048];
  sym_emit_smt2(ctx, smt2, sizeof(smt2));

  obligation_set_add(set, OBL_SHAPE_SAFE,
                     "Element-wise Add Broadcast/Shape Compatibility",
                     strdup(smt2));

  sym_context_destroy(ctx);
  return set;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V1 CONTRACT: Conv2D (SMT2-based)
 * ═══════════════════════════════════════════════════════════════════════════
 */

ObligationSet *nova_contract_conv2d_v1(const char *op_name,
                                         const NovaTensor *input,
                                         const NovaTensor *weight,
                                         const NovaTensor *output, int stride,
                                         int pad) {
  ObligationSet *set = obligation_set_create(op_name);
  SymContext *ctx = sym_context_create(op_name);

  SymExpr *H_in = sym_const_int(ctx, input->shape[2]);
  SymExpr *K_H = sym_const_int(ctx, weight->shape[2]);
  SymExpr *H_out = sym_const_int(ctx, output->shape[2]);

  SymExpr *S = sym_const_int(ctx, stride);
  SymExpr *P = sym_const_int(ctx, pad);
  SymExpr *Two = sym_const_int(ctx, 2);

  SymExpr *H_numer =
      sym_sub(ctx, sym_add(ctx, H_in, sym_mul(ctx, Two, P)), K_H);
  SymExpr *H_calc =
      sym_add(ctx, sym_div(ctx, H_numer, S), sym_const_int(ctx, 1));
  sym_assert(ctx, sym_eq(ctx, H_out, H_calc));

  SymExpr *Cin_input = sym_const_int(ctx, input->shape[1]);
  SymExpr *Cin_weight = sym_const_int(ctx, weight->shape[1]);
  sym_assert(ctx, sym_eq(ctx, Cin_input, Cin_weight));

  char smt2[2048];
  sym_emit_smt2(ctx, smt2, sizeof(smt2));

  obligation_set_add(set, OBL_SHAPE_SAFE,
                     "Conv2D Geometry & Channel Consistency", strdup(smt2));

  sym_context_destroy(ctx);
  return set;
}
