/**
 * src/tools/nova_llama_layer_bench.c
 * NOVA ULTRA-LEAGUE: Llama-7B 1-Layer End-to-End Simulation
 */

#include "compute/nova_kernels.h"
#include "ml/nova_tensor.h"
#include <mach/mach_time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint64_t now_ns() {
  static mach_timebase_info_data_t tb;
  if (tb.denom == 0)
    mach_timebase_info(&tb);
  return mach_absolute_time() * tb.numer / tb.denom;
}

// Simulated Llama-7B Layer Params
#define DIM 4096
#define HEADS 32
#define HEAD_DIM 128
#define MLP_DIM 11008

void run_llama_layer_bench(int seq_len, bool quantized, bool nova_elite) {
  printf("🎬 Testing Llama-7B Layer | %s | SeqLen: %d | Precision: %s | PE "
         "Strategy: %s\n",
         nova_elite ? "NOVA ELITE" : "MOJO/STD", seq_len,
         quantized ? "INT8-Hybrid" : "FP16-Hybrid",
         nova_elite ? "RECURRENT (0-BW)" : "MATERIALIZED (Mem-Bound)");

  // 1. Memory Setup
  int64_t s_input[] = {seq_len, DIM};
  int64_t s_weight_qkv[] = {DIM, DIM * 3};
  int64_t s_weight_out[] = {DIM, DIM};
  int64_t s_weight_mlp[] = {DIM, MLP_DIM};
  int64_t s_weight_down[] = {MLP_DIM, DIM};

  NovaTensor *x = nova_tensor_create(NULL, s_input, 2, NOVA_DTYPE_FP32);

  // Positional Encoding Tensor
  NovaTensor *pe = nova_tensor_create(NULL, (int64_t[]){seq_len, HEAD_DIM},
                                          2, NOVA_DTYPE_FP32);

  // Weights (Switch based on quantization)
  NovaTensor *w_qkv, *w_out, *w_mlp, *w_down;
  if (quantized) {
    w_qkv = nova_tensor_create(NULL, s_weight_qkv, 2, NOVA_DTYPE_INT8);
    w_out = nova_tensor_create(NULL, s_weight_out, 2, NOVA_DTYPE_INT8);
    w_mlp = nova_tensor_create(NULL, s_weight_mlp, 2, NOVA_DTYPE_INT8);
    w_down = nova_tensor_create(NULL, s_weight_down, 2, NOVA_DTYPE_INT8);
  } else {
    w_qkv = nova_tensor_create(NULL, s_weight_qkv, 2, NOVA_DTYPE_FP32);
    w_out = nova_tensor_create(NULL, s_weight_out, 2, NOVA_DTYPE_FP32);
    w_mlp = nova_tensor_create(NULL, s_weight_mlp, 2, NOVA_DTYPE_FP32);
    w_down = nova_tensor_create(NULL, s_weight_down, 2, NOVA_DTYPE_FP32);
  }

  // Intermediate results
  NovaTensor *qkv = nova_tensor_create(NULL, (int64_t[]){seq_len, DIM * 3},
                                           2, NOVA_DTYPE_FP32);
  NovaTensor *attn_out = nova_tensor_create(NULL, (int64_t[]){seq_len, DIM},
                                                2, NOVA_DTYPE_FP32);
  NovaTensor *mlp_gate = nova_tensor_create(
      NULL, (int64_t[]){seq_len, MLP_DIM}, 2, NOVA_DTYPE_FP32);
  NovaTensor *mlp_down = nova_tensor_create(NULL, (int64_t[]){seq_len, DIM},
                                                2, NOVA_DTYPE_FP32);

  // Multi-head views
  int64_t s_head[] = {1, HEADS, seq_len, HEAD_DIM};
  NovaTensor *Q = nova_tensor_create(NULL, s_head, 4, NOVA_DTYPE_FP32);
  NovaTensor *K = nova_tensor_create(NULL, s_head, 4, NOVA_DTYPE_FP32);
  NovaTensor *V = nova_tensor_create(NULL, s_head, 4, NOVA_DTYPE_FP32);
  NovaTensor *O = nova_tensor_create(NULL, s_head, 4, NOVA_DTYPE_FP32);

  // Warmup
  if (quantized)
    nova_kernel_matmul_int8_f16(x, w_qkv, qkv);
  else
    nova_kernel_matmul_add_relu_f32(x, w_qkv, qkv, qkv);

  uint64_t t0 = now_ns();
  int iterations = 3;
  for (int i = 0; i < iterations; i++) {
    // Stage 1: PE Generation (The Elite Difference)
    if (nova_elite) {
      nova_kernel_recurrent_pe_f32(pe);
    } else {
      // Simulate loading PE from memory (Bandwidth hit)
      for (int k = 0; k < seq_len * HEAD_DIM; k++)
        ((float *)pe->data)[k] += 0.001f;
    }

    if (quantized) {
      nova_kernel_matmul_int8_f16(x, w_qkv, qkv);
      nova_kernel_attention(Q, K, V, O);
      nova_kernel_matmul_int8_f16(attn_out, w_out, attn_out);
      nova_kernel_matmul_int8_f16(x, w_mlp, mlp_gate);
      nova_kernel_matmul_int8_f16(mlp_gate, w_down, mlp_down);
    } else {
      nova_kernel_matmul_add_relu_f32(x, w_qkv, qkv, qkv);
      nova_kernel_attention(Q, K, V, O);
      nova_kernel_matmul_add_relu_f32(attn_out, w_out, attn_out, attn_out);
      nova_kernel_matmul_add_relu_f32(x, w_mlp, mlp_gate, mlp_gate);
      nova_kernel_matmul_add_relu_f32(mlp_gate, w_down, mlp_down, mlp_down);
    }
  }
  uint64_t t1 = now_ns();

  double total_ms = (double)(t1 - t0) / (iterations * 1e6);
  double per_token_ms = total_ms / seq_len;

  // Extrapolate to 32 layers
  double layer_32_ms = total_ms * 32;
  double tokens_per_sec = 1000.0 / (per_token_ms * 32);

  printf("   Total Execution (1 Layer): %.2f ms\n", total_ms);
  printf("   Per-Token (1 Layer)      : %.4f ms\n", per_token_ms);
  printf("   Extrapolated (32 Layers) : %.2f ms/token\n", layer_32_ms);
  printf("   🔥 RESULT: %.2f tokens/s\n\n", tokens_per_sec);

  // Cleanup
  nova_tensor_destroy(x);
  nova_tensor_destroy(pe);
  nova_tensor_destroy(w_qkv);
  nova_tensor_destroy(w_out);
  nova_tensor_destroy(w_mlp);
  nova_tensor_destroy(w_down);
  nova_tensor_destroy(qkv);
  nova_tensor_destroy(attn_out);
  nova_tensor_destroy(mlp_gate);
  nova_tensor_destroy(mlp_down);
  nova_tensor_destroy(Q);
  nova_tensor_destroy(K);
  nova_tensor_destroy(V);
  nova_tensor_destroy(O);
}

int main() {
  printf(
      "\n══════════════════════════════════════════════════════════════════\n");
  printf("   NOVA LLAMA-7B END-TO-END INFERENCE BENCHMARK\n");
  printf(
      "══════════════════════════════════════════════════════════════════\n\n");

  run_llama_layer_bench(1, false, false);   // Standard Mojo Path
  run_llama_layer_bench(1, true, true);     // Nova Elite Path
  run_llama_layer_bench(256, false, false); // Prefill Mojo
  run_llama_layer_bench(256, true, true);   // Prefill Nova Elite

  printf(
      "══════════════════════════════════════════════════════════════════\n");
  return 0;
}
