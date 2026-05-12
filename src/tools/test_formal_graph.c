/**
 * test_formal_graph.c - Standalone test for Nova AST -> Graph -> Verify
 * Pipeline
 */

#include "compiler/nova_ast.h"
#include "nova_graph_obligations.h"
#include "nova_kernel_contracts.h"
#include "compiler/nova_lower_graph.h"
#include "nova_obligation.h" /* For ObligationKind/Set types */
#include "ml/nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * STUBS FOR LINKING
 * ═══════════════════════════════════════════════════════════════════════════
 */

void *nova_malloc(size_t size) { return malloc(size); }
void nova_free(void *ptr) { free(ptr); }
void *nova_malloc_aligned(size_t size, size_t align) {
  (void)align;
  return malloc(size);
}

/* V1 Obligation Stubs (Not used directly by V2 test, but linked via graph
 * verify) */

ObligationSet *obligation_set_create(const char *name) {
  (void)name;
  return NULL;
}

Obligation *obligation_set_add(ObligationSet *set, ObligationKind type,
                               const char *desc, const char *smt) {
  (void)set;
  (void)type;
  (void)desc;
  (void)smt;
  return NULL;
}

uint64_t get_device_memory_limit(void) { return 1024ULL * 1024 * 1024; }

/* Tensor Ops Stubs (Not used directly by graph verifier) */
void nova_op_add(void *a, void *b, void *c) {
  (void)a;
  (void)b;
  (void)c;
}
void nova_op_matmul(void *a, void *b, void *c) {
  (void)a;
  (void)b;
  (void)c;
}
void nova_op_mul(void *a, void *b, void *c) {
  (void)a;
  (void)b;
  (void)c;
}
void nova_op_relu(void *a, void *b) {
  (void)a;
  (void)b;
}
void nova_op_softmax(void *a, void *b, int dim) {
  (void)a;
  (void)b;
  (void)dim;
}

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

static NovaTensor *mock_tensor(int64_t *shape, int rank) {
  NovaTensor *t = calloc(1, sizeof(NovaTensor));
  t->shape = malloc(rank * sizeof(int64_t));
  memcpy(t->shape, shape, rank * sizeof(int64_t));
  t->ndim = rank;
  return t;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * MAIN TEST
 * ═══════════════════════════════════════════════════════════════════════════
 */

int main(void) {
  printf("\n════════════════════════════════════════════\n");
  printf("🔍 NOVA FORMAL PIPELINE TEST (Standalone)\n");
  printf("════════════════════════════════════════════\n");

  /* 1. Create Mock AST: matmul(A, B) */
  printf("\n[1] Constructing Mock AST: matmul(A, B)...\n");
  ASTNode *argA = create_ident("A");
  ASTNode *argB = create_ident("B");
  ASTNode *args[] = {argA, argB};
  ASTNode *root = create_call("matmul", args, 2);

  /* 2. Lowering (AST -> Graph) */
  printf("[2] Lowering AST to Graph...\n");
  NovaIRGraph *g = nova_lower_ast_to_graph(root);

  if (!g) {
    fprintf(stderr, "❌ Lowering failed: Graph is NULL\n");
    return 1;
  }
  printf("✅ Graph created successfully with %u nodes.\n", g->num_nodes);

  /* Inspect Nodes */
  for (uint32_t i = 0; i < g->num_nodes; i++) {
    NovaIRNode *n = g->nodes[i];
    printf("    Node %llu: %-8s (Inputs: %d)\n", (unsigned long long)n->node_id,
           n->op ? n->op : "?", n->num_input_edges);
  }

  /* 3. Manual Shape Injection */
  printf("\n[3] Injecting Shape Info (Simulating Semantic Pass via Edge "
         "Traversal)...\n");

  /* Find MatMul node */
  NovaIRNode *matmul = NULL;
  for (uint32_t i = 0; i < g->num_nodes; i++) {
    if (g->nodes[i]->op && strcmp(g->nodes[i]->op, "matmul") == 0) {
      matmul = g->nodes[i];
      break;
    }
  }

  if (matmul && matmul->num_input_edges >= 2) {
    int64_t shapeA[] = {128, 64};
    int64_t shapeB[] = {64, 32};
    int64_t shapeOut[] = {128, 32};

    NovaTensor *tA = mock_tensor(shapeA, 2);
    NovaTensor *tB = mock_tensor(shapeB, 2);
    NovaTensor *tOut = mock_tensor(shapeOut, 2);

    /* Get inputs from edges */
    NovaIRNode *srcA = matmul->input_edges[0]->source_node;
    NovaIRNode *srcB = matmul->input_edges[1]->source_node;

    /* Setup Source A output */
    if (!srcA->outputs) {
      srcA->outputs = calloc(1, sizeof(NovaTensor *));
      srcA->num_outputs = 1;
    }
    srcA->outputs[0] = tA;

    /* Setup Source B output */
    if (!srcB->outputs) {
      srcB->outputs = calloc(1, sizeof(NovaTensor *));
      srcB->num_outputs = 1;
    }
    srcB->outputs[0] = tB;

    /* Setup MatMul inputs (Propagate from sources) */
    if (!matmul->inputs) {
      matmul->inputs = calloc(2, sizeof(NovaTensor *));
      matmul->num_inputs = 2;
    }
    /* IMPORTANT: Matmul contract expects inputs[0]=A, inputs[1]=B */
    matmul->inputs[0] = tA;
    matmul->inputs[1] = tB;

    /* Setup MatMul output */
    if (!matmul->outputs) {
      matmul->outputs = calloc(1, sizeof(NovaTensor *));
      matmul->num_outputs = 1;
    }
    matmul->outputs[0] = tOut;

    printf("    - Injected shapes: A[128,64] x B[64,32] -> Out[128,32]\n");
  } else {
    printf(
        "⚠️ Warning: MatMul node structure unexpected, skipping injection.\n");
  }

  /* 4. Verification (V2) */
  printf("\n[4] Running Formal Verification (V2 Contracts)...\n");
  NovaObligationList obligations = nova_obligation_list_create();
  nova_graph_collect_obligations(g, &obligations);

  nova_obligation_list_print(&obligations);

  if (nova_obligation_list_all_satisfied(&obligations)) {
    printf("\n✅ RESULT: VERIFICATION PASSED!\n");
  } else {
    printf("\n⛔ RESULT: VERIFICATION FAILED.\n");
  }

  /* Cleanup omitted for test brevity */
  return 0;
}
