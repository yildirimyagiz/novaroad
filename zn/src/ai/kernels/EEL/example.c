/*
 * example.c — Minimal Nova usage example
 *
 * Demonstrates:
 *  1. Building a compute graph
 *  2. Running the optimizer (fusion, dead-node elim, memory planning)
 *  3. Executing on the CPU backend
 *  4. Reading trace output
 *
 * Build:
 *   cc -O2 -std=c11 -o example example.c \
 *       nova_graph.c nova_fusion.c nova_memory.c \
 *       nova_scheduler.c nova_backend.c nova_trace.c -lm
 */

#include "nova.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Fill tensor with a value */
static void fill_const(NovaTensor *t, float v) {
  if (!t || !t->data)
    return;
  float *d = (float *)t->data;
  size_t n = t->nbytes / sizeof(float);
  for (size_t i = 0; i < n; ++i)
    d[i] = v;
}

static float sum_tensor(NovaTensor *t) {
  if (!t || !t->data)
    return 0.0f;
  float *d = (float *)t->data;
  size_t n = t->nbytes / sizeof(float);
  float s = 0.0f;
  for (size_t i = 0; i < n; ++i)
    s += d[i];
  return s;
}

int main(void) {
  printf("═══════════════════════════════════════════════════════════\n");
  printf("  Nova Example: MLP forward pass (linear → bias → relu)\n");
  printf("═══════════════════════════════════════════════════════════\n\n");

  /* ----------------------------------------------------------------
   * Graph: input[8,16] · weight[16,32] + bias[32] → relu → out[8,32]
   * ---------------------------------------------------------------- */
  NovaGraph *g = nova_graph_create("mlp_forward", 16, 16);
  if (!g) {
    fprintf(stderr, "graph_create failed\n");
    return 1;
  }

  nova_graph_set_backend(g, nova_backend_cpu());

  /* Define shapes */
  int64_t sh_input[2] = {8, 16};
  int64_t sh_weight[2] = {16, 32};
  int64_t sh_mm_out[2] = {8, 32};
  int64_t sh_bias[1] = {32};
  int64_t sh_out[2] = {8, 32};

  /* Tensors */
  uint32_t t_input = nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh_input, true);
  uint32_t t_weight =
      nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh_weight, true);
  uint32_t t_mm_out =
      nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh_mm_out, false);
  uint32_t t_bias = nova_tensor_add(g, NOVA_DTYPE_F32, 1, sh_bias, true);
  uint32_t t_bias_out =
      nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh_out, false);
  uint32_t t_relu_out =
      nova_tensor_add(g, NOVA_DTYPE_F32, 2, sh_out, false);

  /* Nodes */
  uint32_t mm_in[2] = {t_input, t_weight};
  uint32_t mm_out[1] = {t_mm_out};
  uint32_t bias_in[2] = {t_mm_out, t_bias};
  uint32_t bias_out[1] = {t_bias_out};
  uint32_t relu_in[1] = {t_bias_out};
  uint32_t relu_out[1] = {t_relu_out};

  nova_node_add(g, NOVA_OP_MATMUL, "linear", mm_in, 2, mm_out, 1);
  nova_node_add(g, NOVA_OP_BIAS_ADD, "bias", bias_in, 2, bias_out, 1);
  nova_node_add(g, NOVA_OP_RELU, "relu", relu_in, 1, relu_out, 1);

  printf("[main] Graph constructed: %u nodes, %u tensors\n", g->num_nodes,
         g->num_tensors);

  /* ----------------------------------------------------------------
   * Optimize (fusion + dead-node-elim + memory planning)
   * ---------------------------------------------------------------- */
  NovaStatus s = nova_graph_optimize(g);
  if (s != NOVA_OK) {
    fprintf(stderr, "optimize failed: %s\n", nova_status_str(s));
    nova_graph_destroy(g);
    return 1;
  }
  printf("[main] After optimize: %u live nodes, %u fusions applied\n\n",
         g->topo_len, g->fusions_applied);

  /* ----------------------------------------------------------------
   * Allocate memory
   * ---------------------------------------------------------------- */
  s = nova_memory_allocate(g);
  if (s != NOVA_OK) {
    fprintf(stderr, "memory_allocate failed: %s\n", nova_status_str(s));
    nova_graph_destroy(g);
    return 1;
  }

  /* ----------------------------------------------------------------
   * Fill inputs with known values
   * ---------------------------------------------------------------- */
  fill_const(&g->tensors[t_input], 1.0f);  /* all ones */
  fill_const(&g->tensors[t_weight], 0.1f); /* 0.1 */
  fill_const(&g->tensors[t_bias], 0.5f);   /* 0.5 */

  /* ----------------------------------------------------------------
   * Execute with tracing
   * ---------------------------------------------------------------- */
  NovaTrace *trace = nova_trace_create(64);

  /* Re-implement execute with trace hooks for this example */
  for (uint32_t oi = 0; oi < g->topo_len; ++oi) {
    uint32_t ni = g->topo_order[oi];
    NovaNode *nd = &g->nodes[ni];
    if (nd->is_fused || nd->executed)
      continue;

    nova_trace_begin(trace, nd->id);
    g->backend->execute(g->backend, nd, g->tensors);
    nd->executed = true;
    g->kernel_launches++;

    uint64_t rd = 0, wr = 0;
    for (uint32_t i = 0; i < nd->num_inputs; ++i)
      rd += g->tensors[nd->inputs[i]].nbytes;
    for (uint32_t i = 0; i < nd->num_outputs; ++i)
      wr += g->tensors[nd->outputs[i]].nbytes;
    nova_trace_end(trace, nd->id, rd, wr);
  }
  if (g->backend->synchronize)
    g->backend->synchronize(g->backend);

  /* ----------------------------------------------------------------
   * Validate output
   *
   * Expected: input=1.0, weight=0.1, bias=0.5
   *  matmul: each row of output = sum_k(input[i,k] * weight[k,j])
   *          = 16 * 0.1 * 0.1 = ... wait, input is [8,16], weight [16,32]
   *          C[i,j] = sum_k A[i,k]*B[k,j] = 16 * 1.0 * 0.1 = 1.6
   *  bias:   1.6 + 0.5 = 2.1
   *  relu:   max(0, 2.1) = 2.1
   * ---------------------------------------------------------------- */
  NovaTensor *result_tensor = &g->tensors[t_relu_out];

  /* If fused, the output may be in t_mm_out (fusion rewired outputs) */
  /* Find the actual live output tensor */
  NovaTensor *final_out = NULL;
  for (uint32_t i = 0; i < g->num_tensors; ++i) {
    if (g->tensors[i].is_output && g->tensors[i].data) {
      final_out = &g->tensors[i];
      break;
    }
  }
  if (!final_out)
    final_out = result_tensor;

  float total = sum_tensor(final_out);
  float count = (float)(8 * 32);
  float avg = final_out->data ? total / count : -1.0f;

  printf("[main] Output tensor: %ld×%ld, avg value = %.4f (expected ~2.1)\n",
         final_out->shape[0], final_out->shape[1], avg);

  /* ----------------------------------------------------------------
   * Print trace
   * ---------------------------------------------------------------- */
  nova_trace_print(trace, g);

  /* ----------------------------------------------------------------
   * Available backends
   * ---------------------------------------------------------------- */
  printf("Available backends:\n");
  printf("  CPU:    %s\n", nova_backend_cpu()->is_available() ? "YES" : "NO");
  printf("  CUDA:   %s\n",
         nova_backend_cuda_stub()->is_available() ? "YES" : "NO(stub)");
  printf("  Metal:  %s\n", nova_backend_metal_stub()->is_available()
                               ? "YES (Apple)"
                               : "NO(stub)");
  printf("  Vulkan: %s\n",
         nova_backend_vulkan_stub()->is_available() ? "YES" : "NO(stub)");

  nova_trace_destroy(trace);
  nova_graph_destroy(g);

  printf("\nDone.\n");
  return 0;
}
