#ifndef NOVA_PYTHON_BINDING_H
#define NOVA_PYTHON_BINDING_H

#include "compute/graph/nova_graph.h"
#include "../runtime/nova_context.h"
#include "../ml/nova_tensor.h"

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA PYTHON BINDING API (C-Bridge)
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Context
NovaContext *py_nova_init(bool strict_determinism);
void py_nova_shutdown(NovaContext *ctx);

// Tensor
NovaTensor *py_nova_tensor_create(NovaContext *ctx, int64_t *shape,
                                      int ndim, int dtype);
void py_nova_tensor_destroy(NovaTensor *t);
float *py_nova_tensor_data(NovaTensor *t);

// Graph
NovaExecGraph *py_nova_graph_create(NovaContext *ctx);
uint64_t py_nova_graph_add_input(NovaExecGraph *graph, NovaTensor *t,
                                   const char *name);
uint64_t py_nova_graph_add_matmul(NovaExecGraph *graph, uint64_t a_id,
                                    uint64_t b_id, const char *name);
uint64_t py_nova_graph_add_add(NovaExecGraph *graph, uint64_t a_id,
                                 uint64_t b_id, const char *name);
int py_nova_graph_execute(NovaExecGraph *graph);

#endif // NOVA_PYTHON_BINDING_H
