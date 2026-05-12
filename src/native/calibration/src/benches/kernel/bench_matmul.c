/**
 * Matrix Multiplication Kernel Benchmark
 * Measures GEMM performance across different sizes and implementations
 */

#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_SIZE 64
#define MAX_SIZE 4096

typedef struct {
    float *A;
    float *B;
    float *C;
    int M, N, K;
} MatmulContext;

extern void naive_matmul(float *A, float *B, float *C, int M, int N, int K);
extern void blocked_matmul(float *A, float *B, float *C, int M, int N, int K);
extern void simd_matmul(float *A, float *B, float *C, int M, int N, int K);

static void setup_matmul(void **ctx, int size) {
    MatmulContext *mc = (MatmulContext *)malloc(sizeof(MatmulContext));
    mc->M = mc->N = mc->K = size;
    mc->A = (float *)malloc(size * size * sizeof(float));
    mc->B = (float *)malloc(size * size * sizeof(float));
    mc->C = (float *)malloc(size * size * sizeof(float));
    
    for (int i = 0; i < size * size; i++) {
        mc->A[i] = (float)rand() / RAND_MAX;
        mc->B[i] = (float)rand() / RAND_MAX;
    }
    
    *ctx = mc;
}

static void run_naive_matmul(void *ctx) {
    MatmulContext *mc = (MatmulContext *)ctx;
    naive_matmul(mc->A, mc->B, mc->C, mc->M, mc->N, mc->K);
}

static void run_blocked_matmul(void *ctx) {
    MatmulContext *mc = (MatmulContext *)ctx;
    blocked_matmul(mc->A, mc->B, mc->C, mc->M, mc->N, mc->K);
}

static void run_simd_matmul(void *ctx) {
    MatmulContext *mc = (MatmulContext *)ctx;
    simd_matmul(mc->A, mc->B, mc->C, mc->M, mc->N, mc->K);
}

static void teardown_matmul(void *ctx) {
    MatmulContext *mc = (MatmulContext *)ctx;
    free(mc->A);
    free(mc->B);
    free(mc->C);
    free(mc);
}

int main(void) {
    printf("=== Matrix Multiplication Kernel Benchmark ===\n");
    
    AutocalContext *ctx = autocal_create();
    
    const char *variants[] = {"Naive", "Blocked", "SIMD"};
    void (*funcs[])(void*) = {run_naive_matmul, run_blocked_matmul, run_simd_matmul};
    
    for (int size = MIN_SIZE; size <= MAX_SIZE; size *= 2) {
        printf("\nSize %dx%d:\n", size, size);
        
        for (int v = 0; v < 3; v++) {
            void *bench_ctx;
            setup_matmul(&bench_ctx, size);
            
            double time_ms = autocal_measure_time(
                (AutocalWorkload){
                    .name = variants[v],
                    .execute = funcs[v],
                    .context = bench_ctx,
                    .iterations = 10
                }
            );
            
            double gflops = (2.0 * size * size * size) / (time_ms * 1e6);
            printf("  %8s: %.3f ms, %.2f GFLOPS\n", variants[v], time_ms, gflops);
            
            teardown_matmul(bench_ctx);
        }
    }
    
    autocal_destroy(ctx);
    return 0;
}
