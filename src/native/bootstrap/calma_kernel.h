#ifndef CALMA_KERNEL_H
#define CALMA_KERNEL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * CALMA SOVEREIGN KERNEL
 * High-performance, zero-legacy systems interface for Nova.
 */

#ifdef __cplusplus
extern "C" {
#endif

// --- Type System ---
typedef int32_t i32;
typedef int64_t i64;
typedef uint32_t u32;
typedef uint64_t u64;
typedef double f64;
typedef const char *str_ref;

// --- Memory Regions (Sovereign Memory Model) ---
typedef struct {
  void *base;
  size_t size;
  size_t offset;
} CalmaRegion;

static inline CalmaRegion *calma_region_create(size_t size) {
  CalmaRegion *region = (CalmaRegion *)malloc(sizeof(CalmaRegion));
  region->base = malloc(size);
  region->size = size;
  region->offset = 0;
  return region;
}

static inline void *calma_alloc(CalmaRegion *region, size_t bytes) {
  // Aligned allocation for performance
  size_t aligned = (bytes + 7) & ~7;
  if (region->offset + aligned > region->size)
    return NULL;
  void *ptr = (uint8_t *)region->base + region->offset;
  region->offset += aligned;
  return ptr;
}

// --- Hardware Pulse ---
static inline u64 calma_clock_ticks() {
  u64 val;
#if defined(__x86_64__) || defined(__i386__)
  u32 lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  val = ((u64)hi << 32) | lo;
#elif defined(__aarch64__)
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
#else
  val = 0; // Fallback
#endif
  return val;
}

// --- Sovereign Hardware Dispatcher ---
typedef enum {
  CALMA_BACKEND_CPU = 0,
  CALMA_BACKEND_CUDA,
  CALMA_BACKEND_METAL,
  CALMA_BACKEND_ARMY_VPU // 🦅 High-tier Sovereign VPU
} CalmaBackend;

static inline CalmaBackend calma_get_active_backend() {
#if defined(__x86_64__) || defined(__aarch64__)
  #if defined(__APPLE__)
    return CALMA_BACKEND_METAL;
  #else
    return CALMA_BACKEND_CPU; // Default for x86/ARM Linux/Win
  #endif
#else
  return CALMA_BACKEND_CPU;
#endif
}

// --- Acceleration Stubs (Muscle) ---
static inline void calma_gpu_dispatch(const char* kernel_name) {
    printf("🚀 [SOVEREIGN GPU] Dispatching kernel: %s via %s\n", 
           kernel_name, 
           calma_get_active_backend() == CALMA_BACKEND_METAL ? "METAL" : "CUDA/CPU");
}

// --- Native Builtins ---
static inline void calma_log_str(str_ref s) { printf("%s\n", s); }
static inline void calma_log_f64(f64 f) { printf("%f\n", f); }

// --- Sovereign AI Support ---
typedef struct nova_tensor nova_tensor_t;
typedef struct nova_grad_tape nova_grad_tape_t;

typedef nova_tensor_t* tensor;
typedef nova_grad_tape_t* tape;

// Autograd Engine Stubs (For Demo)
static inline tape nova_grad_tape_create(void) { printf("  [AI] Tape Created\n"); return NULL; }
static inline void nova_grad_tape_begin(tape t) { printf("  [AI] Recording Started\n"); }
static inline void nova_grad_tape_watch(tape t, tensor x) { printf("  [AI] Watching Tensor\n"); }
static inline int  nova_grad_tape_backward(tape t, tensor loss) { printf("  [AI] Backward Pass Executed\n"); return 0; }
static inline tensor nova_tensor_create(const int64_t* shape, int64_t ndim, int dtype) { printf("  [AI] Tensor Allocated\n"); return NULL; }

// Tensor Factory
extern nova_tensor_t *nova_tensor_create_real(const size_t *shape, size_t ndim, int dtype);
extern void nova_tensor_destroy(nova_tensor_t *tensor);

#ifdef __cplusplus
}
#endif

#endif // CALMA_KERNEL_H
