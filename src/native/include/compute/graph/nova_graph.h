#ifndef NOVA_GRAPH_H
#define NOVA_GRAPH_H

#include "../ml/nova_tensor.h"
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA EXECUTION GRAPH - Lazy Computation Engine
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef enum {
  NOVA_EXEC_NODE_OP,
  NOVA_EXEC_NODE_INPUT,
  NOVA_NODE_CONSTANT,
  NOVA_EXEC_NODE_PLACEHOLDER
} NovaGraphNodeType;

typedef enum {
  NOVA_EXEC_OP_MATMUL,
  NOVA_EXEC_OP_ADD,
  NOVA_OP_SUB,
  NOVA_EXEC_OP_MUL,
  NOVA_OP_DIV,
  NOVA_EXEC_OP_ACTIVATION,
  NOVA_OP_CONV2D,
  NOVA_OP_ATTENTION,
  NOVA_OP_NORM,
  NOVA_OP_CUSTOM,
  NOVA_EXEC_OP_FUSED
} NovaExecGraphOpType;

typedef struct NovaExecGraphNode {
  uint64_t id;
  NovaGraphNodeType type;
  NovaExecGraphOpType op_type;

  // Connections
  struct NovaExecGraphNode **inputs;
  int input_count;
  struct NovaExecGraphNode **outputs;
  int output_count;

  // Result representation
  NovaTensor *tensor;

  // Execution metadata
  bool is_executed;
  int64_t scheduled_offset; // For memory planning
  int preferred_device_id;

  char *name;
  void *params; // Op-specific parameters (e.g. activation type)
} NovaExecGraphNode;

typedef struct {
  NovaContext *ctx;
  NovaExecGraphNode **nodes;
  int node_count;
  int node_capacity;

  bool is_finalized;
  uint64_t version;
} NovaExecGraph;

// Graph Lifecycle
NovaExecGraph *nova_exec_graph_create(NovaContext *ctx);
void nova_exec_graph_destroy(NovaExecGraph *graph);

// Node Creation
NovaExecGraphNode *nova_exec_graph_add_input(NovaExecGraph *graph, NovaTensor *t,
                                        const char *name);
NovaExecGraphNode *nova_exec_graph_add_op(NovaExecGraph *graph, NovaExecGraphOpType op,
                                     NovaExecGraphNode **inputs, int input_count,
                                     const char *name);

// Compilation & Execution
int nova_exec_graph_compile(NovaExecGraph *graph);
int nova_exec_graph_execute(NovaExecGraph *graph);

#endif // NOVA_GRAPH_H
