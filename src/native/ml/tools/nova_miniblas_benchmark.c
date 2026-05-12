/**
 * nova_miniblas_benchmark.c
 * ═══════════════════════════════════════════════════════════════════════════
 * Nova ML — Mini-BLAS Matmul: Toward Apple M1 Peak GFLOPS
 *
 * Roofline (single P-core, FP32):
 *   Theoretical peak : 102.4 GFLOPS  (3.2GHz × 4 FMA × 4 SIMD × 2 ops)
 *   Memory BW        :  68.25 GB/s
 *   All sizes compute-bound → maximize ILP + register reuse
 *
 * Kernels (progressive):
 *  1. Baseline:       Naive ijk
 *  2. Trans-B:        Transposed-B (cache-friendly)
 *  3. Hybrid-v1:      Blocked-64 + Transposed + NEON 8-wide
 *  4. Hybrid-v2:      Blocked-64 + Transposed + NEON 8-wide + prefetch
 *  5. Micro-8×8:      8×8 register tiling + NEON + prefetch (mini-BLAS)
 *  6. Micro-8×8-deep: 8×8 + 4 accumulator groups (max ILP)
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
static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ── Memory ────────────────────────────────────────────────────── */
static float *alloc_mat(int n) {
    float *p = (float *)aligned_alloc(128, (size_t)n * n * sizeof(float));
    if (!p) { fprintf(stderr, "OOM\n"); exit(1); }
    return p;
}
static void fill_rand(float *m, int n) {
    for (int i = 0; i < n*n; i++) m[i] = (float)(rand()&0xFF)/128.f - 1.f;
}
static void zero_mat(float *m, int n) {
    memset(m, 0, (size_t)n*n*sizeof(float));
}
static float *make_transposed(const float *B, int n) {
    float *Bt = alloc_mat(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            Bt[j*n+i] = B[i*n+j];
    return Bt;
}

/* ═══════════════════════════════════════════════════════════════
 * 1. NAIVE
 * ═══════════════════════════════════════════════════════════════ */
static void matmul_naive(const float *A, const float *B, float *C, int n) {
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++) {
            float s=0;
            for (int k=0;k<n;k++) s += A[i*n+k]*B[k*n+j];
            C[i*n+j]=s;
        }
}

/* ═══════════════════════════════════════════════════════════════
 * 2. TRANSPOSED-B
 * ═══════════════════════════════════════════════════════════════ */
static void matmul_transb(const float *A, const float *B, float *C, int n) {
    float *Bt = make_transposed(B, n);
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++) {
            const float *ar=A+i*n, *br=Bt+j*n;
            float s=0;
            for (int k=0;k<n;k++) s+=ar[k]*br[k];
            C[i*n+j]=s;
        }
    free(Bt);
}

#if NOVA_HAS_NEON
/* ═══════════════════════════════════════════════════════════════
 * 3. HYBRID v1: Blocked-64 + Transposed + NEON 8-wide
 * ═══════════════════════════════════════════════════════════════ */
#define TILE 64
static void matmul_hybrid_v1(const float *A, const float *B, float *C, int n) {
    float *Bt = make_transposed(B, n);
    zero_mat(C, n);

    for (int ii=0; ii<n; ii+=TILE)
    for (int jj=0; jj<n; jj+=TILE) {
        int i_end = ii+TILE<n ? ii+TILE : n;
        int j_end = jj+TILE<n ? jj+TILE : n;

        for (int i=ii; i<i_end; i++) {
            for (int j=jj; j<j_end; j++) {
                const float *ar = A  + i*n;
                const float *br = Bt + j*n;
                float32x4_t acc0 = vdupq_n_f32(0.f);
                float32x4_t acc1 = vdupq_n_f32(0.f);
                int k=0;
                for (; k<=n-8; k+=8) {
                    acc0 = vmlaq_f32(acc0, vld1q_f32(ar+k),   vld1q_f32(br+k));
                    acc1 = vmlaq_f32(acc1, vld1q_f32(ar+k+4), vld1q_f32(br+k+4));
                }
                acc0 = vaddq_f32(acc0, acc1);
                float s = vaddvq_f32(acc0);
                for (; k<n; k++) s += ar[k]*br[k];
                C[i*n+j] = s;
            }
        }
    }
    free(Bt);
}

/* ═══════════════════════════════════════════════════════════════
 * 4. HYBRID v2: v1 + ARM64 prefetch
 * ═══════════════════════════════════════════════════════════════ */
