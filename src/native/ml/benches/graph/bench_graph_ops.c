/**
 * Graph Operations Benchmark
 * Measures computational graph execution performance
 */

#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OP_ADD, OP_MUL, OP_MATMUL, OP_RELU, OP_SOFTMAX
} OpType;

typedef struct GraphNode {
    OpType op;
    struct GraphNode **inputs;
    int num_inputs;
    float *data;
    int size;
} GraphNode;

typedef struct {
    GraphNode **nodes;
    int num_nodes;
} ComputeGraph;

extern void execute_graph(ComputeGraph *graph);

static ComputeGraph *create_test_graph(int depth, int width) {
    ComputeGraph *graph = (ComputeGraph *)malloc(sizeof(ComputeGraph));
    graph->num_nodes = depth * width;
    graph->nodes = (GraphNode **)malloc(graph->num_nodes * sizeof(GraphNode*));
    
    int idx = 0;
    for (int d = 0; d < depth; d++) {
        for (int w = 0; w < width; w++) {
            GraphNode *node = (GraphNode *)malloc(sizeof(GraphNode));
            node->op = (OpType)(rand() % 5);
            node->size = 1024;
            node->data = (float *)malloc(1024 * sizeof(float));
            node->num_inputs = (d == 0) ? 0 : 2;
            
            if (node->num_inputs > 0) {
                node->inputs = (GraphNode **)malloc(2 * sizeof(GraphNode*));
                node->inputs[0] = graph->nodes[idx - width];
                node->inputs[1] = graph->nodes[idx - width + (w > 0 ? -1 : 0)];
            }
            
            graph->nodes[idx++] = node;
        }
    }
    
    return graph;
}

static void destroy_graph(ComputeGraph *graph) {
    for (int i = 0; i < graph->num_nodes; i++) {
        free(graph->nodes[i]->data);
        if (graph->nodes[i]->inputs) free(graph->nodes[i]->inputs);
        free(graph->nodes[i]);
    }
    free(graph->nodes);
    free(graph);
}

static void run_graph_execution(void *ctx) {
    execute_graph((ComputeGraph *)ctx);
}

int main(void) {
    printf("=== Computational Graph Benchmark ===\n");
    
    AutocalContext *ctx = autocal_create();
    
    int configs[][2] = {{10, 5}, {20, 10}, {50, 20}};
    
    for (int i = 0; i < 3; i++) {
        int depth = configs[i][0];
        int width = configs[i][1];
        
        ComputeGraph *graph = create_test_graph(depth, width);
        
        double time_ms = autocal_measure_time(
            (AutocalWorkload){
                .name = "graph_execution",
                .execute = run_graph_execution,
                .context = graph,
                .iterations = 100
            }
        );
        
        printf("Depth=%2d, Width=%2d: %.3f ms, %d ops\n",
               depth, width, time_ms, graph->num_nodes);
        
        destroy_graph(graph);
    }
    
    autocal_destroy(ctx);
    return 0;
}
