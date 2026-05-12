/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_gc_concurrent.c - Concurrent Garbage Collector Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_gc_concurrent.h"
#include <stdio.h>
#include <stdlib.h>

NovaConcurrentGC *nova_concurrent_gc_init(size_t young_size,
                                              size_t old_size) {
  (void)young_size;
  (void)old_size;
  NovaConcurrentGC *gc = calloc(1, sizeof(NovaConcurrentGC));
  return gc;
}

void nova_concurrent_gc_destroy(NovaConcurrentGC *gc) {
  if (!gc)
    return;
  free(gc);
}

void nova_concurrent_gc_start(NovaConcurrentGC *gc) { (void)gc; }

void nova_concurrent_gc_stop(NovaConcurrentGC *gc) { (void)gc; }

void nova_concurrent_gc_collect(NovaConcurrentGC *gc) { (void)gc; }

ConcurrentGCStats nova_concurrent_gc_stats(NovaConcurrentGC *gc) {
  (void)gc;
  ConcurrentGCStats stats = {0};
  return stats;
}

void nova_concurrent_gc_print_stats(NovaConcurrentGC *gc) {
  (void)gc;
  printf("Concurrent GC Stats: [STUB]\n");
}
