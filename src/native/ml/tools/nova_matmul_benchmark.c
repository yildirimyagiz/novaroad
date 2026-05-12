/**
 * nova_matmul_benchmark.c
 * ═══════════════════════════════════════════════════════════════════════════
 * Nova ML — Matmul GFLOPS Benchmark
 * Algorithms: Naive, Blocked, Transposed-B, NEON SIMD (ARM64)
 * Sizes: 64×64, 128×128, 256×256, 512×512, 1024×1024
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#  include <arm_neon.h>
#  define NOVA_HAS_NEON 1
#else
#  define NOVA_HAS_NEON 0
#endif

/* ── Timing ────────────────────────────────────────────────────── */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ── Memory ────────────────────────────────────────────────────── */
static float *alloc_matrix(int n) {
    float *p = (float *)aligned_alloc(64, (size_t)n * n * sizeof(float));
    if (!p) { fprintf(stderr, "OOM\n"); exit(1); }
    return p;
}
static void fill_rand(float *m, int n) {
    for (int i = 0; i < n * n; i++)
        m[i] = (float)(rand() & 0xFF) / 128.0f - 1.0f;
}
static void zero(float *m, int n) {
    memset(m, 0, (size_t)n * n * sizeof(float));
}

/* ─────────────────────────────────────────────────────────────────
 * 1. NAIVE — ijk
 * ───────────────────────────────────────────────────────────────── */
static void matmul_naive(const float *A, const float *B, float *C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            float s = 0.f;
            for (int k = 0; k < n; k++)
                s += A[i*n+k] * B[k*n+j];
            C[i*n+j] = s;
        }
}

/* ─────────────────────────────────────────────────────────────────
 * 2. TRANSPOSED-B — ikj, better cache on B
 * ───────────────────────────────────────────────────────────────── */
static void matmul_transposed(const float *A, const float *B, float *C, int n) {
    /* Transpose B into Bt */
    float *Bt = alloc_matrix(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            Bt[j*n+i] = B[i*n+j];

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            float s = 0.f;
            const float *ar = A  + i*n;
            const float *br = Bt + j*n;
            for (int k = 0; k < n; k++)
                s += ar[k] * br[k];
            C[i*n+j] = s;
        }
    free(Bt);
}

/* ─────────────────────────────────────────────────────────────────
 * 3. BLOCKED — cache-tiled (block=64)
 * ───────────────────────────────────────────────────────────────── */
#define BLOCK 64
static void matmul_blocked(const float *A, const float *B, float *C, int n) {
    zero(C, n);  /* accumulate into C */
    for (int ii = 0; ii < n; ii += BLOCK)
    for (int kk = 0; kk < n; kk += BLOCK)
    for (int jj = 0; jj < n; jj += BLOCK) {
        int i_end = ii+BLOCK < n ? ii+BLOCK : n;
        int k_end = kk+BLOCK < n ? kk+BLOCK : n;
        int j_end = jj+BLOCK < n ? jj+BLOCK : n;
        for (int i = ii; i < i_end; i++)
        for (int k = kk; k < k_end; k++) {
            float a = A[i*n+k];
            for (int j = jj; j < j_end; j++)
                C[i*n+j] += a * B[k*n+j];
        }
    }
}

/* ─────────────────────────────────────────────────────────────────
 * 4. NEON SIMD (ARM64 only) — 4-wide FMA inner loop
 * ───────────────────────────────────────────────────────────────── */
