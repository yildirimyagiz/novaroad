/**
 * Graph Operations Benchmark for Auto-Calibration
 * Tests computation graph execution and optimization
 */

#include "nova_autocal.h"
#include "nova_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef enum {
    OP_ADD, OP_MUL, OP_RELU, OP_CONV, OP_POOL
} OpType;

typedef struct GraphNode {
    OpType op;
    size_t *input_indices;
    size_t input_count;
    size_t output_size;
    float *data;
} GraphNode;

typedef struct {
    GraphNode *nodes;
    size_t node_count;
    double time_ms;
    double ops_per_sec;
} GraphBenchResult;

static void execute_op(GraphNode *node, float **inputs) {
    switch (node->op) {
        case OP_ADD:
            for (size_t i = 0; i < node->output_size; i++) {
                node->data[i] = inputs[0][i] + inputs[1][i];
            }
            break;
        case OP_MUL:
            for (size_t i = 0; i < node->output_size; i++) {
                node->data[i] = inputs[0][i] * inputs[1][i];
            }
            break;
        case OP_RELU:
            for (size_t i = 0; i < node->output_size; i++) {
                node->data[i] = fmaxf(0.0f, inputs[0][i]);
            }
            break;
        default:
            break;
    }
}

static GraphBenchResult bench_graph_execution(size_t graph_size, size_t tensor_size, size_t iterations) {
    GraphBenchResult result = {0};
    
    // Build simple computation graph
    GraphNode *nodes = (GraphNode*)calloc(graph_size, sizeof(GraphNode));
    if (!nodes) return result;
    
    // Initialize nodes
    for (size_t i = 0; i < graph_size; i++) {
        nodes[i].op = (OpType)(i % 5);
        nodes[i].output_size = tensor_size;
        nodes[i].data = (float*)malloc(tensor_size * sizeof(float));
        nodes[i].input_count = (i == 0) ? 0 : ((i % 3 == 0) ? 2 : 1);
        
        if (nodes[i].input_count > 0) {
            nodes[i].input_indices = (size_t*)malloc(nodes[i].input_count * sizeof(size_t));
            for (size_t j = 0; j < nodes[i].input_count; j++) {
                nodes[i].input_indices[j] = (i > j) ? (i - j - 1) : 0;
            }
        }
        
        // Initialize data
        for (size_t j = 0; j < tensor_size; j++) {
            nodes[i].data[j] = (float)rand() / RAND_MAX;
        }
    }
    
    // Benchmark execution
    nova_timer_t *timer = nova_timer_create();
    nova_timer_start(timer);
    
    for (size_t iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < graph_size; i++) {
            if (nodes[i].input_count == 0) continue;
            
            float **inputs = (float**)malloc(nodes[i].input_count * sizeof(float*));
            for (size_t j = 0; j < nodes[i].input_count; j++) {
                inputs[j] = nodes[nodes[i].input_indices[j]].data;
            }
            
            execute_op(&nodes[i], inputs);
            free(inputs);
        }
    }
    
    nova_timer_stop(timer);
    result.time_ms = nova_timer_elapsed_ms(timer) / iterations;
    result.ops_per_sec = (graph_size * 1000.0) / result.time_ms;
    result.nodes = nodes;
    result.node_count = graph_size;
    
    nova_timer_destroy(timer);
    
    return result;
}

void nova_autocal_bench_graph_ops(nova_autocal_context_t *ctx) {
    printf("=== Graph Operations Auto-Calibration ===\n\n");
    
    size_t graph_sizes[] = {10, 50, 100, 500};
    size_t tensor_size = 1024;
    
    for (size_t i = 0; i < 4; i++) {
        size_t size = graph_sizes[i];
        
        printf("Graph size: %zu nodes, tensor size: %zu\n", size, tensor_size);
        
        GraphBenchResult res = bench_graph_execution(size, tensor_size, 100);
        
        printf("  Time: %.3f ms\n", res.time_ms);
        printf("  Ops/sec: %.2f\n\n", res.ops_per_sec);
        
        char key[256];
        snprintf(key, sizeof(key), "graph_%zu_nodes", size);
        nova_autocal_record_metric(ctx, key, "time_ms", res.time_ms);
        nova_autocal_record_metric(ctx, key, "ops_per_sec", res.ops_per_sec);
        
        // Cleanup
        for (size_t j = 0; j < res.node_count; j++) {
            free(res.nodes[j].data);
            free(res.nodes[j].input_indices);
        }
        free(res.nodes);
    }
    
    printf("Graph ops calibration complete.\n");
}