static void matmul_hybrid_v2(const float *A, const float *B, float *C, int n) {
    float *Bt = make_transposed(B, n);
    zero_mat(C, n);

    for (int ii=0; ii<n; ii+=TILE)
    for (int jj=0; jj<n; jj+=TILE) {
        int i_end = ii+TILE<n ? ii+TILE : n;
        int j_end = jj+TILE<n ? jj+TILE : n;

        for (int i=ii; i<i_end; i++) {
            /* Prefetch next row of A */
            if (i+1 < i_end)
                __builtin_prefetch(A+(i+1)*n, 0, 3);

            for (int j=jj; j<j_end; j++) {
                const float *ar = A  + i*n;
                const float *br = Bt + j*n;

                /* Prefetch Bt row 2 ahead */
                if (j+2 < j_end)
                    __builtin_prefetch(Bt+(j+2)*n, 0, 2);

                float32x4_t acc0 = vdupq_n_f32(0.f);
                float32x4_t acc1 = vdupq_n_f32(0.f);
                int k=0;
                for (; k<=n-8; k+=8) {
                    __builtin_prefetch(ar+k+32, 0, 1);
                    __builtin_prefetch(br+k+32, 0, 1);
                    acc0 = vmlaq_f32(acc0, vld1q_f32(ar+k),   vld1q_f32(br+k));
                    acc1 = vmlaq_f32(acc1, vld1q_f32(ar+k+4), vld1q_f32(br+k+4));
                }
                acc0 = vaddq_f32(acc0, acc1);
                float s = vaddvq_f32(acc0);
                for (; k<n; k++) s += ar[k]*br[k];
                C[i*n+j] = s;
            }
        }
    }
    free(Bt);
}

/* ═══════════════════════════════════════════════════════════════
 * 5. MICRO-KERNEL 8×8
 *    Computes an 8-row × 8-col block using 8×8=64 NEON accumulators
 *    Each accumulator is a scalar accumulated via NEON FMA
 *    Inner k-loop: 8 A-scalars broadcast × 8 B-scalars = 64 FMAs/iter
 *    Then we step k by 1 — maximum register reuse
 * ═══════════════════════════════════════════════════════════════ */
#define MR 8   /* rows per micro-tile  */
#define NR 8   /* cols per micro-tile  */

/* Inner micro-kernel: computes C[i:i+MR][j:j+NR] += A[i:i+MR][*] * Bt[j:j+NR][*] */
static inline void __attribute__((always_inline))
micro_8x8(const float *A, const float *Bt, float *C, int n,
          int i_base, int j_base) {
    /* 8×8 = 64 scalar accumulators in NEON registers */
    float32x4_t c[8][2];   /* c[row][0..1] = 8 cols as 2×4-wide */
    for (int r=0;r<8;r++) {
        c[r][0] = vdupq_n_f32(0.f);
        c[r][1] = vdupq_n_f32(0.f);
    }

    for (int k=0; k<n; k++) {
        /* prefetch 8 steps ahead */
        __builtin_prefetch(A+(i_base+0)*n+k+8, 0, 1);
        __builtin_prefetch(Bt+(j_base+0)*n+k+8, 0, 1);

        /* Load 8 B-values (one per Bt row) — broadcast into pairs */
        /* Bt[j*n+k] = B[k*n+j] */
        float32x4_t bv0 = {Bt[(j_base+0)*n+k], Bt[(j_base+1)*n+k],
                            Bt[(j_base+2)*n+k], Bt[(j_base+3)*n+k]};
        float32x4_t bv1 = {Bt[(j_base+4)*n+k], Bt[(j_base+5)*n+k],
                            Bt[(j_base+6)*n+k], Bt[(j_base+7)*n+k]};

        /* For each of 8 A-rows: FMA with both b-vectors */
        for (int r=0; r<8; r++) {
            float32x4_t av = vdupq_n_f32(A[(i_base+r)*n+k]);
            c[r][0] = vmlaq_f32(c[r][0], av, bv0);
            c[r][1] = vmlaq_f32(c[r][1], av, bv1);
        }
    }

    /* Store results */
    for (int r=0; r<8; r++) {
        vst1q_f32(C+(i_base+r)*n+(j_base+0), c[r][0]);
        vst1q_f32(C+(i_base+r)*n+(j_base+4), c[r][1]);
    }
}

static void matmul_micro8x8(const float *A, const float *B, float *C, int n) {
    float *Bt = make_transposed(B, n);

    int i=0;
    for (; i<=n-MR; i+=MR) {
        int j=0;
        for (; j<=n-NR; j+=NR) {
            __builtin_prefetch(A+i*n, 0, 3);
            __builtin_prefetch(Bt+j*n, 0, 3);
            micro_8x8(A, Bt, C, n, i, j);
        }
        /* tail cols */
        for (; j<n; j++) {
            for (int r=0;r<MR;r++) {
                float s=0;
                for (int k=0;k<n;k++) s+=A[(i+r)*n+k]*Bt[j*n+k];
                C[(i+r)*n+j]=s;
            }
        }
    }
    /* tail rows */
    for (; i<n; i++)
        for (int j=0;j<n;j++) {
            float s=0;
            for (int k=0;k<n;k++) s+=A[i*n+k]*Bt[j*n+k];
            C[i*n+j]=s;
        }
    free(Bt);
}

