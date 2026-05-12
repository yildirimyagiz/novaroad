/**
 * src/tools/nova_chaos_lab.c
 * NOVA CHAOS LAB - Experimental Graph Construction & Fuzzing
 *
 * "Order from Chaos" - Fixing the broken graph construction logic.
 */

#include "nova_graph_structs.h"
#include "nova_proof.h"
#include "nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  printf("🧪 Nova Chaos Lab: Graph Construction Test\n");

  // Create a dummy node
  NovaIRNode *node = calloc(1, sizeof(NovaIRNode));

  // FIX: 'id' -> 'node_id'
  node->node_id = 42;

  // FIX: 'OP_MUL' -> 'OP_MATMUL' (or add OP_MUL to enum if needed, but using
  // existing for now)
  node->op_type = OP_MATMUL;
  node->op = "MatMul_Chaos";

  // FIX: 'metadata' -> No such member. Using 'op_params' for extra data if
  // needed. node->metadata = ...; // REMOVED
  node->op_params = NULL;

  // Proof Level Checks
  // FIX: 'NOVA_PROOF_REASSOC_LEGAL' -> 'PROOF_HEURISTIC' or internal flag
  ProofLevel p_level = PROOF_HEURISTIC;

  // FIX: 'NOVA_PROOF_BIT_EXACT' -> 'PROOF_VERIFIED'
  ProofLevel d_level = PROOF_VERIFIED;

  printf("Node ID: %llu, Op: %s\n", node->node_id, node->op);
  printf("Proof Level: %d, Determinism Level: %d\n", p_level, d_level);

  // FIX: 'NOVA_DEVICE_GPU' -> 'NOVA_DEVICE_METAL_GPU'
  NovaDevice dev = NOVA_DEVICE_METAL_GPU;
  printf("Target Device: %d\n", dev);

  free(node);
  return 0;
}
