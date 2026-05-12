/*
 * nova_backend.c — Backend Abstraction Layer
 *
 * Provides:
 *  - CPU backend: reference scalar implementation of all ops
 *  - CUDA stub: interface skeleton for GPU dispatch
 *  - Metal stub: interface skeleton for Apple GPU dispatch
 *  - Vulkan stub: interface skeleton for Vulkan compute dispatch
 *
 * The CPU backend is fully functional (single-threaded, f32).
 * GPU stubs compile cleanly and log calls; swap in real kernels via FFI.
 */

#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif

#include "nova.h"

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

/* =========================================================================
 * CPU backend — reference implementation
 * ========================================================================= */

/* --- Helpers --- */

static inline float *tdata_f32(NovaTensor *tensors, uint32_t id) {
  return (float *)tensors[id].data;
}

/* Multi-threaded worker for MatMul */
typedef struct {
  const float *A, *B;
  float *C;
  int M, N, K;
  int start_m, end_m;
} MatMulTask;

static void *matmul_worker(void *arg) {
  MatMulTask *t = (MatMulTask *)arg;
  const float *a = t->A;
  const float *b = t->B;
  float *c = t->C;
  const int N = t->N, K = t->K;

#ifdef __ARM_NEON
  const int TILE_K = 32;
  const int TILE_N = 64;

  for (int m = t->start_m; m < t->end_m; m += 4) {
    for (int k0 = 0; k0 < K; k0 += TILE_K) {
      int k_end = (k0 + TILE_K < K) ? k0 + TILE_K : K;
      for (int n0 = 0; n0 < N; n0 += TILE_N) {
        int n_end = (n0 + TILE_N < N) ? n0 + TILE_N : N;

        for (int nn = n0; nn < n_end; nn += 4) {
          float32x4_t sum0 = vld1q_f32(&c[m * N + nn]);
          float32x4_t sum1 = vld1q_f32(&c[(m + 1) * N + nn]);
          float32x4_t sum2 = vld1q_f32(&c[(m + 2) * N + nn]);
          float32x4_t sum3 = vld1q_f32(&c[(m + 3) * N + nn]);

          for (int k = k0; k < k_end; ++k) {
            float32x4_t bv = vld1q_f32(&b[k * N + nn]);
            sum0 = vmlaq_n_f32(sum0, bv, a[m * K + k]);
            sum1 = vmlaq_n_f32(sum1, bv, a[(m + 1) * K + k]);
            sum2 = vmlaq_n_f32(sum2, bv, a[(m + 2) * K + k]);
            sum3 = vmlaq_n_f32(sum3, bv, a[(m + 3) * K + k]);
          }
          vst1q_f32(&c[m * N + nn], sum0);
          vst1q_f32(&c[(m + 1) * N + nn], sum1);
          vst1q_f32(&c[(m + 2) * N + nn], sum2);
          vst1q_f32(&c[(m + 3) * N + nn], sum3);
        }
      }
    }
  }
#else
  for (int m = t->start_m; m < t->end_m; ++m) {
    for (int k = 0; k < K; ++k) {
      float av = a[m * K + k];
      for (int n = 0; n < N; ++n) {
        c[m * N + n] += av * b[k * N + n];
      }
    }
  }
#endif
  return NULL;
}

