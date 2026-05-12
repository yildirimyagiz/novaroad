// native/src/optimizer/nova_kernel_fusion.c
#include "nova_kernel_fusion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NOVA_MAX
#define NOVA_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// ---- Op codes aligned with nova_graph.h NovaGraphOpType ----
enum {
  ZF_OP_MATMUL = 0,     // NOVA_OP_MATMUL
  ZF_OP_ADD = 1,        // NOVA_OP_ADD
  ZF_OP_ACTIVATION = 5, // NOVA_OP_ACTIVATION (ReLU lives here)

  ZF_OP_CONV2D = 6,   // NOVA_OP_CONV2D
  ZF_OP_BIAS_ADD = 1, // Bias-add uses same ADD op

  // Fused op nodes:
  ZF_OP_FUSED = 10 // NOVA_OP_FUSED
};

enum { NOVA_DTYPE_FP32 = 1 };

typedef struct Registry {
  NovaFusionKernel *items;
  int64_t len;
  int64_t cap;
} Registry;

static Registry g_reg = {0};

static void reg_reserve(int64_t need) {
  if (g_reg.cap >= need)
    return;
  int64_t nc = g_reg.cap ? g_reg.cap : 16;
  while (nc < need)
    nc *= 2;
  NovaFusionKernel *ni = (NovaFusionKernel *)realloc(
      g_reg.items, (size_t)nc * sizeof(NovaFusionKernel));
  if (!ni)
    return;
  g_reg.items = ni;
  g_reg.cap = nc;
}

void nova_fusion_init(void) {
  if (g_reg.items)
    return;

  reg_reserve(16);
  g_reg.len = 0;

  // Built-in known kernel names (backend may override/register explicitly)
  nova_fusion_register(NOVA_FUSED_MATMUL_ADD_RELU_F32,
                         "nova_kernel_matmul_add_relu_f32");
  nova_fusion_register(NOVA_FUSED_CONV2D_BIAS_RELU_F32,
                         "nova_kernel_conv2d_bias_relu_f32");
}

void nova_fusion_register(NovaFusedOp fused_op, const char *kernel_name) {
  if (!kernel_name)
    return;
  if (!g_reg.items)
    nova_fusion_init();

  for (int64_t i = 0; i < g_reg.len; i++) {
    if (g_reg.items[i].fused_op == fused_op) {
      g_reg.items[i].kernel_name = kernel_name;
      return;
    }
  }

  reg_reserve(g_reg.len + 1);
  if (!g_reg.items)
    return;

  g_reg.items[g_reg.len].fused_op = fused_op;
  g_reg.items[g_reg.len].kernel_name = kernel_name;
  g_reg.len++;
}

const NovaFusionKernel *nova_fusion_lookup(NovaFusedOp fused_op) {
  if (!g_reg.items)
    return NULL;
  for (int64_t i = 0; i < g_reg.len; i++) {
    if (g_reg.items[i].fused_op == fused_op)
      return &g_reg.items[i];
  }
  return NULL;
}

static int is_fp32(const NovaGraphVTable *vt, void *g, int64_t n) {
  if (!vt->node_dtype)
    return 1;
  return vt->node_dtype(g, n) == NOVA_DTYPE_FP32;
}

static int64_t find_single_user(const NovaGraphVTable *vt, void *g,
                                int64_t node_id) {
  // expects vt->node_output_count/output encode uses as "outputs"
  // If your graph doesn't expose users via outputs, adapt vtable to provide it.
  if (!vt->node_output_count || !vt->node_output)
    return -1;
  int64_t oc = vt->node_output_count(g, node_id);
  // Simplification: In many graph reprs, outputs are data consumers.
  // If we have single consumer, good. If multiple key is to check if all are
  // handled or just abort fusion. For simple linear fusion, require oc==1.
  if (oc != 1)
    return -1;
  return vt->node_output(g, node_id, 0);
}

static int match_matmul_add_relu(void *graph, const NovaGraphVTable *vt,
                                 int64_t n_matmul, int64_t *out_add,
                                 int64_t *out_relu) {
  if (vt->node_op(graph, n_matmul) != ZF_OP_MATMUL)
    return 0;
  if (!is_fp32(vt, graph, n_matmul))
    return 0;

  int64_t n_add = find_single_user(vt, graph, n_matmul);
  if (n_add < 0)
    return 0;
  if (vt->node_op(graph, n_add) != ZF_OP_ADD)
    return 0;
  if (!is_fp32(vt, graph, n_add))
    return 0;

  int64_t n_relu = find_single_user(vt, graph, n_add);
  if (n_relu < 0)
    return 0;
  if (vt->node_op(graph, n_relu) != ZF_OP_ACTIVATION)
    return 0;
  if (!is_fp32(vt, graph, n_relu))
    return 0;

  *out_add = n_add;
  *out_relu = n_relu;
  return 1;
}

static int match_conv_bias_relu(void *graph, const NovaGraphVTable *vt,
                                int64_t n_conv, int64_t *out_bias,
                                int64_t *out_relu) {
  if (vt->node_op(graph, n_conv) != ZF_OP_CONV2D)
    return 0;
  if (!is_fp32(vt, graph, n_conv))
    return 0;

  int64_t n_bias = find_single_user(vt, graph, n_conv);
  if (n_bias < 0)
    return 0;
  if (vt->node_op(graph, n_bias) != ZF_OP_BIAS_ADD)
    return 0;
  if (!is_fp32(vt, graph, n_bias))
    return 0;

  int64_t n_relu = find_single_user(vt, graph, n_bias);
  if (n_relu < 0)
    return 0;
  if (vt->node_op(graph, n_relu) != ZF_OP_ACTIVATION)
    return 0;
  if (!is_fp32(vt, graph, n_relu))
    return 0;

  *out_bias = n_bias;
  *out_relu = n_relu;
  return 1;
}

