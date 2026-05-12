/* nova_kernel_contracts_groq.c - Groq AI Optimized Kernel Contracts */
/* Original from formal/nova_kernel_contracts.c, adapted for Groq AI */

#include "nova_kernel_contracts.h"
#include "nova_graph_structs.h"
#include "nova_obligation.h"
#include "nova_tensor.h"
#include <string.h>

// Groq AI: Enerji monitörü ve delta processing için global vars
static double groq_energy_consumed = 0.0;
static int groq_delta_operations = 0;

// Groq AI: Enerji tracking fonksiyonu
void groq_track_energy(double watts, int ops) {
    groq_energy_consumed += watts * (ops / 1e9); // Tahmini
    groq_delta_operations += ops;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * HELPERS
 * ═══════════════════════════════════════════════════════════════════════════
 */

static int64_t z_tensor_numel(const NovaTensor *t) {
  if (!t)
    return 0;
  return (int64_t)t->total_elements;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: MatMul  A[M,K] x B[K,N] + Bias? -> Out[M,N]
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_matmul(const NovaTensor *A,
                                        const NovaTensor *B,
                                        const NovaTensor *Bias,
                                        NovaTensor *Out) {
  NovaObligation ob = nova_obligation_begin("matmul");
  nova_obligation_require_tensor(&ob, "A", A);
  nova_obligation_require_tensor(&ob, "B", B);
  nova_obligation_require_tensor(&ob, "Out", Out);

  // Groq AI: Matmul için enerji ve performans kontrolü
  groq_track_energy(50.0, A->total_elements * B->shape[1]); // Tahmini

  if (A && B && Out) {
    /* Both must be at least 2D */
    nova_obligation_require(&ob, "A_rank_ge_2", A->rank >= 2);
    nova_obligation_require(&ob, "B_rank_ge_2", B->rank >= 2);
    nova_obligation_require(&ob, "Out_rank_ge_2", Out->rank >= 2);

    if (A->rank >= 2 && B->rank >= 2) {
      /* Inner dimension: A.cols == B.rows */
      int64_t A_cols = A->shape[A->rank - 1];
      int64_t B_rows = B->shape[B->rank - 2];
      nova_obligation_require(&ob, "inner_dim_match", A_cols == B_rows);

      /* Output shape: [A.rows, B.cols] */
      int64_t A_rows = A->shape[A->rank - 2];
      int64_t B_cols = B->shape[B->rank - 1];
      if (Out->rank >= 2) {
        nova_obligation_require(&ob, "out_rows",
                                  Out->shape[Out->rank - 2] == A_rows);
        nova_obligation_require(&ob, "out_cols",
                                  Out->shape[Out->rank - 1] == B_cols);
      }
    }

    /* Optional Bias check */
    if (Bias) {
      nova_obligation_require(&ob, "bias_1d", Bias->rank == 1);
      if (Bias->rank == 1 && B->rank >= 1) {
        nova_obligation_require(&ob, "bias_dim",
                                  Bias->shape[0] == B->shape[B->rank - 1]);
      }
    }
  }
  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: Element-wise Add  A + B -> Out
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_add(const NovaTensor *A,
                                     const NovaTensor *B, NovaTensor *Out) {
  NovaObligation ob = nova_obligation_begin("add");
  nova_obligation_require_tensor(&ob, "A", A);
  nova_obligation_require_tensor(&ob, "B", B);
  nova_obligation_require_tensor(&ob, "Out", Out);

  // Groq AI: Add için delta processing
  groq_track_energy(10.0, A->total_elements);

  if (A && B && Out) {
    /* Broadcasting: ranks can differ, but dimensions must be compatible */
    int max_rank = A->rank > B->rank ? A->rank : B->rank;
    nova_obligation_require(&ob, "out_rank", Out->rank == max_rank);

    /* Check broadcast compatibility from right */
    for (int i = 0; i < max_rank; i++) {
      int64_t da = (i < A->rank) ? A->shape[A->rank - 1 - i] : 1;
      int64_t db = (i < B->rank) ? B->shape[B->rank - 1 - i] : 1;
      nova_obligation_require(&ob, "broadcast_compat",
                                da == db || da == 1 || db == 1);
    }
  }
  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: Conv2D  X[N,Ci,H,W] * W[Co,Ci,Kh,Kw] + Bias? -> Y[N,Co,Ho,Wo]
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_conv2d(const NovaTensor *X,
                                        const NovaTensor *W,
                                        const NovaTensor *Bias,
                                        NovaTensor *Y, int64_t stride_h,
                                        int64_t stride_w, int64_t pad_h,
                                        int64_t pad_w) {
  NovaObligation ob = nova_obligation_begin("conv2d");
  nova_obligation_require_tensor(&ob, "X", X);
  nova_obligation_require_tensor(&ob, "W", W);
  nova_obligation_require_tensor(&ob, "Y", Y);

  nova_obligation_require(&ob, "stride_h_pos", stride_h > 0);
  nova_obligation_require(&ob, "stride_w_pos", stride_w > 0);
  nova_obligation_require(&ob, "pad_h_nonneg", pad_h >= 0);
  nova_obligation_require(&ob, "pad_w_nonneg", pad_w >= 0);

  // Groq AI: Conv2D için yüksek enerji kontrolü
  groq_track_energy(100.0, X->total_elements * W->shape[0]);

  if (X && W && Y) {
    nova_obligation_require(&ob, "X_4d", X->rank == 4);
    nova_obligation_require(&ob, "W_4d", W->rank == 4);
    nova_obligation_require(&ob, "Y_4d", Y->rank == 4);

    if (X->rank == 4 && W->rank == 4 && Y->rank == 4) {
      /* Channel consistency: X.C_in == W.C_in */
      nova_obligation_require(&ob, "cin_match", X->shape[1] == W->shape[1]);

      /* Output channels: Y.C_out == W.C_out */
      nova_obligation_require(&ob, "cout_match", Y->shape[1] == W->shape[0]);

      /* Batch: Y.N == X.N */
      nova_obligation_require(&ob, "batch_match", Y->shape[0] == X->shape[0]);

      /* Spatial geometry: H_out = (H_in + 2*pad_h - K_h) / stride_h + 1 */
      if (stride_h > 0 && stride_w > 0) {
        int64_t H_out = (X->shape[2] + 2 * pad_h - W->shape[2]) / stride_h + 1;
        int64_t W_out = (X->shape[3] + 2 * pad_w - W->shape[3]) / stride_w + 1;
        nova_obligation_require(&ob, "H_out_correct", Y->shape[2] == H_out);
        nova_obligation_require(&ob, "W_out_correct", Y->shape[3] == W_out);
      }
    }

    /* Optional Bias */
    if (Bias) {
      nova_obligation_require(&ob, "bias_1d", Bias->rank == 1);
      if (Bias->rank == 1 && W->rank >= 1) {
        nova_obligation_require(&ob, "bias_cout",
                                  Bias->shape[0] == W->shape[0]);
      }
    }
  }
  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: ReLU  X -> Y  (shape-preserving, output >= 0)
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_relu(const NovaTensor *X, NovaTensor *Y) {
  NovaObligation ob = nova_obligation_begin("relu");
  nova_obligation_require_tensor(&ob, "X", X);
  nova_obligation_require_tensor(&ob, "Y", Y);
  if (X && Y) {
    nova_obligation_require(&ob, "rank_equal", X->rank == Y->rank);
    if (X->rank == Y->rank) {
      for (int64_t i = 0; i < X->rank; i++) {
        nova_obligation_require(&ob, "shape_equal",
                                  X->shape[i] == Y->shape[i]);
      }
    }
  }
  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: Softmax  X -> Y  (shape-preserving, axis in range)
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_softmax(const NovaTensor *X, NovaTensor *Y,
                                         int64_t axis) {
  NovaObligation ob = nova_obligation_begin("softmax");
  nova_obligation_require_tensor(&ob, "X", X);
  nova_obligation_require_tensor(&ob, "Y", Y);
  if (X && Y) {
    nova_obligation_require(&ob, "rank_equal", X->rank == Y->rank);
    if (X->rank == Y->rank) {
      for (int64_t i = 0; i < X->rank; i++) {
        nova_obligation_require(&ob, "shape_equal",
                                  X->shape[i] == Y->shape[i]);
      }
    }
    nova_obligation_require(&ob, "axis_in_range",
                              (axis >= 0) && (axis < X->rank));
  }
  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: Transpose  X -> Y  (dimension permutation)
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_transpose(const NovaTensor *X,
                                           NovaTensor *Y, const int64_t *perm,
                                           int64_t perm_len) {
  NovaObligation ob = nova_obligation_begin("transpose");
  nova_obligation_require_tensor(&ob, "X", X);
  nova_obligation_require_tensor(&ob, "Y", Y);
  nova_obligation_require(&ob, "perm_nonnull", perm != 0);
  if (X && Y && perm && perm_len > 0) {
    nova_obligation_require(&ob, "perm_len_eq_rank",
                              perm_len == X->rank && Y->rank == X->rank);

    if (perm_len == X->rank && Y->rank == X->rank) {
      for (int64_t i = 0; i < perm_len; i++) {
        nova_obligation_require(&ob, "perm_in_range",
                                  perm[i] >= 0 && perm[i] < perm_len);
      }
      for (int64_t i = 0; i < perm_len; i++) {
        int64_t pi = perm[i];
        if (pi >= 0 && pi < perm_len) {
          nova_obligation_require(&ob, "shape_perm",
                                    Y->shape[i] == X->shape[pi]);
        }
      }
    }
  }
  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CONTRACT: Reshape  X -> Y  (element count preservation)
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_reshape(const NovaTensor *X, NovaTensor *Y,
                                         const int64_t *new_shape,
                                         int64_t new_rank) {
  NovaObligation ob = nova_obligation_begin("reshape");
  nova_obligation_require_tensor(&ob, "X", X);
  nova_obligation_require_tensor(&ob, "Y", Y);
  nova_obligation_require(&ob, "new_shape_nonnull", new_shape != 0);
  nova_obligation_require(&ob, "new_rank_pos", new_rank > 0);

  if (X && Y && new_shape && new_rank > 0) {
    nova_obligation_require(&ob, "y_rank_eq_new_rank", Y->rank == new_rank);

    int64_t known_prod = 1;
    int64_t has_infer = 0;
    int64_t infer_idx = -1;

    for (int64_t i = 0; i < new_rank; i++) {
      int64_t d = new_shape[i];
      if (d == -1) {
        has_infer++;
        infer_idx = i;
      } else {
        nova_obligation_require(&ob, "dim_positive", d > 0);
        if (d > 0)
          known_prod *= d;
      }
    }

    nova_obligation_require(&ob, "infer_at_most_one", has_infer <= 1);

    if (has_infer == 0) {
      nova_obligation_require(&ob, "numel_equal",
                                z_tensor_numel(X) == z_tensor_numel(Y));
    } else if (has_infer == 1) {
      nova_obligation_require(&ob, "known_prod_nonzero", known_prod > 0);
      if (known_prod > 0) {
        nova_obligation_require(&ob, "divisible",
                                  (z_tensor_numel(X) % known_prod) == 0);
      }
      nova_obligation_require(&ob, "infer_idx_valid",
                                infer_idx >= 0 && infer_idx < new_rank);
    }
  }

  return ob;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * NODE HELPERS
 * ═══════════════════════════════════════════════════════════════════════════
 */

static int z_node_is(const NovaIRNode *n, const char *name) {
  if (!n || !name)
    return 0;
  if (!n->op)
    return 0;
  return strcmp(n->op, name) == 0;
}

static const NovaTensor *z_node_in(const NovaIRNode *n, int idx) {
  if (!n || idx < 0 || idx >= (int)n->num_inputs)
    return 0;
  return n->inputs[idx];
}

static NovaTensor *z_node_out(const NovaIRNode *n, int idx) {
  if (!n || idx < 0 || idx >= (int)n->num_outputs)
    return 0;
  return n->outputs[idx];
}

/* ═══════════════════════════════════════════════════════════════════════════
 * DISPATCHER: Graph Node -> Contract
 * ═══════════════════════════════════════════════════════════════════════════
 */

NovaObligation nova_contract_from_node(const NovaIRNode *node) {
  if (!node || !node->op) {
    NovaObligation ob = nova_obligation_begin("node_contract");
    nova_obligation_require(&ob, "node_nonnull", node != 0);
    return ob;
  }

  if (z_node_is(node, "matmul")) {
    const NovaTensor *A = z_node_in(node, 0);
    const NovaTensor *B = z_node_in(node, 1);
    const NovaTensor *Bias = (node->num_inputs >= 3) ? z_node_in(node, 2) : 0;
    NovaTensor *Out = z_node_out(node, 0);
    return nova_contract_matmul(A, B, Bias, Out);
  }

  if (z_node_is(node, "add")) {
    const NovaTensor *A = z_node_in(node, 0);
    const NovaTensor *B = z_node_in(node, 1);
    NovaTensor *Out = z_node_out(node, 0);
    return nova_contract_add(A, B, Out);
  }

  if (z_node_is(node, "conv2d")) {
    const NovaTensor *X = z_node_in(node, 0);
    const NovaTensor *W = z_node_in(node, 1);
    const NovaTensor *Bias = (node->num_inputs >= 3) ? z_node_in(node, 2) : 0;
    NovaTensor *Y = z_node_out(node, 0);

    int64_t sh = node->attrs.conv2d.stride_h;
    int64_t sw = node->attrs.conv2d.stride_w;
    int64_t ph = node->attrs.conv2d.pad_h;
    int64_t pw = node->attrs.conv2d.pad_w;

    return nova_contract_conv2d(X, W, Bias, Y, sh, sw, ph, pw);
  }

  if (z_node_is(node, "relu")) {
    const NovaTensor *X = z_node_in(node, 0);
    NovaTensor *Y = z_node_out(node, 0);
    return nova_contract_relu(X, Y);
  }

  if (z_node_is(node, "softmax")) {
    const NovaTensor *X = z_node_in(node, 0);
    NovaTensor *Y = z_node_out(node, 0);
    int64_t axis = node->attrs.softmax.axis;
    return nova_contract_softmax(X, Y, axis);
  }

  if (z_node_is(node, "transpose")) {
    const NovaTensor *X = z_node_in(node, 0);
    NovaTensor *Y = z_node_out(node, 0);
    return nova_contract_transpose(X, Y, node->attrs.transpose.perm,
                                     node->attrs.transpose.perm_len);
  }

  if (z_node_is(node, "reshape")) {
    const NovaTensor *X = z_node_in(node, 0);
    NovaTensor *Y = z_node_out(node, 0);
    return nova_contract_reshape(X, Y, node->attrs.reshape.new_shape,
                                   node->attrs.reshape.new_rank);
  }

  if (z_node_is(node, "input") || z_node_is(node, "const") ||
      z_node_is(node, "return")) {
    NovaObligation ob = nova_obligation_begin(node->op);
    /* Input/Const/Return are always structurally valid leaf/sink nodes */
    return ob;
  }

  /* Unknown operation — emit warning */
  NovaObligation ob = nova_obligation_begin("unknown_op");
  nova_obligation_require(&ob, "op_known", 0);
  return ob;
}
