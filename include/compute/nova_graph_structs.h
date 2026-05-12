/**
 * ═══════════════════════════════════════════════════════════════════════════
 * nova_graph_structs.h — High-Level Compute Graph Structures
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * JIT ve Graph Verification tarafından kullanılan Compute Graph tanımları.
 * nova_ir.h (Low-Level SSA) ile karıştırılmamalıdır.
 */

#ifndef NOVA_GRAPH_STRUCTS_H
#define NOVA_GRAPH_STRUCTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declarations */
typedef struct NovaTensor NovaTensor;
typedef struct NovaIRNode NovaIRNode;
typedef struct NovaIREdge NovaIREdge;

/* ═══════════════════════════════════════════════════════════════════════════
 * GRAPH EDGE (Legacy / Edge-based view)
 * ═══════════════════════════════════════════════════════════════════════════
 */

struct NovaIREdge {
  NovaIRNode *source_node;
  int source_output_idx;
  NovaIRNode *target_node;
  int target_input_idx;
  int tensor_dtype; /* e.g. F32, F16 */
};

/* ═══════════════════════════════════════════════════════════════════════════
 * OP ATTRIBUTES — Per-operation parameters
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  int64_t stride_h, stride_w;
  int64_t pad_h, pad_w;
  int64_t dilation_h, dilation_w;
} NovaConv2DAttrs;

typedef struct {
  int64_t axis;
} NovaSoftmaxAttrs;

typedef struct {
  const int64_t *perm;
  int64_t perm_len;
} NovaTransposeAttrs;

typedef struct {
  const int64_t *new_shape;
  int64_t new_rank;
} NovaReshapeAttrs;

typedef union {
  NovaConv2DAttrs conv2d;
  NovaSoftmaxAttrs softmax;
  NovaTransposeAttrs transpose;
  NovaReshapeAttrs reshape;
} NovaNodeAttrs;

/* ═══════════════════════════════════════════════════════════════════════════
 * GRAPH NODE
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Dual-interface: supports both edge-based (graph builder) and
 * tensor-based (contract checker) access patterns.
 */

struct NovaIRNode {
  uint64_t node_id;

  /* Operation identifier — preferred by contract API */
  const char *op;

  /* Legacy alias (points to same string as 'op') */
  const char *op_name;

  int op_type; /* Enum identifier */

  /* ── Tensor-based I/O (used by contract system) ── */
  const NovaTensor **inputs;
  uint32_t num_inputs;

  NovaTensor **outputs;
  uint32_t num_outputs;

  /* ── Edge-based I/O (used by graph builder / lowering) ── */
  NovaIREdge **input_edges;
  int *input_types;
  int num_input_edges;

  NovaIREdge **output_edges;
  int *output_types;
  int num_output_edges;

  /* ── Operation attributes ── */
  NovaNodeAttrs attrs;

  /* ── Metadata ── */
  uint64_t output_size_bytes;
  void *op_params; /* Generic kernel params (legacy) */
};

/* ═══════════════════════════════════════════════════════════════════════════
 * COMPUTE GRAPH
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
  NovaIRNode **nodes;
  uint32_t num_nodes;
  const char *graph_name;

  /* Formal Verification Status - Used by Codegen */
  bool is_verified;      // Contracts & Topology checked
  bool is_memory_proven; // VRAM bounds checked

  /* Optimization Status */
  bool is_optimized;
} NovaIRGraph;

/* ═══════════════════════════════════════════════════════════════════════════
 * OPTIMIZER GRAPH NODES (Legacy / Optimizer View)
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* nova_opt_obligations.c tarafından kullanılıyor */
typedef enum {
  OP_UNKNOWN,
  OP_MATMUL,
  OP_ADD,
  OP_RELU,
  OP_SOFTMAX,
  OP_ATTENTION,
  OP_ADD_CONST,
  OP_FUSED_RELU_ADD
} NovaOp;

typedef struct {
  NovaOp op_type;
  const char *name;
  void *data;
} NovaGraphNode;

/* ═══════════════════════════════════════════════════════════════════════════
 * HELPER PROTOTYPES
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* This is often a macro or external function in real system */
uint64_t get_device_memory_limit(void);

#endif /* NOVA_GRAPH_STRUCTS_H */