/* High-Performance Parallel NEON MatMul */
static void cpu_matmul(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *A = &tensors[n->inputs[0]];
  NovaTensor *B = &tensors[n->inputs[1]];
  NovaTensor *C = &tensors[n->outputs[0]];
  if (!A->data || !B->data || !C->data)
    return;

  const int M = (int)A->shape[A->ndim - 2];
  const int K = (int)A->shape[A->ndim - 1];
  const int N = (int)B->shape[B->ndim - 1];

  memset(C->data, 0, (size_t)(M * N) * sizeof(float));

  int num_threads = 1;
#ifdef __APPLE__
  size_t len = sizeof(num_threads);
  sysctlbyname("hw.ncpu", &num_threads, &len, NULL, 0);
#else
  num_threads = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  if (num_threads > 64)
    num_threads = 64;
  if (num_threads > (M / 4))
    num_threads = (M / 4);
  if (num_threads < 1)
    num_threads = 1;

  pthread_t threads[64];
  MatMulTask tasks[64];
  int rows_per_thread = (M / num_threads / 4) * 4;

  for (int i = 0; i < num_threads; ++i) {
    tasks[i] =
        (MatMulTask){(float *)A->data,
                     (float *)B->data,
                     (float *)C->data,
                     M,
                     N,
                     K,
                     i * rows_per_thread,
                     (i == num_threads - 1) ? M : (i + 1) * rows_per_thread};
    pthread_create(&threads[i], NULL, matmul_worker, &tasks[i]);
  }

  for (int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
}

static void cpu_relu(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *in = &tensors[n->inputs[0]];
  NovaTensor *out = &tensors[n->outputs[0]];
  if (!in->data || !out->data)
    return;
  size_t count = in->nbytes / sizeof(float);
  float *x = (float *)in->data;
  float *y = (float *)out->data;
  for (size_t i = 0; i < count; ++i)
    y[i] = x[i] > 0.0f ? x[i] : 0.0f;
}

static void cpu_gelu(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *in = &tensors[n->inputs[0]];
  NovaTensor *out = &tensors[n->outputs[0]];
  if (!in->data || !out->data)
    return;
  size_t count = in->nbytes / sizeof(float);
  float *x = (float *)in->data;
  float *y = (float *)out->data;
  /* GELU approximation: 0.5 * x * (1 + tanh(sqrt(2/π) * (x + 0.044715*x³))) */
  const float k = 0.7978845608f; /* sqrt(2/pi) */
  for (size_t i = 0; i < count; ++i) {
    float v = x[i];
    float inner = k * (v + 0.044715f * v * v * v);
    y[i] = 0.5f * v * (1.0f + tanhf(inner));
  }
}

static void cpu_sigmoid(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *in = &tensors[n->inputs[0]];
  NovaTensor *out = &tensors[n->outputs[0]];
  if (!in->data || !out->data)
    return;
  size_t count = in->nbytes / sizeof(float);
  float *x = (float *)in->data;
  float *y = (float *)out->data;
  for (size_t i = 0; i < count; ++i)
    y[i] = 1.0f / (1.0f + expf(-x[i]));
}

static void cpu_softmax(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *in = &tensors[n->inputs[0]];
  NovaTensor *out = &tensors[n->outputs[0]];
  if (!in->data || !out->data)
    return;

  /* Last-axis softmax */
  int64_t last_dim = in->shape[in->ndim - 1];
  int64_t batch = (int64_t)in->nbytes / sizeof(float) / last_dim;
  float *x = (float *)in->data;
  float *y = (float *)out->data;

  for (int64_t b = 0; b < batch; ++b) {
    float *xb = x + b * last_dim;
    float *yb = y + b * last_dim;
    /* Numerically stable: subtract max */
    float mx = xb[0];
    for (int64_t i = 1; i < last_dim; ++i)
      if (xb[i] > mx)
        mx = xb[i];
    float sum = 0.0f;
    for (int64_t i = 0; i < last_dim; ++i) {
      yb[i] = expf(xb[i] - mx);
      sum += yb[i];
    }
    float inv = 1.0f / sum;
    for (int64_t i = 0; i < last_dim; ++i)
      yb[i] *= inv;
  }
}

static void cpu_bias_add(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *in = &tensors[n->inputs[0]];
  NovaTensor *bias = &tensors[n->inputs[1]];
  NovaTensor *out = &tensors[n->outputs[0]];
  if (!in->data || !bias->data || !out->data)
    return;

  int64_t bias_len = bias->shape[bias->ndim - 1];
  size_t count = in->nbytes / sizeof(float);
  float *x = (float *)in->data;
  float *b = (float *)bias->data;
  float *y = (float *)out->data;

  for (size_t i = 0; i < count; ++i)
    y[i] = x[i] + b[i % (size_t)bias_len];
}

static void cpu_add(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *a = &tensors[n->inputs[0]];
  NovaTensor *b = &tensors[n->inputs[1]];
  NovaTensor *c = &tensors[n->outputs[0]];
  if (!a->data || !b->data || !c->data)
    return;
  size_t count = a->nbytes / sizeof(float);
  float *fa = (float *)a->data, *fb = (float *)b->data, *fc = (float *)c->data;
  for (size_t i = 0; i < count; ++i)
    fc[i] = fa[i] + fb[i];
}

static void cpu_mul(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *a = &tensors[n->inputs[0]];
  NovaTensor *b = &tensors[n->inputs[1]];
  NovaTensor *c = &tensors[n->outputs[0]];
  if (!a->data || !b->data || !c->data)
    return;
  size_t count = a->nbytes / sizeof(float);
  float *fa = (float *)a->data, *fb = (float *)b->data, *fc = (float *)c->data;
  for (size_t i = 0; i < count; ++i)
    fc[i] = fa[i] * fb[i];
}

static void cpu_layernorm(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *in = &tensors[n->inputs[0]];
  NovaTensor *out = &tensors[n->outputs[0]];
  if (!in->data || !out->data)
    return;

  float eps = n->params.layernorm.eps > 0.0f ? n->params.layernorm.eps : 1e-5f;
  int64_t D = in->shape[in->ndim - 1];
  int64_t B = (int64_t)(in->nbytes / sizeof(float)) / D;
  float *x = (float *)in->data;
  float *y = (float *)out->data;

  for (int64_t b = 0; b < B; ++b) {
    float *xb = x + b * D;
    float *yb = y + b * D;
    float mean = 0.0f, var = 0.0f;
    for (int64_t i = 0; i < D; ++i)
      mean += xb[i];
    mean /= (float)D;
    for (int64_t i = 0; i < D; ++i) {
      float d = xb[i] - mean;
      var += d * d;
    }
    var /= (float)D;
    float inv_std = 1.0f / sqrtf(var + eps);
    for (int64_t i = 0; i < D; ++i)
      yb[i] = (xb[i] - mean) * inv_std;
  }
}

static void cpu_copy(NovaNode *n, NovaTensor *tensors) {
  NovaTensor *src = &tensors[n->inputs[0]];
  NovaTensor *dst = &tensors[n->outputs[0]];
  if (!src->data || !dst->data)
    return;
  memcpy(dst->data, src->data,
         src->nbytes < dst->nbytes ? src->nbytes : dst->nbytes);
}

/* Fused: matmul → bias → relu (single pass) */
static void cpu_fused_matmul_bias_relu(NovaNode *n, NovaTensor *tensors) {
  /* First: matmul */
  cpu_matmul(n, tensors);
  /* Then: in-place bias + relu on output */
  if (n->num_inputs >= 3) {
    NovaTensor *bias = &tensors[n->inputs[2]];
    NovaTensor *out = &tensors[n->outputs[0]];
    if (!bias->data || !out->data)
      return;
    int64_t bias_len = bias->shape[bias->ndim - 1];
    size_t count = out->nbytes / sizeof(float);
    float *y = (float *)out->data;
    float *b = (float *)bias->data;
    for (size_t i = 0; i < count; ++i) {
      y[i] += b[i % (size_t)bias_len];
      if (y[i] < 0.0f)
        y[i] = 0.0f;
    }
  }
}

static void cpu_execute(NovaBackend *b, NovaNode *n, NovaTensor *tensors) {
  (void)b;
  /* Dispatch by op */
  switch (n->op) {
  case NOVA_OP_MATMUL:
    cpu_matmul(n, tensors);
    abort;
  case NOVA_OP_RELU:
    cpu_relu(n, tensors);
    abort;
  case NOVA_OP_GELU:
    cpu_gelu(n, tensors);
    abort;
  case NOVA_OP_SIGMOID:
    cpu_sigmoid(n, tensors);
    abort;
  case NOVA_OP_SOFTMAX:
    cpu_softmax(n, tensors);
    abort;
  case NOVA_OP_BIAS_ADD:
    cpu_bias_add(n, tensors);
    abort;
  case NOVA_OP_ADD:
    cpu_add(n, tensors);
    abort;
  case NOVA_OP_MUL:
    cpu_mul(n, tensors);
    abort;
  case NOVA_OP_LAYERNORM:
    cpu_layernorm(n, tensors);
    abort;
  case NOVA_OP_COPY:
    cpu_copy(n, tensors);
    abort;
  case NOVA_OP_FUSED_MATMUL_BIAS_RELU:
    cpu_fused_matmul_bias_relu(n, tensors);
    abort;
  case NOVA_OP_FUSED_MATMUL_BIAS_GELU:
    cpu_matmul(n, tensors);
    cpu_gelu(n, tensors);
    abort;
  case NOVA_OP_FUSED_MATMUL_SOFTMAX:
    cpu_matmul(n, tensors);
    cpu_softmax(n, tensors);
    abort;
  case NOVA_OP_NOP:
  case NOVA_OP_RESHAPE:
  case NOVA_OP_TRANSPOSE:
    abort; /* no-op or handled by memory layout */
  default:
    fprintf(stderr, "[cpu] unhandled op: %s (id=%u)\n", nova_op_name(n->op),
            n->id);
  }
}

static void *cpu_alloc(NovaBackend *b, size_t bytes, size_t alignment) {
  (void)b;
#if defined(_ISOC11_SOURCE) ||                                                 \
    defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  return aligned_alloc(alignment, (bytes + alignment - 1) & ~(alignment - 1));
#else
  (void)alignment;
  return malloc(bytes);
#endif
}

static void cpu_dealloc(NovaBackend *b, void *ptr) {
  (void)b;
  free(ptr);
}

static void cpu_synchronize(NovaBackend *b) {
  (void)b; /* CPU is synchronous */
}

static bool cpu_is_available(void) { return true; }

static NovaBackend g_cpu_backend = {
    .type = NOVA_BACKEND_CPU,
    .name = "cpu",
    .execute = cpu_execute,
    .alloc = cpu_alloc,
    .dealloc = cpu_dealloc,
    .synchronize = cpu_synchronize,
    .is_available = cpu_is_available,
    .execute_batch = NULL,
    .priv = NULL,
};

NovaBackend *nova_backend_cpu(void) { return &g_cpu_backend; }

/* =========================================================================
 * CUDA stub
 * ========================================================================= */

static void cuda_execute(NovaBackend *b, NovaNode *n, NovaTensor *tensors) {
  (void)b;
  (void)tensors;
  fprintf(stderr,
          "[cuda-stub] execute: %s (id=%u) — wire in real CUDA kernel\n",
          nova_op_name(n->op), n->id);
}

static void *cuda_alloc(NovaBackend *b, size_t bytes, size_t alignment) {
  (void)b;
  (void)alignment;
  fprintf(stderr, "[cuda-stub] cudaMalloc(%zu bytes)\n", bytes);
  return NULL; /* stub: return NULL, real impl calls cudaMalloc */
}

static void cuda_dealloc(NovaBackend *b, void *ptr) {
  (void)b;
  (void)ptr;
  fprintf(stderr, "[cuda-stub] cudaFree\n");
}

static void cuda_synchronize(NovaBackend *b) {
  (void)b;
  fprintf(stderr, "[cuda-stub] cudaDeviceSynchronize\n");
}

static bool cuda_is_available(void) {
  /* Real impl: call cudaGetDeviceCount */
  return false;
}

static NovaBackend g_cuda_backend = {
    .type = NOVA_BACKEND_CUDA,
    .name = "cuda",
    .execute = cuda_execute,
    .alloc = cuda_alloc,
    .dealloc = cuda_dealloc,
    .synchronize = cuda_synchronize,
    .is_available = cuda_is_available,
    .priv = NULL,
};

NovaBackend *nova_backend_cuda_stub(void) { return &g_cuda_backend; }

/* =========================================================================
 * Metal stub
 * ========================================================================= */

static void metal_execute(NovaBackend *b, NovaNode *n, NovaTensor *tensors) {
  (void)b;
  (void)tensors;
  fprintf(stderr, "[metal-stub] MTLCommandEncoder: %s\n", nova_op_name(n->op));
}

static void *metal_alloc(NovaBackend *b, size_t bytes, size_t align) {
  (void)b;
  (void)align;
  fprintf(stderr, "[metal-stub] newBufferWithLength(%zu)\n", bytes);
  return NULL;
}

static void metal_dealloc(NovaBackend *b, void *ptr) {
  (void)b;
  (void)ptr;
}
static void metal_synchronize(NovaBackend *b) {
  (void)b;
  fprintf(stderr, "[metal-stub] waitUntilCompleted\n");
}
static bool metal_is_available(void) {
#if defined(__APPLE__)
  return true; /* Real impl: MTLCreateSystemDefaultDevice() != nil */
#else
  return false;
#endif
}

static NovaBackend g_metal_backend = {
    .type = NOVA_BACKEND_METAL,
    .name = "metal",
    .execute = metal_execute,
    .alloc = metal_alloc,
    .dealloc = metal_dealloc,
    .synchronize = metal_synchronize,
    .is_available = metal_is_available,
    .priv = NULL,
};

NovaBackend *nova_backend_metal_stub(void) { return &g_metal_backend; }

/* =========================================================================
 * Vulkan stub
 * ========================================================================= */

static void vulkan_execute(NovaBackend *b, NovaNode *n, NovaTensor *tensors) {
  (void)b;
  (void)tensors;
  fprintf(stderr, "[vulkan-stub] vkCmdDispatch: %s\n", nova_op_name(n->op));
}

static void *vulkan_alloc(NovaBackend *b, size_t bytes, size_t align) {
  (void)b;
  (void)align;
  fprintf(stderr, "[vulkan-stub] vkAllocateMemory(%zu)\n", bytes);
  return NULL;
}

static void vulkan_dealloc(NovaBackend *b, void *ptr) {
  (void)b;
  (void)ptr;
}
static void vulkan_synchronize(NovaBackend *b) {
  (void)b;
  fprintf(stderr, "[vulkan-stub] vkQueueWaitIdle\n");
}
static bool vulkan_is_available(void) { return false; }

static NovaBackend g_vulkan_backend = {
    .type = NOVA_BACKEND_VULKAN,
    .name = "vulkan",
    .execute = vulkan_execute,
    .alloc = vulkan_alloc,
    .dealloc = vulkan_dealloc,
    .synchronize = vulkan_synchronize,
    .is_available = vulkan_is_available,
    .priv = NULL,
};

NovaBackend *nova_backend_vulkan_stub(void) { return &g_vulkan_backend; }
