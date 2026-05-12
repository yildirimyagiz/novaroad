/**
 * LLM Inference Benchmark
 * Measures token generation throughput
 */

#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>

#define VOCAB_SIZE 32000
#define HIDDEN_DIM 4096
#define NUM_LAYERS 32
#define SEQ_LEN 2048

typedef struct {
    float *embeddings;
    float *hidden_states;
    int batch_size;
    int seq_len;
} LLMContext;

extern void llm_forward_pass(float *embeddings, float *hidden_states, 
                              int batch_size, int seq_len, int num_layers);

static void setup_llm(void **ctx, int batch_size) {
    LLMContext *lc = (LLMContext *)malloc(sizeof(LLMContext));
    lc->batch_size = batch_size;
    lc->seq_len = SEQ_LEN;
    
    lc->embeddings = (float *)malloc(VOCAB_SIZE * HIDDEN_DIM * sizeof(float));
    lc->hidden_states = (float *)malloc(batch_size * SEQ_LEN * HIDDEN_DIM * sizeof(float));
    
    for (int i = 0; i < VOCAB_SIZE * HIDDEN_DIM; i++) {
        lc->embeddings[i] = (float)rand() / RAND_MAX;
    }
    
    *ctx = lc;
}

static void run_llm_inference(void *ctx) {
    LLMContext *lc = (LLMContext *)ctx;
    llm_forward_pass(lc->embeddings, lc->hidden_states, 
                     lc->batch_size, lc->seq_len, NUM_LAYERS);
}

static void teardown_llm(void *ctx) {
    LLMContext *lc = (LLMContext *)ctx;
    free(lc->embeddings);
    free(lc->hidden_states);
    free(lc);
}

int main(void) {
    printf("=== LLM Inference Benchmark ===\n");
    printf("Model: %d layers, %d hidden dim, %d vocab\n", 
           NUM_LAYERS, HIDDEN_DIM, VOCAB_SIZE);
    
    AutocalContext *ctx = autocal_create();
    
    int batch_sizes[] = {1, 2, 4, 8, 16};
    
    for (int i = 0; i < 5; i++) {
        int bs = batch_sizes[i];
        
        void *bench_ctx;
        setup_llm(&bench_ctx, bs);
        
        double time_ms = autocal_measure_time(
            (AutocalWorkload){
                .name = "llm_inference",
                .execute = run_llm_inference,
                .context = bench_ctx,
                .iterations = 10
            }
        );
        
        double tokens_per_sec = (bs * SEQ_LEN * 1000.0) / time_ms;
        
        printf("Batch=%2d: %.2f ms/forward, %.0f tokens/sec\n",
               bs, time_ms, tokens_per_sec);
        
        teardown_llm(bench_ctx);
    }
    
    autocal_destroy(ctx);
    return 0;
}
