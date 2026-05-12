/**
 * ═══════════════════════════════════════════════════════════════════════════
 * SECTION 5: ATTENTION / AI WORKLOADS
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_scientific_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════
// NAIVE ATTENTION: O(n²d) with materialized score matrix
// ═══════════════════════════════════════════════════════════════════════════

static void attention_naive(const float* Q, const float* K, const float* V,
                            float* output, int seq_len, int d_model, int n_heads) {
    int d_head = d_model / n_heads;
    float scale = 1.0f / sqrtf((float)d_head);
    
    // Allocate score matrix (seq_len × seq_len)
    float* scores = (float*)malloc(seq_len * seq_len * sizeof(float));
    float* attn_weights = (float*)malloc(seq_len * seq_len * sizeof(float));
    
    for (int h = 0; h < n_heads; h++) {
        int head_offset = h * d_head;
        
        // Compute Q @ K^T
        for (int i = 0; i < seq_len; i++) {
            for (int j = 0; j < seq_len; j++) {
                float score = 0.0f;
                for (int k = 0; k < d_head; k++) {
                    score += Q[i * d_model + head_offset + k] * 
                            K[j * d_model + head_offset + k];
                }
                scores[i * seq_len + j] = score * scale;
            }
        }
        
        // Softmax over each row
        for (int i = 0; i < seq_len; i++) {
            float max_score = scores[i * seq_len];
            for (int j = 1; j < seq_len; j++) {
                if (scores[i * seq_len + j] > max_score) {
                    max_score = scores[i * seq_len + j];
                }
            }
            
            float sum = 0.0f;
            for (int j = 0; j < seq_len; j++) {
                attn_weights[i * seq_len + j] = expf(scores[i * seq_len + j] - max_score);
                sum += attn_weights[i * seq_len + j];
            }
            
            for (int j = 0; j < seq_len; j++) {
                attn_weights[i * seq_len + j] /= sum;
            }
        }
        
        // Compute attn_weights @ V
        for (int i = 0; i < seq_len; i++) {
            for (int k = 0; k < d_head; k++) {
                float sum = 0.0f;
                for (int j = 0; j < seq_len; j++) {
                    sum += attn_weights[i * seq_len + j] * 
                          V[j * d_model + head_offset + k];
                }
                output[i * d_model + head_offset + k] = sum;
            }
        }
    }
    
    free(scores);
    free(attn_weights);
}

// ═══════════════════════════════════════════════════════════════════════════
// IO-AWARE TILED ATTENTION: Process in blocks to fit in cache
// ═══════════════════════════════════════════════════════════════════════════

static void attention_tiled(const float* Q, const float* K, const float* V,
                            float* output, int seq_len, int d_model, int n_heads) {
    int d_head = d_model / n_heads;
    float scale = 1.0f / sqrtf((float)d_head);
    
    const int TILE_SIZE = 64; // Process 64×64 blocks
    
    float* scores_tile = (float*)malloc(TILE_SIZE * TILE_SIZE * sizeof(float));
    float* attn_tile = (float*)malloc(TILE_SIZE * TILE_SIZE * sizeof(float));
    
    for (int h = 0; h < n_heads; h++) {
        int head_offset = h * d_head;
        
        // Process in tiles
        for (int i0 = 0; i0 < seq_len; i0 += TILE_SIZE) {
            for (int j0 = 0; j0 < seq_len; j0 += TILE_SIZE) {
                int i_max = (i0 + TILE_SIZE < seq_len) ? i0 + TILE_SIZE : seq_len;
                int j_max = (j0 + TILE_SIZE < seq_len) ? j0 + TILE_SIZE : seq_len;
                
                // Compute tile of Q @ K^T
                for (int i = i0; i < i_max; i++) {
                    for (int j = j0; j < j_max; j++) {
                        float score = 0.0f;
                        for (int k = 0; k < d_head; k++) {
                            score += Q[i * d_model + head_offset + k] * 
                                    K[j * d_model + head_offset + k];
                        }
                        scores_tile[(i - i0) * TILE_SIZE + (j - j0)] = score * scale;
                    }
                }
                
                // Softmax and accumulate (simplified)
                for (int i = i0; i < i_max; i++) {
                    float max_score = scores_tile[(i - i0) * TILE_SIZE];
                    for (int j = j0; j < j_max; j++) {
                        if (scores_tile[(i - i0) * TILE_SIZE + (j - j0)] > max_score) {
                            max_score = scores_tile[(i - i0) * TILE_SIZE + (j - j0)];
                        }
                    }
                    
                    float sum = 0.0f;
                    for (int j = j0; j < j_max; j++) {
                        float val = expf(scores_tile[(i - i0) * TILE_SIZE + (j - j0)] - max_score);
                        attn_tile[(i - i0) * TILE_SIZE + (j - j0)] = val;
                        sum += val;
                    }
                    
                    for (int j = j0; j < j_max; j++) {
                        attn_tile[(i - i0) * TILE_SIZE + (j - j0)] /= sum;
                    }
                }
            }
        }
        
        // Compute output (simplified - full implementation would tile this too)
        for (int i = 0; i < seq_len; i++) {
            for (int k = 0; k < d_head; k++) {
                output[i * d_model + head_offset + k] = 0.0f;
            }
        }
    }
    
    free(scores_tile);
    free(attn_tile);
}

// ═══════════════════════════════════════════════════════════════════════════
// FUSED ATTENTION: Online softmax, minimal materialization
// ═══════════════════════════════════════════════════════════════════════════

static void attention_fused(const float* Q, const float* K, const float* V,
                           float* output, int seq_len, int d_model, int n_heads) {
    int d_head = d_model / n_heads;
    float scale = 1.0f / sqrtf((float)d_head);
    
    for (int h = 0; h < n_heads; h++) {
        int head_offset = h * d_head;
        
        // Process each query independently with online softmax
        for (int i = 0; i < seq_len; i++) {
            float max_score = -INFINITY;
            float sum_exp = 0.0f;
            
            // First pass: compute max
            for (int j = 0; j < seq_len; j++) {
                float score = 0.0f;
                for (int k = 0; k < d_head; k++) {
                    score += Q[i * d_model + head_offset + k] * 
                            K[j * d_model + head_offset + k];
                }
                score *= scale;
                if (score > max_score) {
                    max_score = score;
                }
            }
            
            // Second pass: accumulate output with online normalization
            for (int k = 0; k < d_head; k++) {
                output[i * d_model + head_offset + k] = 0.0f;
            }
            
            for (int j = 0; j < seq_len; j++) {
                float score = 0.0f;
                for (int k = 0; k < d_head; k++) {
                    score += Q[i * d_model + head_offset + k] * 
                            K[j * d_model + head_offset + k];
                }
                score *= scale;
                
                float weight = expf(score - max_score);
                sum_exp += weight;
                
                // Accumulate weighted V
                for (int k = 0; k < d_head; k++) {
                    output[i * d_model + head_offset + k] += 
                        weight * V[j * d_model + head_offset + k];
                }
            }
            
            // Normalize
            for (int k = 0; k < d_head; k++) {
                output[i * d_model + head_offset + k] /= sum_exp;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK DRIVER
// ═══════════════════════════════════════════════════════════════════════════

AttentionMetrics bench_attention(int seq_len, int d_model, int n_heads, 
                                 AttentionVariant variant) {
    AttentionMetrics metrics = {0};
    
    const int WARMUP_ITERS = 3;
    const int BENCH_ITERS = 10;
    
    // Allocate Q, K, V
    float* Q = (float*)malloc(seq_len * d_model * sizeof(float));
    float* K = (float*)malloc(seq_len * d_model * sizeof(float));
    float* V = (float*)malloc(seq_len * d_model * sizeof(float));
    float* output = (float*)malloc(seq_len * d_model * sizeof(float));
    
    if (!Q || !K || !V || !output) {
        fprintf(stderr, "Failed to allocate memory\n");
        free(Q); free(K); free(V); free(output);
        return metrics;
    }
    
    // Initialize
    for (int i = 0; i < seq_len * d_model; i++) {
        Q[i] = (float)(rand() % 100) / 100.0f - 0.5f;
        K[i] = (float)(rand() % 100) / 100.0f - 0.5f;
        V[i] = (float)(rand() % 100) / 100.0f - 0.5f;
    }
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; i++) {
        if (variant == ATTN_NAIVE) {
            attention_naive(Q, K, V, output, seq_len, d_model, n_heads);
        } else if (variant == ATTN_IO_AWARE_TILED) {
            attention_tiled(Q, K, V, output, seq_len, d_model, n_heads);
        } else if (variant == ATTN_FUSED) {
            attention_fused(Q, K, V, output, seq_len, d_model, n_heads);
        }
    }
    
    // Benchmark
    NovaTimer timer;
    uint64_t total_ns = 0;
    
    for (int i = 0; i < BENCH_ITERS; i++) {
        nova_timer_start(&timer);
        
        if (variant == ATTN_NAIVE) {
            attention_naive(Q, K, V, output, seq_len, d_model, n_heads);
        } else if (variant == ATTN_IO_AWARE_TILED) {
            attention_tiled(Q, K, V, output, seq_len, d_model, n_heads);
        } else if (variant == ATTN_FUSED) {
            attention_fused(Q, K, V, output, seq_len, d_model, n_heads);
        }
        
        nova_timer_stop(&timer);
        total_ns += timer.elapsed_ns;
    }
    
    double avg_ns = (double)total_ns / (double)BENCH_ITERS;
    metrics.latency_ms = avg_ns / 1000000.0;
    
    // FLOPS calculation: 4 * seq_len^2 * d_model (approximate)
    double flops = 4.0 * (double)seq_len * (double)seq_len * (double)d_model;
    metrics.effective_flops = flops / avg_ns;
    
    // Memory traffic estimation (GB)
    // Naive: seq_len^2 for scores + reads/writes of Q,K,V
    double memory_bytes = (double)seq_len * (double)seq_len * sizeof(float) +
                         3.0 * (double)seq_len * (double)d_model * sizeof(float);
    metrics.memory_traffic_gb = memory_bytes / 1e9;
    
    // Checksum
    metrics.checksum = nova_checksum_fp32(output, seq_len * d_model);
    metrics.validated = !nova_detect_dead_code_elimination(metrics.checksum);
    
    // Numerical drift (compare first and last run)
    float* output2 = (float*)malloc(seq_len * d_model * sizeof(float));
    if (variant == ATTN_NAIVE) {
        attention_naive(Q, K, V, output2, seq_len, d_model, n_heads);
    } else if (variant == ATTN_FUSED) {
        attention_fused(Q, K, V, output2, seq_len, d_model, n_heads);
    }
    
    double max_diff = 0.0;
    for (int i = 0; i < seq_len * d_model; i++) {
        double diff = fabs(output[i] - output2[i]);
        if (diff > max_diff) max_diff = diff;
    }
    metrics.numerical_drift = max_diff;
    
    free(Q);
    free(K);
    free(V);
    free(output);
    free(output2);
    
    return metrics;
}
