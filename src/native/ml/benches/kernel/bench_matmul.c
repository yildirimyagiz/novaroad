/**
 * Matrix Multiplication Kernel Benchmark
 * ══════════════════════════════════════════════════════════════════
 * Measures GEMM performance across different sizes and implementations.
 * Self-contained: no external autocal dependency.
 *
 * Usage:
 *   clang -O3 -march=native -ffast-math -I../../nova_gemm/include \
 *         bench_matmul.c -L../../nova_gemm -lnova_gemm -lm -o bench_matmul
 *   ./bench_matmul
 * ══════════════════════════════════════════════════════════════════
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MIN_SIZE 64
#define MAX_SIZE 4096

/* ── Timing ───────────────────────────────────────────────────────── */
static inline double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ── Matrix helpers ───────────────────────────────────────────────── */
typedef struct {
    float *A;
    float *B;
    float *C;
    int M, N, K;
} MatmulContext;

static void setup_matmul(MatmulContext *mc, int size)
{
    mc->M = mc->N = mc->K = size;
    mc->A = (float *) malloc((size_t) size * size * sizeof(float));
    mc->B = (float *) malloc((size_t) size * size * sizeof(float));
    mc->C = (float *) malloc((size_t) size * size * sizeof(float));

    for (int i = 0; i < size * size; i++) {
        mc->A[i] = (float) rand() / RAND_MAX;
        mc->B[i] = (float) rand() / RAND_MAX;
    }
}

static void teardown_matmul(MatmulContext *mc)
{
    free(mc->A);
    free(mc->B);
    free(mc->C);
}

/* ── Kernel implementations ───────────────────────────────────────── */

/* Naive: i-j-k triple loop */
static void naive_matmul(const float *A, const float *B, float *C, int M, int N, int K)
{
    memset(C, 0, (size_t) M * N * sizeof(float));
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < K; k++)
                C[i * N + j] += A[i * K + k] * B[k * N + j];
}

/* Blocked: i-k-j with cache blocking */
static void blocked_matmul(const float *A, const float *B, float *C, int M, int N, int K)
{
    enum { BLK = 64 };
    memset(C, 0, (size_t) M * N * sizeof(float));
    for (int ii = 0; ii < M; ii += BLK)
        for (int kk = 0; kk < K; kk += BLK)
            for (int jj = 0; jj < N; jj += BLK) {
                int i_end = (ii + BLK < M) ? ii + BLK : M;
                int k_end = (kk + BLK < K) ? kk + BLK : K;
                int j_end = (jj + BLK < N) ? jj + BLK : N;
                for (int i = ii; i < i_end; i++)
                    for (int k = kk; k < k_end; k++) {
                        float a = A[i * K + k];
                        for (int j = jj; j < j_end; j++)
                            C[i * N + j] += a * B[k * N + j];
                    }
            }
}

/* ── Benchmark runner ─────────────────────────────────────────────── */
typedef void (*matmul_fn)(const float *, const float *, float *, int, int, int);

static double bench_kernel(matmul_fn fn, MatmulContext *mc, int reps)
{
    /* Warmup */
    fn(mc->A, mc->B, mc->C, mc->M, mc->N, mc->K);

    double best = 1e18;
    for (int r = 0; r < reps; r++) {
        double t0 = now_sec();
        fn(mc->A, mc->B, mc->C, mc->M, mc->N, mc->K);
        double dt = now_sec() - t0;
        if (dt < best)
            best = dt;
    }
    return best;
}

int main(void)
{
    srand(42);

    printf("\n");
    printf("  ╔════════════════════════════════════════════════════════╗\n");
    printf("  ║  Matrix Multiplication Kernel Benchmark               ║\n");
    printf("  ║  Variants: Naive (ijk) · Blocked (ikj, 64-tile)      ║\n");
    printf("  ╚════════════════════════════════════════════════════════╝\n\n");

    const char *names[] = {"Naive", "Blocked"};
    matmul_fn funcs[] = {naive_matmul, blocked_matmul};
    int nv = 2;

    /* Limit naive to small sizes — it's O(n³) with no optimization */
    int max_naive = 512;

    printf("  %-12s %-10s %10s %10s %12s\n", "Size", "Variant", "GFLOPS", "% Peak", "Time (ms)");
    printf("  %-12s %-10s %10s %10s %12s\n", "────────────", "──────────", "──────────",
           "──────────", "────────────");

    for (int size = MIN_SIZE; size <= MAX_SIZE; size *= 2) {
        /* Adaptive reps: more reps for smaller sizes */
        int base_reps = (size <= 128) ? 20 : (size <= 512) ? 5 : (size <= 1024) ? 2 : 1;

        for (int v = 0; v < nv; v++) {
            /* Skip naive for large sizes */
            if (v == 0 && size > max_naive)
                continue;

            MatmulContext mc;
            setup_matmul(&mc, size);

            double best_t = bench_kernel(funcs[v], &mc, base_reps);
            double gflops = 2.0 * (double) size * size * size / best_t / 1e9;
            double pct = gflops / 102.4 * 100.0; /* M1 peak */
            double ms = best_t * 1000.0;

            printf("  %4d×%-6d %-10s %10.2f %9.1f%% %11.3f\n", size, size, names[v], gflops, pct,
                   ms);

            teardown_matmul(&mc);
        }
        printf("\n");
    }

    printf("  Peak reference: 102.4 GFLOPS (M1 single P-core)\n");
    printf("  Note: Use nova_gemm bench for optimized GEBP numbers.\n\n");

    return 0;
}