#if NOVA_HAS_NEON
/* 4×4 register tiling NEON kernel — computes 4 rows × 4 cols per iteration */
static void matmul_neon(const float *A, const float *B, float *C, int n) {
    float *Bt = alloc_matrix(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            Bt[j*n+i] = B[i*n+j];

    int i = 0;
    for (; i <= n-4; i+=4) {
        int j = 0;
        for (; j <= n-4; j+=4) {
            /* 4×4 accumulator registers */
            float32x4_t c00=vdupq_n_f32(0), c01=vdupq_n_f32(0),
                        c02=vdupq_n_f32(0), c03=vdupq_n_f32(0),
                        c10=vdupq_n_f32(0), c11=vdupq_n_f32(0),
                        c12=vdupq_n_f32(0), c13=vdupq_n_f32(0),
                        c20=vdupq_n_f32(0), c21=vdupq_n_f32(0),
                        c22=vdupq_n_f32(0), c23=vdupq_n_f32(0),
                        c30=vdupq_n_f32(0), c31=vdupq_n_f32(0),
                        c32=vdupq_n_f32(0), c33=vdupq_n_f32(0);

            for (int k = 0; k < n; k++) {
                float32x4_t a0 = vdupq_n_f32(A[(i+0)*n+k]);
                float32x4_t a1 = vdupq_n_f32(A[(i+1)*n+k]);
                float32x4_t a2 = vdupq_n_f32(A[(i+2)*n+k]);
                float32x4_t a3 = vdupq_n_f32(A[(i+3)*n+k]);

                float32x4_t b0 = vld1q_f32(Bt+(j+0)*n+k);
                /* Note: Bt is transposed, so Bt[j*n+k] = B[k*n+j] */
                /* Each b-vector is a length-4 slice — but for 4×4 tiling
                   we need 4 separate b-scalars per k. Use scalar broadcast. */
                float b00 = Bt[(j+0)*n+k], b01 = Bt[(j+1)*n+k];
                float b02 = Bt[(j+2)*n+k], b03 = Bt[(j+3)*n+k];
                (void)b0;

                float32x4_t bv0 = vdupq_n_f32(b00);
                float32x4_t bv1 = vdupq_n_f32(b01);
                float32x4_t bv2 = vdupq_n_f32(b02);
                float32x4_t bv3 = vdupq_n_f32(b03);

                c00 = vmlaq_f32(c00, a0, bv0); c01 = vmlaq_f32(c01, a0, bv1);
                c02 = vmlaq_f32(c02, a0, bv2); c03 = vmlaq_f32(c03, a0, bv3);
                c10 = vmlaq_f32(c10, a1, bv0); c11 = vmlaq_f32(c11, a1, bv1);
                c12 = vmlaq_f32(c12, a1, bv2); c13 = vmlaq_f32(c13, a1, bv3);
                c20 = vmlaq_f32(c20, a2, bv0); c21 = vmlaq_f32(c21, a2, bv1);
                c22 = vmlaq_f32(c22, a2, bv2); c23 = vmlaq_f32(c23, a2, bv3);
                c30 = vmlaq_f32(c30, a3, bv0); c31 = vmlaq_f32(c31, a3, bv1);
                c32 = vmlaq_f32(c32, a3, bv2); c33 = vmlaq_f32(c33, a3, bv3);
            }

            /* Horizontal reduce and store */
            #define HSUM(v) (vaddvq_f32(v))
            C[(i+0)*n+(j+0)]=HSUM(c00); C[(i+0)*n+(j+1)]=HSUM(c01);
            C[(i+0)*n+(j+2)]=HSUM(c02); C[(i+0)*n+(j+3)]=HSUM(c03);
            C[(i+1)*n+(j+0)]=HSUM(c10); C[(i+1)*n+(j+1)]=HSUM(c11);
            C[(i+1)*n+(j+2)]=HSUM(c12); C[(i+1)*n+(j+3)]=HSUM(c13);
            C[(i+2)*n+(j+0)]=HSUM(c20); C[(i+2)*n+(j+1)]=HSUM(c21);
            C[(i+2)*n+(j+2)]=HSUM(c22); C[(i+2)*n+(j+3)]=HSUM(c23);
            C[(i+3)*n+(j+0)]=HSUM(c30); C[(i+3)*n+(j+1)]=HSUM(c31);
            C[(i+3)*n+(j+2)]=HSUM(c32); C[(i+3)*n+(j+3)]=HSUM(c33);
            #undef HSUM
        }
        /* tail columns */
        for (; j < n; j++) {
            float s0=0,s1=0,s2=0,s3=0;
            for (int k=0;k<n;k++){
                float bk=Bt[j*n+k];
                s0+=A[(i+0)*n+k]*bk; s1+=A[(i+1)*n+k]*bk;
                s2+=A[(i+2)*n+k]*bk; s3+=A[(i+3)*n+k]*bk;
            }
            C[(i+0)*n+j]=s0; C[(i+1)*n+j]=s1;
            C[(i+2)*n+j]=s2; C[(i+3)*n+j]=s3;
        }
    }
    /* tail rows */
    for (; i < n; i++)
        for (int j=0;j<n;j++){
            float s=0;
            for(int k=0;k<n;k++) s+=A[i*n+k]*Bt[j*n+k];
            C[i*n+j]=s;
        }
    free(Bt);
}

/* NEON 8-wide vectorized dot product (Transposed-B, 8 elements per iter) */
static void matmul_neon8(const float *A, const float *B, float *C, int n) {
    float *Bt = alloc_matrix(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            Bt[j*n+i] = B[i*n+j];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            const float *ar = A  + i*n;
            const float *br = Bt + j*n;
            float32x4_t acc0 = vdupq_n_f32(0.f);
            float32x4_t acc1 = vdupq_n_f32(0.f);
            int k = 0;
            for (; k <= n-8; k+=8) {
                acc0 = vmlaq_f32(acc0, vld1q_f32(ar+k),   vld1q_f32(br+k));
                acc1 = vmlaq_f32(acc1, vld1q_f32(ar+k+4), vld1q_f32(br+k+4));
            }
            acc0 = vaddq_f32(acc0, acc1);
            float s = vaddvq_f32(acc0);
            for (; k < n; k++) s += ar[k] * br[k];
            C[i*n+j] = s;
        }
    }
    free(Bt);
}
#endif /* NOVA_HAS_NEON */

/* ─────────────────────────────────────────────────────────────────
 * Benchmark runner
 * ───────────────────────────────────────────────────────────────── */
typedef struct {
    const char *name;
    void (*fn)(const float*, const float*, float*, int);
    bool skip_large;   /* skip for n>=512 (too slow for naive) */
} BenchAlgo;

