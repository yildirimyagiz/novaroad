/*
 * nova_trace.c — Performance Instrumentation
 *
 * Lightweight trace ring: records per-node timing, memory traffic,
 * and aggregate stats. Zero overhead when disabled.
 */

#include "nova.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Per-node timestamps stored across begin/end calls */
static uint64_t *g_pending_ts = NULL;
static uint32_t g_pending_cap = 0;

NovaTrace *nova_trace_create(uint32_t capacity) {
  NovaTrace *t = calloc(1, sizeof(NovaTrace));
  if (!t)
    return NULL;
  t->events = calloc(capacity, sizeof(NovaTraceEvent));
  t->capacity = capacity;
  t->enabled = true;

  if (!t->events) {
    free(t);
    return NULL;
  }

  /* Pending timestamps indexed by node_id (up to 4096 nodes) */
  g_pending_cap = 4096;
  g_pending_ts = calloc(g_pending_cap, sizeof(uint64_t));

  return t;
}

void nova_trace_destroy(NovaTrace *t) {
  if (!t)
    return;
  free(t->events);
  free(t);
  free(g_pending_ts);
  g_pending_ts = NULL;
  g_pending_cap = 0;
}

void nova_trace_begin(NovaTrace *t, uint32_t node_id) {
  if (!t || !t->enabled)
    return;
  if (g_pending_ts && node_id < g_pending_cap)
    g_pending_ts[node_id] = nova_clock_ns();
}

void nova_trace_end(NovaTrace *t, uint32_t node_id, uint64_t mem_read,
                      uint64_t mem_written) {
  if (!t || !t->enabled)
    return;
  if (t->count >= t->capacity)
    return; /* ring full — drop */

  uint64_t now = nova_clock_ns();
  NovaTraceEvent *ev = &t->events[t->count++];
  ev->node_id = node_id;
  ev->ts_begin =
      (g_pending_ts && node_id < g_pending_cap) ? g_pending_ts[node_id] : now;
  ev->ts_end = now;
  ev->mem_read = mem_read;
  ev->mem_written = mem_written;
}

void nova_trace_print(const NovaTrace *t, const NovaGraph *g) {
  if (!t || t->count == 0) {
    printf("[trace] empty\n");
    return;
  }

  printf(
      "\n╔══════════════════════════════════════════════════════════════╗\n");
  printf("║  Nova Execution Trace — %u events                          \n",
         t->count);
  printf("╠══════════════════════════════════════════════════════════════╣\n");
  printf("║ %-4s  %-28s  %10s  %10s  %10s\n", "ID", "Name", "Latency(µs)",
         "Read(KB)", "Write(KB)");
  printf("╠══════════════════════════════════════════════════════════════╣\n");

  uint64_t total_ns = 0;
  uint64_t total_rd = 0;
  uint64_t total_wr = 0;

  for (uint32_t i = 0; i < t->count; ++i) {
    const NovaTraceEvent *ev = &t->events[i];
    uint64_t lat_us = (ev->ts_end - ev->ts_begin) / 1000;
    total_ns += (ev->ts_end - ev->ts_begin);
    total_rd += ev->mem_read;
    total_wr += ev->mem_written;

    const char *name = "?";
    if (g) {
      const NovaNode *nd =
          (ev->node_id < g->num_nodes) ? &g->nodes[ev->node_id] : NULL;
      if (nd)
        name = nd->name;
    }

    printf("║ %-4u  %-28s  %10llu  %10.1f  %10.1f\n", ev->node_id, name,
           (unsigned long long)lat_us, (double)ev->mem_read / 1024.0,
           (double)ev->mem_written / 1024.0);
  }

  printf("╠══════════════════════════════════════════════════════════════╣\n");
  printf("║ TOTAL                                %10.2f  %10.1f  %10.1f\n",
         (double)total_ns / 1000.0, (double)total_rd / 1024.0,
         (double)total_wr / 1024.0);
  printf(
      "╚══════════════════════════════════════════════════════════════╝\n\n");

  /* Throughput */
  if (total_ns > 0) {
    double gb_per_s = (double)(total_rd + total_wr) / (double)total_ns;
    printf("  Memory bandwidth (approx): %.2f GB/s\n", gb_per_s);
  }

  if (g) {
    printf("  Kernel launches: %u\n", g->kernel_launches);
    printf("  Fusions applied: %u\n", g->fusions_applied);
  }
  printf("\n");
}