static int64_t rewrite_matmul_add_relu(void *graph, const NovaGraphVTable *vt,
                                       int64_t n_matmul, int64_t n_add,
                                       int64_t n_relu) {
  // Expect:
  // matmul(A,B) -> add(matmul_out, bias_or_other) -> relu(add_out)
  // Create fused node with inputs: A,B, (the other add input)
  int64_t a = vt->node_input(graph, n_matmul, 0);
  int64_t b = vt->node_input(graph, n_matmul, 1);

  // Determine "other" add input (not the matmul node)
  int64_t add_in0 = vt->node_input(graph, n_add, 0);
  int64_t add_in1 = vt->node_input(graph, n_add, 1);
  int64_t other = (add_in0 == n_matmul) ? add_in1 : add_in0;
  if (add_in0 != n_matmul && add_in1 != n_matmul) {
    // Should not happen if match logic logic is correct regarding topology data
    other = add_in1;
  }

  // Inputs: A, B, Bias, OpCode
  int64_t inputs[4] = {a, b, other, (int64_t)NOVA_FUSED_MATMUL_ADD_RELU_F32};
  // convention: last input stores fused-op id OR use attrs in your graph; adapt
  // as needed
  int64_t n_fused = vt->node_create(graph, ZF_OP_FUSED, inputs, 4);

  if (vt->node_set_tag)
    vt->node_set_tag(graph, n_fused, "FUSED:MATMUL+ADD+RELU");

  // Wire new node to consumers of the old relu node
  vt->node_replace_uses(graph, n_relu, n_fused);

  // Cleanup
  vt->node_delete(graph, n_relu);
  vt->node_delete(graph, n_add);
  vt->node_delete(graph, n_matmul);
  return 1;
}

static int64_t rewrite_conv_bias_relu(void *graph, const NovaGraphVTable *vt,
                                      int64_t n_conv, int64_t n_bias,
                                      int64_t n_relu) {
  // Expect:
  // conv2d(input, weight, ...) -> bias_add(conv_out, bias) -> relu
  // Fuse: input, weight, bias, (attrs as separate nodes/inputs in your graph)
  // Here: take first 2 conv inputs + bias other input.
  int64_t x = vt->node_input(graph, n_conv, 0);
  int64_t w = vt->node_input(graph, n_conv, 1);

  int64_t bias_in0 = vt->node_input(graph, n_bias, 0);
  int64_t bias_in1 = vt->node_input(graph, n_bias, 1);
  int64_t bias = (bias_in0 == n_conv) ? bias_in1 : bias_in0;

  int64_t inputs[4] = {x, w, bias, (int64_t)NOVA_FUSED_CONV2D_BIAS_RELU_F32};
  int64_t n_fused = vt->node_create(graph, ZF_OP_FUSED, inputs, 4);

  if (vt->node_set_tag)
    vt->node_set_tag(graph, n_fused, "FUSED:CONV2D+BIAS+RELU");

  vt->node_replace_uses(graph, n_relu, n_fused);
  vt->node_delete(graph, n_relu);
  vt->node_delete(graph, n_bias);
  vt->node_delete(graph, n_conv);
  return 1;
}

int64_t nova_fusion_run(void *graph, const NovaGraphVTable *vt) {
  if (!graph || !vt || !vt->node_count || !vt->node_id_at || !vt->node_op ||
      !vt->node_input || !vt->node_input_count || !vt->node_create ||
      !vt->node_replace_uses || !vt->node_delete) {
    return 0;
  }

  if (!g_reg.items)
    nova_fusion_init();

  int64_t fused = 0;
  int64_t n = vt->node_count(graph);

  int64_t *candidates_matmul = calloc(n, sizeof(int64_t));
  int64_t *candidates_conv = calloc(n, sizeof(int64_t));
  int64_t c_matmul = 0;
  int64_t c_conv = 0;

  for (int64_t i = 0; i < n; i++) {
    int64_t id = vt->node_id_at(graph, i);
    int64_t n2 = -1, n3 = -1;

    if (match_matmul_add_relu(graph, vt, id, &n2, &n3)) {
      candidates_matmul[c_matmul++] = id;
      continue;
    }
    if (match_conv_bias_relu(graph, vt, id, &n2, &n3)) {
      candidates_conv[c_conv++] = id;
    }
  }

  // Apply matmul+add+relu rewrites
  for (int64_t i = 0; i < c_matmul; i++) {
    int64_t id = candidates_matmul[i];
    int64_t n2, n3;
    if (match_matmul_add_relu(graph, vt, id, &n2, &n3)) {
      rewrite_matmul_add_relu(graph, vt, id, n2, n3);
      fused++;
    }
  }

  // Apply conv+bias+relu rewrites
  for (int64_t i = 0; i < c_conv; i++) {
    int64_t id = candidates_conv[i];
    int64_t n2, n3;
    if (match_conv_bias_relu(graph, vt, id, &n2, &n3)) {
      rewrite_conv_bias_relu(graph, vt, id, n2, n3);
      fused++;
    }
  }

  free(candidates_matmul);
  free(candidates_conv);

  return fused;
}