static double run_bench(BenchAlgo *algo, int n, int reps) {
    float *A = alloc_matrix(n);
    float *B = alloc_matrix(n);
    float *C = alloc_matrix(n);
    fill_rand(A, n);
    fill_rand(B, n);
    zero(C, n);

    /* warm-up */
    algo->fn(A, B, C, n);

    double best = 1e18;
    for (int r = 0; r < reps; r++) {
        zero(C, n);
        double t0 = now_sec();
        algo->fn(A, B, C, n);
        double t1 = now_sec();
        if (t1-t0 < best) best = t1-t0;
    }

    free(A); free(B); free(C);
    double flops = 2.0 * n * n * n;
    return flops / best / 1e9;   /* GFLOPS */
}

/* ─────────────────────────────────────────────────────────────────
 * MAIN
 * ───────────────────────────────────────────────────────────────── */
int main(void) {
    srand(42);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║          Nova ML — Matmul GFLOPS Benchmark                      ║\n");
    printf("║  Platform: %s                                    ║\n",
#if NOVA_HAS_NEON
           "ARM64 + NEON ✅   "
#else
           "x86/generic       "
#endif
    );
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    BenchAlgo algos[] = {
        { "Naive (ijk)",      matmul_naive,       true  },
        { "Transposed-B",     matmul_transposed,  true  },
        { "Blocked (64)",     matmul_blocked,     false },
#if NOVA_HAS_NEON
        { "NEON 4×4 tile",    matmul_neon,        false },
        { "NEON 8-wide",      matmul_neon8,       false },
#endif
    };
    int nalgo = sizeof(algos) / sizeof(algos[0]);

    int sizes[]  = { 64, 128, 256, 512, 1024 };
    int reps[]   = {  5,   5,   3,   2,    1 };
    int nsizes   = sizeof(sizes) / sizeof(sizes[0]);

    /* Header */
    printf("%-18s", "Algorithm");
    for (int s = 0; s < nsizes; s++)
        printf("  %6d×%-6d", sizes[s], sizes[s]);
    printf("\n");

    /* Separator */
    printf("%-18s", "──────────────────");
    for (int s = 0; s < nsizes; s++)
        printf("  ─────────────");
    printf("\n");

    /* Best GFLOPS per algo */
    double best_gflops[5]  = {0};   /* per size */

    for (int a = 0; a < nalgo; a++) {
        printf("%-18s", algos[a].name);
        for (int s = 0; s < nsizes; s++) {
            int n = sizes[s];
            if (algos[a].skip_large && n >= 512) {
                printf("  %13s", "     —     ");
                continue;
            }
            double gf = run_bench(&algos[a], n, reps[s]);
            printf("  %10.2f GF", gf);
            if (gf > best_gflops[s]) best_gflops[s] = gf;
        }
        printf("\n");
    }

    /* Best row */
    printf("%-18s", "──────────────────");
    for (int s = 0; s < nsizes; s++)
        printf("  ─────────────");
    printf("\n");
    printf("%-18s", "🏆 Best");
    for (int s = 0; s < nsizes; s++)
        printf("  %10.2f GF", best_gflops[s]);
    printf("\n\n");

    /* Summary */
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  Peak 1024×1024: %6.2f GFLOPS                                  ║\n",
           best_gflops[4]);
    printf("║  Peak  512×512:  %6.2f GFLOPS                                  ║\n",
           best_gflops[3]);
    printf("║  Peak  256×256:  %6.2f GFLOPS                                  ║\n",
           best_gflops[2]);
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    /* Detailed algorithm comparison */
#if NOVA_HAS_NEON
    printf("─── Algoritma Karşılaştırması (256×256) ───────────\n");
    double gf_naive    = run_bench(&algos[0], 256, 3);
    double gf_trans    = run_bench(&algos[1], 256, 3);
    double gf_blocked  = run_bench(&algos[2], 256, 3);
    double gf_neon4    = run_bench(&algos[3], 256, 3);
    double gf_neon8    = run_bench(&algos[4], 256, 3);
    printf("  Naive:          %6.2f GFLOPS  (baseline)\n",   gf_naive);
    printf("  Transposed-B:   %6.2f GFLOPS  (%5.1fx naive)\n", gf_trans,   gf_trans/gf_naive);
    printf("  Blocked-64:     %6.2f GFLOPS  (%5.1fx naive)\n", gf_blocked, gf_blocked/gf_naive);
    printf("  NEON 4×4 tile:  %6.2f GFLOPS  (%5.1fx naive)\n", gf_neon4,  gf_neon4/gf_naive);
    printf("  NEON 8-wide:    %6.2f GFLOPS  (%5.1fx naive)\n", gf_neon8,  gf_neon8/gf_naive);
    double peak = gf_trans > gf_blocked ? gf_trans : gf_blocked;
    peak = peak > gf_neon4 ? peak : gf_neon4;
    peak = peak > gf_neon8 ? peak : gf_neon8;
    printf("  ─────────────────────────────────────────────\n");
    printf("  🏆 En iyi:      %6.2f GFLOPS  (%5.1fx naive)\n", peak, peak/gf_naive);
    printf("\n");
#endif

    printf("✅ Benchmark tamamlandı!\n\n");
    return 0;
}
