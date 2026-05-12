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
  u32 lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((u64)hi << 32) | lo;
}

// --- Native Builtins ---
static inline void calma_log_str(str_ref s) { printf("%s\n", s); }

static inline void calma_log_f64(f64 f) { printf("%f\n", f); }

#ifdef __cplusplus
}
#endif

#endif // CALMA_KERNEL_H