/* ═══════════════════════════════════════════════════════════════
 * 6. MICRO-KERNEL 8×8 DEEP: 4 accumulator groups per row
 *    Unroll k-loop by 4 → 4× throughput on FMA pipeline
 * ═══════════════════════════════════════════════════════════════ */
static inline void __attribute__((always_inline))
micro_8x8_deep(const float *A, const float *Bt, float *C, int n,
               int i_base, int j_base) {
    float32x4_t c[8][2];
    for (int r=0;r<8;r++) { c[r][0]=vdupq_n_f32(0); c[r][1]=vdupq_n_f32(0); }

    int k=0;
    for (; k<=n-4; k+=4) {
        __builtin_prefetch(A+(i_base)*n+k+16, 0, 1);
        __builtin_prefetch(Bt+(j_base)*n+k+16, 0, 1);

        for (int kk=0; kk<4; kk++) {
            float32x4_t bv0 = {Bt[(j_base+0)*n+k+kk], Bt[(j_base+1)*n+k+kk],
                                Bt[(j_base+2)*n+k+kk], Bt[(j_base+3)*n+k+kk]};
            float32x4_t bv1 = {Bt[(j_base+4)*n+k+kk], Bt[(j_base+5)*n+k+kk],
                                Bt[(j_base+6)*n+k+kk], Bt[(j_base+7)*n+k+kk]};
            for (int r=0; r<8; r++) {
                float32x4_t av = vdupq_n_f32(A[(i_base+r)*n+k+kk]);
                c[r][0] = vmlaq_f32(c[r][0], av, bv0);
                c[r][1] = vmlaq_f32(c[r][1], av, bv1);
            }
        }
    }
    for (; k<n; k++) {
        float32x4_t bv0 = {Bt[(j_base+0)*n+k], Bt[(j_base+1)*n+k],
                            Bt[(j_base+2)*n+k], Bt[(j_base+3)*n+k]};
        float32x4_t bv1 = {Bt[(j_base+4)*n+k], Bt[(j_base+5)*n+k],
                            Bt[(j_base+6)*n+k], Bt[(j_base+7)*n+k]};
        for (int r=0; r<8; r++) {
            float32x4_t av = vdupq_n_f32(A[(i_base+r)*n+k]);
            c[r][0] = vmlaq_f32(c[r][0], av, bv0);
            c[r][1] = vmlaq_f32(c[r][1], av, bv1);
        }
    }
    for (int r=0;r<8;r++) {
        vst1q_f32(C+(i_base+r)*n+(j_base+0), c[r][0]);
        vst1q_f32(C+(i_base+r)*n+(j_base+4), c[r][1]);
    }
}

