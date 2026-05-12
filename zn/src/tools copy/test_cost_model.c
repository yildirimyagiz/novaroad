/**
 * test_cost_model.c - Test for Nova Graph Cost Model
 */

#include "nova_ast.h"
#include "nova_cost_model.h"
#include "nova_graph_structs.h"
#include "nova_lower_graph.h"
#include "nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * STUBS FOR LINKING (Copied from test_formal_graph.c)
 * ═══════════════════════════════════════════════════════════════════════════
 */

void *nova_malloc(size_t size) { return malloc(size); }
void nova_free(void *ptr) { free(ptr); }
void *nova_malloc_aligned(size_t size, size_t align) {
  (void)align;
  return malloc(size);
}
uint64_t get_device_memory_limit(void) { return 1024ULL * 1024 * 1024; }

/* ═══════════════════════════════════════════════════════════════════════════
 * MOCK HELPERS
 * ═══════════════════════════════════════════════════════════════════════════
 */

static ASTNode *create_ident(const char *name) {
  ASTNode *n = calloc(1, sizeof(ASTNode));
  n->type = AST_IDENTIFIER;
  n->data.identifier = strdup(name);
  return n;
}

static ASTNode *create_call(const char *callee_name, ASTNode *args[],
                            int count) {
  ASTNode *n = calloc(1, sizeof(ASTNode));
  n->type = AST_CALL;
  n->data.call.callee = create_ident(callee_name);
  n->data.call.arg_count = count;
  n->data.call.arguments = calloc(count, sizeof(ASTNode *));
  for (int i = 0; i < count; i++) {
    n->data.call.arguments[i] = args[i];
  }
  return n;
}

static NovaTensor *mock_tensor(int64_t *shape, int rank, NovaDType dtype) {
  NovaTensor *t = calloc(1, sizeof(NovaTensor));
  t->shape = malloc(rank * sizeof(int64_t));
  memcpy(t->shape, shape, rank * sizeof(int64_t));
  t->ndim = rank;
  t->rank = rank; // Alias
  t->dtype = dtype;

  t->total_elements = 1;
  for (int i = 0; i < rank; i++) {
    t->total_elements *= shape[i];
  }
  return t;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * MAIN TEST
 * ═══════════════════════════════════════════════════════════════════════════
 */

int main(void) {
  printf("\n════════════════════════════════════════════\n");
  printf("💰 NOVA GRAPH COST MODEL TEST\n");
  printf("════════════════════════════════════════════\n");

  /* 1. Create Mock AST: matmul(A, B) */
  // Note: 'matmul' is lowered. The cost model looks for 'MatMul' or generic
  // naming. The lowering pass converts AST call "matmul" to a graph node. Let's
  // assume it preserves the name or maps it.

  ASTNode *argA = create_ident("A");
  ASTNode *argB = create_ident("B");
  ASTNode *args[] = {argA, argB};
  ASTNode *root = create_call("matmul", args, 2);

  /* 2. Lowering (AST -> Graph) */
  printf("[1] Lowering AST to Graph...\n");
  NovaIRGraph *g = nova_lower_ast_to_graph(root);

  if (!g) {
    fprintf(stderr, "❌ Lowering failed.\n");
    return 1;
  }
  printf("✅ Graph created with %u nodes.\n", g->num_nodes);

  /* 3. Inject Shape Info */
  printf("[2] Injecting Shapes (A=[1024,1024], B=[1024,1024])...\n");

  // Find Op Node
  NovaIRNode *matmul = NULL;
  for (uint32_t i = 0; i < g->num_nodes; i++) {
    // Check op name. Lowering might lowercase it.
    if (g->nodes[i]->op && (strcasecmp(g->nodes[i]->op, "matmul") == 0)) {
      matmul = g->nodes[i];
      // Fix name to standard expected by Cost Model if needed (MatMul)
      // Actually my cost model checks "MatMul" or OP_MATMUL.
      // Let's explicitly set it to be safe.
      matmul->op_type = OP_MATMUL;
      matmul->op = "MatMul";
      break;
    }
  }

  if (matmul) {
    int64_t M = 1024, K = 1024, N = 1024;
    int64_t shapeA[] = {M, K};
    int64_t shapeB[] = {K, N};
    int64_t shapeOut[] = {M, N};

    NovaTensor *tA = mock_tensor(shapeA, 2, NOVA_DTYPE_FP32);
    NovaTensor *tB = mock_tensor(shapeB, 2, NOVA_DTYPE_FP32);
    NovaTensor *tOut = mock_tensor(shapeOut, 2, NOVA_DTYPE_FP32);

    // Attach to node inputs/outputs
    // We need to allocate the array if NULL
    if (!matmul->inputs)
      matmul->inputs = calloc(2, sizeof(NovaTensor *));
    matmul->inputs[0] = tA;
    matmul->inputs[1] = tB;
    matmul->num_inputs = 2;

    if (!matmul->outputs)
      matmul->outputs = calloc(1, sizeof(NovaTensor *));
    matmul->outputs[0] = tOut;
    matmul->num_outputs = 1;

    printf("    - Injected 1K x 1K MatMul (FP32)\n");
  } else {
    printf("⚠️ Could not find matmul node to inject shapes.\n");
  }

  /* 4. Run Cost Model */
  printf("\n[3] Running Cost Model Analysis...\n");

  NovaHardwareProfile hw_cpu;
  nova_cost_model_init_hw(&hw_cpu, NOVA_DEVICE_CPU);

  NovaCostMetrics cost_cpu = nova_cost_model_estimate_graph(g, &hw_cpu);
  nova_cost_model_print_report(&cost_cpu, "CPU ESTIMATE");

  /* Comparison with GPU */
  NovaHardwareProfile hw_gpu;
  nova_cost_model_init_hw(&hw_gpu, NOVA_DEVICE_METAL_GPU);

  NovaCostMetrics cost_gpu = nova_cost_model_estimate_graph(g, &hw_gpu);
  nova_cost_model_print_report(&cost_gpu, "GPU ESTIMATE");

  return 0;
}
