#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <dispatch/dispatch.h>  // GCD for multi-threading on macOS

// Ultra Optimized Matmul for M1: +10 puan artış hedefi
// Previous: ~4.43 GFLOPS → Target: ~14-23 GFLOPS
// 1. Loop order: ikj
// 2. Blocking: Larger tiles
// 3. -O3 + vectorization
// 4. -march=native
// 5. Multi-thread with GCD

#define TILE_SIZE 64  // Increased from 32 for better blocking
#define NUM_THREADS 8  // M1 cores

typedef struct {
    float *data;
    int rows, cols;
} Matrix;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// GCD-based parallel matmul
void matmul_gcd_parallel(Matrix *A, Matrix *B, volatile Matrix *C) {
    int M = A->rows, K = A->cols, N = B->cols;

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    dispatch_group_t group = dispatch_group_create();

    int chunk_size = M / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; t++) {
        int start_i = t * chunk_size;
        int end_i = (t == NUM_THREADS - 1) ? M : (t + 1) * chunk_size;

        dispatch_group_async(group, queue, ^{
            for (int i = start_i; i < end_i; i++) {
                for (int k = 0; k < K; k++) {
                    float a_ik = A->data[i * K + k];
                    for (int j = 0; j < N; j++) {
                        C->data[i * N + j] += a_ik * B->data[k * N + j];
                    }
                }
            }
        });
    }

    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    dispatch_release(group);
}

// Ultra blocked matmul with larger tiles and prefetch
void matmul_ultra_blocked(Matrix *A, Matrix *B, volatile Matrix *C) {
    int M = A->rows, K = A->cols, N = B->cols;

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
    dispatch_group_t group = dispatch_group_create();

    int chunk_size = M / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; t++) {
        int start_i = t * chunk_size;
        int end_i = (t == NUM_THREADS - 1) ? M : (t + 1) * chunk_size;

        dispatch_group_async(group, queue, ^{
            for (int i = start_i; i < end_i; i += TILE_SIZE) {
                for (int j = 0; j < N; j += TILE_SIZE) {
                    for (int k = 0; k < K; k += TILE_SIZE) {
                        int i_max = (i + TILE_SIZE < end_i) ? i + TILE_SIZE : end_i;
                        int j_max = (j + TILE_SIZE < N) ? j + TILE_SIZE : N;
                        int k_max = (k + TILE_SIZE < K) ? k + TILE_SIZE : K;

                        for (int ii = i; ii < i_max; ii++) {
                            for (int kk = k; kk < k_max; kk++) {
                                float a_val = A->data[ii * K + kk];
                                // Prefetch for next
                                __builtin_prefetch(&B->data[kk * N + j + TILE_SIZE], 0, 3);
                                for (int jj = j; jj < j_max; jj += 4) {  // Unroll 4
                                    C->data[ii * N + jj] += a_val * B->data[kk * N + jj];
                                    C->data[ii * N + jj + 1] += a_val * B->data[kk * N + jj + 1];
                                    C->data[ii * N + jj + 2] += a_val * B->data[kk * N + jj + 2];
                                    C->data[ii * N + jj + 3] += a_val * B->data[kk * N + jj + 3];
                                }
                            }
                        }
                    }
                }
            }
        });
    }

    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    dispatch_release(group);
}

void benchmark_ultra_matmul(int size, int runs, const char *version) {
    Matrix A = {malloc(size * size * sizeof(float)), size, size};
    Matrix B = {malloc(size * size * sizeof(float)), size, size};
    Matrix C = {malloc(size * size * sizeof(float)), size, size};

    for (int i = 0; i < size * size; i++) {
        A.data[i] = 1.0f;
        B.data[i] = 1.0f;
        C.data[i] = 0.0f;
    }

    double total_time = 0.0;
    for (int r = 0; r < runs; r++) {
        for (int i = 0; i < size * size; i++) C.data[i] = 0.0f;

        double start = get_time();
        if (strcmp(version, "gcd") == 0) {
            matmul_gcd_parallel(&A, &B, (volatile Matrix*)&C);
        } else if (strcmp(version, "ultra") == 0) {
            matmul_ultra_blocked(&A, &B, (volatile Matrix*)&C);
        }
        double end = get_time();
        total_time += (end - start);
    }

    double avg_time = total_time / runs;
    double flops = 2.0 * size * size * size;
    double gflops = (flops / avg_time) / 1e9;

    int errors = 0;
    for (int i = 0; i < size * size; i++) {
        if (fabs(C.data[i] - size) > 1e-3) errors++;
    }

    printf("%s Matmul %dx%d (%d runs): Avg %.6f s, %.2f GFLOPS, Errors: %d/%d\n",
           version, size, size, runs, avg_time, gflops, errors, size*size);

    free(A.data); free(B.data); free(C.data);
}

int main() {
    printf("Ultra Optimized Matmul: +10 puan artış hedefi\n");
    printf("Previous: ~4.43 GFLOPS → Target: 14-23 GFLOPS\n");
    printf("Compiler: -O3 -march=native\n");
    printf("Threads: %d (GCD)\n\n", NUM_THREADS);

    int sizes[] = {128, 256, 512};
    for (int s = 0; s < 3; s++) {
        benchmark_ultra_matmul(sizes[s], 3, "gcd");
        benchmark_ultra_matmul(sizes[s], 3, "ultra");
    }

    printf("\nOptimizations Enhanced:\n");
    printf("1. Loop order: ikj\n");
    printf("2. Blocking: TILE_SIZE=64, prefetch\n");
    printf("3. -O3 + vectorization + unroll\n");
    printf("4. -march=native (M1)\n");
    printf("5. Multi-thread: GCD parallel\n");
    printf("\nTarget: +10 puan (14-23 GFLOPS)\n");

    return 0;
}
