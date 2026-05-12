/**
 * Flash Attention Benchmark
 * Measures performance of Flash Attention v2 implementation
 */

#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SEQ_LEN_MIN 128
#define SEQ_LEN_MAX 8192
#define HEAD_DIM 64
#define NUM_HEADS 8

extern void flash_attention_v2(float *Q, float *K, float *V, float *out,
                                int seq_len, int head_dim);

static void setup_flash_attention(void **ctx, int seq_len) {
    size_t qkv_size = seq_len * HEAD_DIM * sizeof(float);
    
    float *Q = (float *)malloc(qkv_size);
    float *K = (float *)malloc(qkv_size);
    float *V = (float *)malloc(qkv_size);
    float *out = (float *)malloc(qkv_size);
    
    // Initialize with random values
    for (int i = 0; i < seq_len * HEAD_DIM; i++) {
        Q[i] = (float)rand() / RAND_MAX;
        K[i] = (float)rand() / RAND_MAX;
        V[i] = (float)rand() / RAND_MAX;
    }
    
    void **data = (void **)malloc(4 * sizeof(void*));
    data[0] = Q;
    data[1] = K;
    data[2] = V;
    data[3] = out;
    *ctx = data;
}

static void run_flash_attention(void *ctx, int seq_len) {
    void **data = (void **)ctx;
    flash_attention_v2(data[0], data[1], data[2], data[3], seq_len, HEAD_DIM);
}

static void teardown_flash_attention(void *ctx) {
    void **data = (void **)ctx;
    free(data[0]);
    free(data[1]);
    free(data[2]);
    free(data[3]);
    free(data);
}

int main(void) {
    printf("=== Flash Attention v2 Benchmark ===\n");
    
    AutocalContext *ctx = autocal_create();
    
    // Benchmark different sequence lengths
    for (int seq_len = SEQ_LEN_MIN; seq_len <= SEQ_LEN_MAX; seq_len *= 2) {
        void *bench_ctx;
        setup_flash_attention(&bench_ctx, seq_len);
        
        double time_ms = autocal_measure_time(
            (AutocalWorkload){
                .name = "flash_attention_v2",
                .setup = NULL,
                .execute = (void (*)(void*))run_flash_attention,
                .teardown = NULL,
                .context = bench_ctx,
                .iterations = 100
            }
        );
        
        double throughput = (seq_len * seq_len * HEAD_DIM * 2.0) / (time_ms * 1e6);
        
        printf("SeqLen=%4d: %.3f ms, %.2f GFLOPS\n", 
               seq_len, time_ms, throughput);
        
        teardown_flash_attention(bench_ctx);
    }
    
    autocal_destroy(ctx);
    return 0;
}
