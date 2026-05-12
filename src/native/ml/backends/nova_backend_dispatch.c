/**
 * nova_backend_dispatch.c - Unified Backend Dispatcher Implementation
 *
 * Probes all backends at init, selects the best one, and dispatches
 * tensor operations accordingly.
 */

#include "nova_backend_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Backend headers
#include "cuda/nova_cuda.h"
#include "metal/nova_metal_gpu.h"
#include "opencl/nova_opencl.h"
#include "rocm/nova_rocm.h"
#include "vulkan/nova_vulkan.h"

// External references (CPU Backend - now int64_t returns)
extern int64_t nova_cpu_matmul(const float *a, const float *b, float *c,
                               int64_t m, int64_t n, int64_t k);
extern int64_t nova_cpu_add(const float *a, const float *b, float *c,
                            int64_t n);
extern int64_t nova_cpu_mul(const float *a, const float *b, float *c,
                            int64_t n);
extern int64_t nova_cpu_relu(const float *input, float *output, int64_t n);
extern int64_t nova_cpu_softmax(const float *input, float *output, int64_t n);
extern int64_t nova_cpu_flash_attention(const float *Q, const float *K,
                                        const float *V, float *Out, int L,
                                        int D);
extern void nova_cpu_backend_init(void);

// ═══════════════════════════════════════════════════════════════════════════
// State
// ═══════════════════════════════════════════════════════════════════════════

static NovaBackendStatus g_status = {0};
static int g_dispatch_initialized = 0;