static void matmul_micro8x8_deep(const float *A, const float *B, float *C, int n) {
    float *Bt = make_transposed(B, n);
    int i=0;
    for (; i<=n-8; i+=8) {
        int j=0;
        for (; j<=n-8; j+=8)
            micro_8x8_deep(A, Bt, C, n, i, j);
        for (; j<n; j++) {
            for (int r=0;r<8;r++) {
                float s=0;
                for (int k=0;k<n;k++) s+=A[(i+r)*n+k]*Bt[j*n+k];
                C[(i+r)*n+j]=s;
            }
        }
    }
    for (; i<n; i++)
        for (int j=0;j<n;j++) {
            float s=0;
            for (int k=0;k<n;k++) s+=A[i*n+k]*Bt[j*n+k];
            C[i*n+j]=s;
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
    bool skip_large;
} BenchAlgo;

static double run_bench(BenchAlgo *algo, int n, int reps) {
    float *A = alloc_mat(n);
    float *B = alloc_mat(n);
    float *C = alloc_mat(n);
    fill_rand(A, n); fill_rand(B, n); zero_mat(C, n);

    algo->fn(A, B, C, n);   /* warm-up */

    double best = 1e18;
    for (int r=0; r<reps; r++) {
        zero_mat(C, n);
        double t0 = now_sec();
        algo->fn(A, B, C, n);
        double dt = now_sec() - t0;
        if (dt < best) best = dt;
    }
    free(A); free(B); free(C);
    return 2.0*n*n*n / best / 1e9;   /* GFLOPS */
}

/* Roofline */
static void print_roofline(void) {
    double peak_gf = 102.4;   /* M1 single P-core FP32 */
    double mem_bw  = 68.25;   /* GB/s */
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║         Apple M1 Single P-core Roofline (FP32)                 ║\n");
    printf("║  Theoretical Peak : %6.1f GFLOPS                               ║\n", peak_gf);
    printf("║  Memory Bandwidth :  %5.2f GB/s                                ║\n", mem_bw);
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║  Size       AI (FLOPs/B)   Roofline    Ridge point              ║\n");
    printf("║  ─────────────────────────────────────────────────              ║\n");
    int sizes[] = {64,128,256,512,1024};
    for (int s=0; s<5; s++) {
        int n = sizes[s];
        double ai   = (2.0*n*n*n) / (3.0*n*n*4.0);
        double roof = ai*mem_bw < peak_gf ? ai*mem_bw : peak_gf;
        const char *bound = ai*mem_bw < peak_gf ? "memory " : "compute";
        printf("║  %4dx%-4d  %8.1f         %6.1f GF  %s bound  ║\n",
               n, n, ai, roof, bound);
    }
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
}

/* ─────────────────────────────────────────────────────────────────
 * MAIN
 * ───────────────────────────────────────────────────────────────── */
int main(void) {
    srand(42);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║       Nova ML — Mini-BLAS Matmul Benchmark                      ║\n");
    printf("║       Platform: Apple M1 ARM64 + NEON                           ║\n");
    printf("║       Goal: Max single-core GFLOPS toward 102.4 peak            ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    print_roofline();

    BenchAlgo algos[] = {
        { "1. Naive",            matmul_naive,         true  },
        { "2. Transposed-B",     matmul_transb,        true  },
#if NOVA_HAS_NEON
        { "3. Hybrid-v1",        matmul_hybrid_v1,     false },
        { "4. Hybrid+Prefetch",  matmul_hybrid_v2,     false },
        { "5. Micro-8×8",        matmul_micro8x8,      false },
        { "6. Micro-8×8-Deep",   matmul_micro8x8_deep, false },
#endif
    };
    int nalgo  = sizeof(algos)/sizeof(algos[0]);
    int sizes[] = {64, 128, 256, 512, 1024};
    int reps[]  = {  5,   5,   3,   2,    2};
    int nsizes  = 5;
    double peak = 102.4;

    /* ── Header ── */
    printf("%-22s", "Algorithm");
    for (int s=0; s<nsizes; s++)
        printf("  %5dx%-5d", sizes[s], sizes[s]);
    printf("\n");
    printf("%-22s", "──────────────────────");
    for (int s=0; s<nsizes; s++) printf("  ─────────────");
    printf("\n");

    double best_gf[5] = {0};

    for (int a=0; a<nalgo; a++) {
        printf("%-22s", algos[a].name);
        for (int s=0; s<nsizes; s++) {
            int n = sizes[s];
            if (algos[a].skip_large && n>=512) {
                printf("  %13s", "     —     ");
                continue;
            }
            double gf = run_bench(&algos[a], n, reps[s]);
            double pct = gf/peak*100.0;
            printf("  %6.1f(%4.1f%%)", gf, pct);
            if (gf > best_gf[s]) best_gf[s] = gf;
        }
        printf("\n");
    }

    printf("%-22s", "──────────────────────");
    for (int s=0; s<nsizes; s++) printf("  ─────────────");
    printf("\n");
    printf("%-22s", "🏆 Best");
    for (int s=0; s<nsizes; s++)
        printf("  %6.1f(%4.1f%%)", best_gf[s], best_gf[s]/peak*100.0);
    printf("\n");
    printf("%-22s", "🎯 Roofline");
    for (int s=0; s<nsizes; s++) printf("  %6.1f(100.0%%)", peak);
    printf("\n\n");

    /* ── Detailed 256×256 comparison ── */
#if NOVA_HAS_NEON
    printf("─── 256×256 Detaylı Karşılaştırma ──────────────────\n");
    double ref = run_bench(&algos[0], 256, 3);
    for (int a=0; a<nalgo; a++) {
        double gf = run_bench(&algos[a], 256, 3);
        printf("  %-22s %6.2f GFLOPS  %5.1fx naive  %5.1f%% peak\n",
               algos[a].name, gf, gf/ref, gf/peak*100.0);
    }
    printf("\n");

    printf("─── 1024×1024 Detaylı Karşılaştırma ────────────────\n");
    for (int a=2; a<nalgo; a++) {   /* skip naive/transb for 1024 */
        double gf = run_bench(&algos[a], 1024, 2);
        printf("  %-22s %6.2f GFLOPS  %5.1f%% peak\n",
               algos[a].name, gf, gf/peak*100.0);
    }
    printf("\n");
#endif

    printf("✅ Mini-BLAS Benchmark tamamlandı!\n");
    printf("   Hedef: 102.4 GFLOPS (Apple M1 single P-core peak)\n\n");
    return 0;
}
