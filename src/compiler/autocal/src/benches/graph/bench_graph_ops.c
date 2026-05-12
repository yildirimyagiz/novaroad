/**
 * @file bench_graph_ops.c
 * @brief Graph operation benchmarks for autocal
 */

#include "../../../include/zenith_autocal.h"
#include "../../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GRAPH_BENCH_ITERS 100

// Simple graph convolution benchmark
static void graph_conv_cpu(const float *features, const float *adj_matrix,
                           float *output, int num_nodes, int feature_dim)
{
    // Simplified GCN: Output = AdjMatrix @ Features
    for (int i = 0; i < num_nodes; i++) {
        for (int f = 0; f < feature_dim; f++) {
            float sum = 0.0f;
            for (int j = 0; j < num_nodes; j++) {
                sum += adj_matrix[i * num_nodes + j] * features[j * feature_dim + f];
            }
            output[i * feature_dim + f] = sum;
        }
    }
}

double bench_graph_conv_latency(int num_nodes, int feature_dim)
{
    size_t feat_size = num_nodes * feature_dim;
    size_t adj_size = num_nodes * num_nodes;
    
    float *features = malloc(feat_size * sizeof(float));
    float *adj_matrix = malloc(adj_size * sizeof(float));
    float *output = malloc(feat_size * sizeof(float));
    
    if (!features || !adj_matrix || !output) {
        free(features); free(adj_matrix); free(output);
        return -1.0;
    }
    
    // Initialize sparse adjacency (10% density)
    for (size_t i = 0; i < adj_size; i++) {
        adj_matrix[i] = (rand() % 10 == 0) ? 1.0f : 0.0f;
    }
    for (size_t i = 0; i < feat_size; i++) {
        features[i] = (float)rand() / RAND_MAX;
    }
    
    // Benchmark
    double start = zenith_timer_get_sec();
    for (int i = 0; i < GRAPH_BENCH_ITERS; i++) {
        graph_conv_cpu(features, adj_matrix, output, num_nodes, feature_dim);
    }
    double end = zenith_timer_get_sec();
    
    free(features); free(adj_matrix); free(output);
    
    return (end - start) / GRAPH_BENCH_ITERS;
}

void autocal_benchmark_graph_ops(NovaAutocalConfig *config)
{
    printf("📊 [GRAPH] Benchmarking Graph Operations...\n");
    
    struct {
        int nodes;
        int features;
        const char *name;
    } configs[] = {
        {100, 32, "Small Graph"},
        {500, 64, "Medium Graph"},
        {1000, 128, "Large Graph"}
    };
    
    double total_time = 0.0;
    for (int i = 0; i < 3; i++) {
        double latency = bench_graph_conv_latency(configs[i].nodes, configs[i].features);
        printf("   %s (%dx%d): %.3f ms\n", configs[i].name, 
               configs[i].nodes, configs[i].features, latency * 1000);
        total_time += latency;
    }
    
    printf("✅ [GRAPH] Average latency: %.3f ms\n", (total_time / 3.0) * 1000);
}
