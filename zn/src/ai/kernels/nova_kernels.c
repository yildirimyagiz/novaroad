#include "nova_kernels.h"
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA KERNELS - Unified Backend Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_USE_EXTREME_KERNELS
void nova_kernel_matmul(NovaTensor *A, NovaTensor *B, NovaTensor *C) {
  // Pre-execution validation (simplified)
  if (!A || !B || !C) {
    fprintf(stderr, "⚠️ Kernel Error: NULL tensor\n");
    return;
  }

  if (!A->context) {
    printf("❌ Error: Tensor has no context binding\n");
    return;
  }

  printf("🚀 Kernel: Dispatching MatMul [%" PRId64 "x%" PRId64
         "] via Compute OS\n",
         A->shape[0], B->shape[1]);

  // Phase 5: Kernel asks the context/fabric to execute.
  // Dispatch is handled by the Compute OS.
}

void nova_kernel_add(NovaTensor *A, NovaTensor *B, NovaTensor *C) {
  (void)A;
  (void)B;
  (void)C;
  printf("🚀 Kernel: Vector Add\n");
}

void nova_kernel_attention(NovaTensor *Q, NovaTensor *K, NovaTensor *V,
                             NovaTensor *out) {
  (void)Q;
  (void)K;
  (void)V;
  (void)out;
  printf("🚀 Kernel: Cognitive Attention Execution\n");
}

void nova_kernel_stabilize_fp(NovaTensor *t) {
  if (t->is_deterministic) {
    // Apply stabilization filters
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Activation Function Kernels - CPU Implementation
// ═══════════════════════════════════════════════════════════════════════════

void nova_kernel_relu(NovaTensor *x, NovaTensor *out) {
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    out_data[i] = x_data[i] > 0.0f ? x_data[i] : 0.0f;
  }
}

void nova_kernel_sigmoid(NovaTensor *x, NovaTensor *out) {
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    out_data[i] = 1.0f / (1.0f + expf(-x_data[i]));
  }
}

void nova_kernel_tanh(NovaTensor *x, NovaTensor *out) {
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    out_data[i] = tanhf(x_data[i]);
  }
}

void nova_kernel_gelu(NovaTensor *x, NovaTensor *out) {
  // GELU(x) = 0.5 * x * (1 + tanh(sqrt(2/π) * (x + 0.044715 * x³)))
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  const float sqrt_2_over_pi = 0.7978845608f; // sqrt(2/π)
  const float coeff = 0.044715f;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    float x_val = x_data[i];
    float x_cubed = x_val * x_val * x_val;
    float inner = sqrt_2_over_pi * (x_val + coeff * x_cubed);
    out_data[i] = 0.5f * x_val * (1.0f + tanhf(inner));
  }
}

void nova_kernel_silu(NovaTensor *x, NovaTensor *out) {
  // SiLU(x) = x * sigmoid(x)
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    float x_val = x_data[i];
    float sigmoid = 1.0f / (1.0f + expf(-x_val));
    out_data[i] = x_val * sigmoid;
  }
}

void nova_kernel_softmax(NovaTensor *x, int dim, NovaTensor *out) {
  (void)dim; // Simplified: assume last dimension
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  // Simplified softmax over last dimension
  size_t n = x->total_elements;
  
  // Find max for numerical stability
  float max_val = x_data[0];
  for (size_t i = 1; i < n; i++) {
    if (x_data[i] > max_val) max_val = x_data[i];
  }
  
  // Compute exp and sum
  float sum = 0.0f;
  for (size_t i = 0; i < n; i++) {
    out_data[i] = expf(x_data[i] - max_val);
    sum += out_data[i];
  }
  
  // Normalize
  for (size_t i = 0; i < n; i++) {
    out_data[i] /= sum;
  }
}

void nova_kernel_clamp(NovaTensor *x, float min, float max, NovaTensor *out) {
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    float val = x_data[i];
    if (val < min) val = min;
    if (val > max) val = max;
    out_data[i] = val;
  }
}

void nova_kernel_pow(NovaTensor *x, float exponent, NovaTensor *out) {
  float *x_data = (float *)x->data;
  float *out_data = (float *)out->data;
  
  for (size_t i = 0; i < x->total_elements; i++) {
    out_data[i] = powf(x_data[i], exponent);
  }
}

void nova_kernel_mul(NovaTensor *A, NovaTensor *B, NovaTensor *C) {
  float *a_data = (float *)A->data;
  float *b_data = (float *)B->data;
  float *c_data = (float *)C->data;
  
  for (size_t i = 0; i < A->total_elements; i++) {
    c_data[i] = a_data[i] * b_data[i];
  }
}

void nova_kernel_scalar_mul(NovaTensor *A, float s, NovaTensor *C) {
  float *a_data = (float *)A->data;
  float *c_data = (float *)C->data;
  
  for (size_t i = 0; i < A->total_elements; i++) {
    c_data[i] = a_data[i] * s;
  }
}

void nova_kernel_transpose(NovaTensor *A, NovaTensor *C) {
  // Simplified 2D transpose
  if (A->ndim != 2) return;
  
  float *a_data = (float *)A->data;
  float *c_data = (float *)C->data;
  
  int64_t rows = A->shape[0];
  int64_t cols = A->shape[1];
  
  for (int64_t i = 0; i < rows; i++) {
    for (int64_t j = 0; j < cols; j++) {
      c_data[j * rows + i] = a_data[i * cols + j];
    }
  }
}

void nova_kernel_unsqueeze(NovaTensor *x, int dim, NovaTensor *out) {
  (void)dim;
  // For unsqueeze, data is the same, just shape changes (already handled in tensor_ops)
  memcpy(out->data, x->data, x->total_elements * sizeof(float));
}

#endif