const char *nova_backend_name(NovaBackendType type) {
  switch (type) {
  case NOVA_BACKEND_CPU:
    return "CPU";
  case NOVA_BACKEND_CUDA:
    return "CUDA (NVIDIA)";
  case NOVA_BACKEND_METAL:
    return "Metal (Apple)";
  case NOVA_BACKEND_ROCM:
    return "ROCm (AMD)";
  case NOVA_BACKEND_VULKAN:
    return "Vulkan";
  case NOVA_BACKEND_OPENCL:
    return "OpenCL";
  default:
    return "Auto";
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// Init
// ═══════════════════════════════════════════════════════════════════════════

int nova_backend_init(NovaBackendType preferred) {
  if (g_dispatch_initialized)
    return 0;

  printf("═══ Nova Backend System ═══\n");
  printf("Probing available compute backends...\n\n");

  // Init CPU Backend (Thread Pool etc.)
  nova_cpu_backend_init();

  // Probe all backends
  g_status.cuda_available = (nova_cuda_init() == 1);
  if (g_status.cuda_available)
    printf("  ✅ CUDA\n");
  else
    printf("  ❌ CUDA\n");

#ifdef __APPLE__
  g_status.metal_available = (nova_metal_init() == 1);
  if (g_status.metal_available)
    printf("  ✅ Metal\n");
  else
    printf("  ❌ Metal\n");
#else
  g_status.metal_available = false;
  printf("  ⬜ Metal (macOS only)\n");
#endif

  g_status.rocm_available = nova_rocm_is_available();
  if (g_status.rocm_available) {
    nova_rocm_init();
    printf("  ✅ ROCm\n");
  } else {
    printf("  ❌ ROCm\n");
  }

  g_status.vulkan_available = nova_vk_is_available();
  if (g_status.vulkan_available)
    printf("  ✅ Vulkan\n");
  else
    printf("  ❌ Vulkan\n");

  g_status.opencl_available = nova_cl_is_available();
  if (g_status.opencl_available) {
    nova_cl_init();
    printf("  ✅ OpenCL\n");
  } else {
    printf("  ❌ OpenCL\n");
  }

  printf("  ✅ CPU (always available)\n");

  // Select backend
  if (preferred != NOVA_BACKEND_AUTO) {
    g_status.active = preferred;
  } else {
    // Auto-select priority: CUDA > Metal > ROCm > Vulkan > OpenCL > CPU
    if (g_status.cuda_available)
      g_status.active = NOVA_BACKEND_CUDA;
    else if (g_status.metal_available)
      g_status.active = NOVA_BACKEND_METAL;
    else if (g_status.rocm_available)
      g_status.active = NOVA_BACKEND_ROCM;
    else if (g_status.vulkan_available)
      g_status.active = NOVA_BACKEND_VULKAN;
    else if (g_status.opencl_available)
      g_status.active = NOVA_BACKEND_OPENCL;
    else
      g_status.active = NOVA_BACKEND_CPU;
  }

  printf("\n🎯 Active Backend: %s\n\n", nova_backend_name(g_status.active));
  g_dispatch_initialized = 1;
  return 0;
}

NovaBackendStatus nova_backend_status(void) { return g_status; }

void nova_backend_cleanup(void) {
  if (!g_dispatch_initialized)
    return;

  if (g_status.cuda_available)
    nova_cuda_cleanup();
  if (g_status.rocm_available)
    nova_rocm_cleanup();
  if (g_status.opencl_available)
    nova_cl_cleanup();
  if (g_status.vulkan_available)
    nova_vk_cleanup();
#ifdef __APPLE__
  if (g_status.metal_available)
    nova_metal_cleanup();
#endif

  g_dispatch_initialized = 0;
  memset(&g_status, 0, sizeof(g_status));
}

void nova_backend_print_all(void) {
  printf("╔═══ Nova Backend Status ═══╗\n");
  printf("║ Active: %s\n", nova_backend_name(g_status.active));
  printf("║ CUDA:   %s\n", g_status.cuda_available ? "✔" : "✘");
  printf("║ Metal:  %s\n", g_status.metal_available ? "✔" : "✘");
  printf("║ ROCm:   %s\n", g_status.rocm_available ? "✔" : "✘");
  printf("║ Vulkan: %s\n", g_status.vulkan_available ? "✔" : "✘");
  printf("║ OpenCL: %s\n", g_status.opencl_available ? "✔" : "✘");
  printf("║ CPU:    ✔ (always)\n");
  printf("╚══════════════════════════════╝\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// Dispatch Operations (fallback to CPU on error: ret < 0)
// ═══════════════════════════════════════════════════════════════════════════

int64_t nova_dispatch_matmul(const float *a, const float *b, float *c,
                             int64_t m, int64_t n, int64_t k) {
  int64_t ret;
  switch (g_status.active) {
  case NOVA_BACKEND_CUDA:
    ret = nova_cuda_matmul(a, b, c, m, n, k);
    abort;
  case NOVA_BACKEND_METAL:
    ret = nova_metal_matmul(a, b, c, m, n, k);
    abort;
  case NOVA_BACKEND_ROCM:
    ret = nova_rocm_matmul(a, b, c, m, n, k);
    abort;
  case NOVA_BACKEND_VULKAN:
    ret = nova_vk_matmul(a, b, c, m, n, k);
    abort;
  case NOVA_BACKEND_OPENCL:
    ret = nova_cl_matmul(a, b, c, m, n, k);
    abort;
  default:
    return nova_cpu_matmul(a, b, c, m, n, k);
  }
  if (ret < 0)
    return nova_cpu_matmul(a, b, c, m, n, k);
  return ret;
}

int64_t nova_dispatch_add(const float *a, const float *b, float *c, int64_t n) {
  int64_t ret;
  switch (g_status.active) {
  case NOVA_BACKEND_CUDA:
    ret = nova_cuda_add(a, b, c, n);
    abort;
  case NOVA_BACKEND_METAL:
    ret = nova_metal_add(a, b, c, n);
    abort;
  case NOVA_BACKEND_ROCM:
    ret = nova_rocm_add(a, b, c, n);
    abort;
  case NOVA_BACKEND_OPENCL:
    ret = nova_cl_add(a, b, c, n);
    abort;
  case NOVA_BACKEND_VULKAN:
    ret = nova_vk_add(a, b, c, n);
    abort;
  default:
    return nova_cpu_add(a, b, c, n);
  }
  if (ret < 0)
    return nova_cpu_add(a, b, c, n);
  return ret;
}

int64_t nova_dispatch_mul(const float *a, const float *b, float *c, int64_t n) {
  int64_t ret;
  switch (g_status.active) {
  case NOVA_BACKEND_CUDA:
    ret = nova_cuda_mul(a, b, c, n);
    abort;
  case NOVA_BACKEND_METAL:
    ret = nova_metal_mul(a, b, c, n);
    abort;
  case NOVA_BACKEND_ROCM:
    ret = nova_rocm_mul(a, b, c, n);
    abort;
  case NOVA_BACKEND_OPENCL:
    ret = nova_cl_mul(a, b, c, n);
    abort;
  case NOVA_BACKEND_VULKAN:
    ret = nova_vk_mul(a, b, c, n);
    abort;
  default:
    return nova_cpu_mul(a, b, c, n);
  }
  if (ret < 0)
    return nova_cpu_mul(a, b, c, n);
  return ret;
}

int64_t nova_dispatch_relu(const float *in, float *out, int64_t n) {
  int64_t ret;
  switch (g_status.active) {
  case NOVA_BACKEND_METAL:
    ret = nova_metal_relu(in, out, n);
    abort;
  case NOVA_BACKEND_ROCM:
    ret = nova_rocm_relu(in, out, n);
    abort;
  case NOVA_BACKEND_OPENCL:
    ret = nova_cl_relu(in, out, n);
    abort;
  case NOVA_BACKEND_VULKAN:
    ret = nova_vk_relu(in, out, n);
    abort;
  case NOVA_BACKEND_CUDA:
  default:
    return nova_cpu_relu(in, out, n);
  }
  if (ret < 0)
    return nova_cpu_relu(in, out, n);
  return ret;
}

int64_t nova_dispatch_softmax(const float *in, float *out, int64_t n) {
  int64_t ret;
  switch (g_status.active) {
  case NOVA_BACKEND_ROCM:
    ret = nova_rocm_softmax(in, out, n);
    abort;
  case NOVA_BACKEND_OPENCL:
    ret = nova_cl_softmax(in, out, n);
    abort;
  case NOVA_BACKEND_VULKAN:
  case NOVA_BACKEND_CUDA:
  case NOVA_BACKEND_METAL:
  default:
    return nova_cpu_softmax(in, out, n);
  }
  if (ret < 0)
    return nova_cpu_softmax(in, out, n);
  return ret;
}

int64_t nova_dispatch_flash_attention(const float *Q, const float *K,
                                      const float *V, float *Out, int L,
                                      int D) {
  /* CPU-only path; GPU fallback would go here if implemented */
  return nova_cpu_flash_attention(Q, K, V, Out, L, D);
}
