#ifndef NOVA_KERNEL_FUSION_H
#define NOVA_KERNEL_FUSION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Public fused op identifiers (stable ABI) ----
typedef enum NovaFusedOp {
  NOVA_FUSED_NONE = 0,

  // ML critical paths
  NOVA_FUSED_MATMUL_ADD_RELU_F32 = 1001,
  NOVA_FUSED_CONV2D_BIAS_RELU_F32 = 1002,
  NOVA_FUSED_ATTENTION_F32 = 1003, // Hyper-Flash / Nova Ultimatum
  NOVA_FUSED_FLASH_ATTN = 1004,
  NOVA_FUSED_FLASH_ATTN_SYNT_PE = 1005,
  NOVA_FUSED_MATMUL_DELTA = 1006,

} NovaFusedOp;

// ---- Graph abstraction via vtable (no hard dependency on graph structs) ----
// The fusion engine only needs these primitive operations.
typedef struct NovaExecGraphVTable {
  // iteration
  int64_t (*node_count)(void *g);
  int64_t (*node_id_at)(void *g, int64_t index);

  // node queries
  int32_t (*node_op)(void *g, int64_t node_id);
  int32_t (*node_dtype)(void *g, int64_t node_id);

  int64_t (*node_input_count)(void *g, int64_t node_id);
  int64_t (*node_input)(void *g, int64_t node_id, int64_t input_index);

  int64_t (*node_output_count)(void *g, int64_t node_id);
  int64_t (*node_output)(void *g, int64_t node_id, int64_t output_index);

  // rewrite primitives
  // create a new node with given op + inputs. returns new node_id.
  int64_t (*node_create)(void *g, int32_t op, const int64_t *inputs,
                         int64_t n_inputs);

  // replace all uses of old_node outputs with new_node outputs (1:1 assumed).
  void (*node_replace_uses)(void *g, int64_t old_node_id, int64_t new_node_id);

  // delete a node (must be safe after replace_uses)
  void (*node_delete)(void *g, int64_t node_id);

  // optional: annotate for debugging/profiling (may be NULL)
  void (*node_set_tag)(void *g, int64_t node_id, const char *tag);
} NovaExecGraphVTable;

// ---- Fusion registry (maps fused-op -> backend kernel symbol id) ----
typedef struct NovaFusionKernel {
  NovaFusedOp fused_op;
  const char *kernel_name; // e.g. "nova_kernel_matmul_add_relu_f32"
} NovaFusionKernel;

// Initialize with built-in patterns + fused-kernel registry
void nova_fusion_init(void);

// Register a fused kernel implementation name (call from backend init)
void nova_fusion_register(NovaFusedOp fused_op, const char *kernel_name);

// Runs fusion passes; returns number of fusions performed
int64_t nova_fusion_run(void *graph, const NovaExecGraphVTable *vt);

// Query the registry (returns NULL if not registered)
const NovaFusionKernel *nova_fusion_lookup(NovaFusedOp fused_op);

#ifdef __cplusplus
}
#endif

#endif // NOVA_KERNEL_FUSION_H
