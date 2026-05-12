#ifndef NOVA_CONTEXT_H
#define NOVA_CONTEXT_H

typedef struct NovaContext NovaContext;

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA CONTEXT - The Kernel Backbone
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Centralized state management for the Nova Compute OS.
 * Every runtime operation MUST reference a context. No global state allowed.
 */

// Forward declarations using typedef names
typedef struct NovaProfilerV2 NovaProfilerV2;
typedef struct NovaAdaptiveOptimizer NovaAdaptiveOptimizer;
typedef struct NovaExecutionFabric NovaExecutionFabric;
typedef struct NovaCognitiveScheduler NovaCognitiveScheduler;
typedef struct NovaInvariantEngine NovaInvariantEngine;
typedef struct NovaLearningRuntime NovaLearningRuntime;
typedef struct NovaComputeEconomics NovaComputeEconomics;
typedef struct NovaFaultIntelligence NovaFaultIntelligence;




#include "../formal/nova_formal.h"


typedef enum { SMT_BACKEND_NONE, SMT_BACKEND_CVC5 } SmtBackend;

typedef enum { SYMEXEC_BACKEND_NONE, SYMEXEC_BACKEND_KLEE } SymExecBackend;
typedef struct {
  bool strict_determinism;
  bool enable_profiling;
  bool adaptive_optimization;
  uint32_t random_seed;
  size_t default_pool_size;

  // Formal Layer Config
  NovaFormalMode formal_mode;
  SmtBackend solver_backend;
  SymExecBackend symexec_backend;
} ContextConfig;

struct NovaContext {
  ContextConfig config;

  // System Layers
  NovaExecutionFabric *fabric;
  NovaCognitiveScheduler *scheduler;
  NovaAdaptiveOptimizer *optimizer;
  NovaProfilerV2 *profiler;
  NovaInvariantEngine *invariant_engine;
  NovaLearningRuntime *knowledge_base;
  NovaComputeEconomics *economics;
  NovaFaultIntelligence *fault_monitor;

  // Recurrence Cache (Sweet Spot Strategy)
  struct {
    void *data;
    int64_t seq;
    int64_t dim;
    bool valid;
  } pe_cache;

  // Execution State
  uint64_t logical_clock;
  int active_backend_id;

  // Memory State
  void *global_memory_pool;
  size_t pool_capacity;
  size_t pool_usage;
};

// Lifecycle
NovaContext *nova_context_create(ContextConfig config);
NovaContext *nova_context_create_default(void);
void nova_context_destroy(NovaContext *ctx);

// State Management
void nova_context_sync(NovaContext *ctx);
void nova_context_yield(NovaContext *ctx);

#endif // NOVA_CONTEXT_H
