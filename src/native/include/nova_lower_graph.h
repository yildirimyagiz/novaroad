/**
 * Nova AST to IR Graph Lowering
 * Converts High-Level AST to Verification Graph (NovaIRGraph)
 */

#ifndef NOVA_LOWER_GRAPH_H
#define NOVA_LOWER_GRAPH_H

#include "compiler/nova_ast.h"
#include "nova_graph_structs.h"

/**
 * Converts an AST (Program) into a Compute Graph for verification.
 * Functions become connected components within the graph.
 */
NovaIRGraph *nova_lower_ast_to_graph(ASTNode *program);

/**
 * Frees the generated graph.
 */
void nova_ir_graph_free(NovaIRGraph *graph);

#endif // NOVA_LOWER_GRAPH_H
