/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA KERNEL CAPABILITIES - Runtime Backend Detection
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "../../../include/nova_kernels.h"
#include <stdio.h>
#include <string.h>

// External backend initialization functions
#if NOVA_HAS_CUDA
extern int64_t nova_cuda_init(void);
extern int64_t nova_cuda_get_device_count(void);
extern void nova_cuda_cleanup(void);
#endif

#if NOVA_HAS_METAL
extern int64_t nova_metal_init(void);
extern void nova_metal_cleanup(void);
#endif

static NovaKernelCapabilities g_capabilities = {0};
static bool g_capabilities_initialized = false;

NovaKernelCapabilities nova_kernel_get_capabilities(void) {
  if (g_capabilities_initialized) {
    return g_capabilities;
  }

  // CPU is always available
  g_capabilities.cpu_available = true;
  g_capabilities.simd_available = NOVA_HAS_SIMD;

#if NOVA_HAS_CUDA
  int64_t cuda_status = nova_cuda_init();
  if (cuda_status == 0) {
    g_capabilities.cuda_available = true;
    g_capabilities.cuda_device_count = (int)nova_cuda_get_device_count();
  } else {
    g_capabilities.cuda_available = false;
    g_capabilities.cuda_device_count = 0;
  }
#else
  g_capabilities.cuda_available = false;
  g_capabilities.cuda_device_count = 0;
#endif

#if NOVA_HAS_METAL
  int64_t metal_status = nova_metal_init();
  if (metal_status == 0) {
    g_capabilities.metal_available = true;
    g_capabilities.metal_device_count = 1; // Typically one Metal device
  } else {
    g_capabilities.metal_available = false;
    g_capabilities.metal_device_count = 0;
  }
#else
  g_capabilities.metal_available = false;
  g_capabilities.metal_device_count = 0;
#endif

  // ROCm support (future)
  g_capabilities.rocm_available = false;
  g_capabilities.rocm_device_count = 0;

  g_capabilities_initialized = true;
  return g_capabilities;
}

void nova_kernel_print_capabilities(void) {
  NovaKernelCapabilities caps = nova_kernel_get_capabilities();

  printf("═══════════════════════════════════════════════════════════════════════════\n");
  printf("NOVA KERNEL CAPABILITIES\n");
  printf("═══════════════════════════════════════════════════════════════════════════\n");
  
  printf("CPU Backend:    %s\n", caps.cpu_available ? "✓ Available" : "✗ Not Available");
  printf("SIMD Support:   %s", caps.simd_available ? "✓ Available" : "✗ Not Available");
  
#if defined(__x86_64__) || defined(_M_X64)
  printf(" (x86_64 AVX/SSE)\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
  printf(" (ARM NEON)\n");
#else
  printf("\n");
#endif

  printf("Metal Backend:  %s", caps.metal_available ? "✓ Available" : "✗ Not Available");
  if (caps.metal_available) {
    printf(" (%d device%s)\n", caps.metal_device_count, 
           caps.metal_device_count == 1 ? "" : "s");
  } else {
    printf("\n");
  }

  printf("CUDA Backend:   %s", caps.cuda_available ? "✓ Available" : "✗ Not Available");
  if (caps.cuda_available) {
    printf(" (%d device%s)\n", caps.cuda_device_count,
           caps.cuda_device_count == 1 ? "" : "s");
  } else {
    printf("\n");
  }

  printf("ROCm Backend:   %s\n", caps.rocm_available ? "✓ Available" : "✗ Not Available");
  
  printf("═══════════════════════════════════════════════════════════════════════════\n");
}

void nova_kernel_subsystem_init(void) {
  // Initialize capabilities (will auto-detect backends)
  nova_kernel_get_capabilities();
  nova_kernel_print_capabilities();
}

void nova_kernel_subsystem_cleanup(void) {
#if NOVA_HAS_CUDA
  if (g_capabilities.cuda_available) {
    nova_cuda_cleanup();
  }
#endif

#if NOVA_HAS_METAL
  if (g_capabilities.metal_available) {
    nova_metal_cleanup();
  }
#endif

  g_capabilities_initialized = false;
  memset(&g_capabilities, 0, sizeof(g_capabilities));
}
