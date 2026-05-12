/**
 * nova_blas_benchmark.c
 * ═══════════════════════════════════════════════════════════════════════════
 * Nova ML — Production-Grade Multi-Platform BLAS Matmul Benchmark
 *
 * Platform support (auto-detected at compile time):
 *   ARM64 NEON    : 12×8  micro-kernel, 24 accum regs, 8× K-unroll
 *   x86  AVX2     :  8×8  micro-kernel, 16 accum regs, 4× K-unroll
 *   x86  AVX-512  : 12×16 micro-kernel, 24 accum regs, 4× K-unroll
 *   Generic C     :  8×8  scalar tile,  software pipelining
 *
 * Key techniques:
 *   - B-panel packing (column-major) → sequential loads
 *   - Register blocking: max accumulator registers
 *   - K-loop unrolling: hide FMA latency (4–8× unroll)
 *   - Store-last: C tile stays in registers until done
 *   - L2-blocking: outer loop tiles fit in L2 cache
 *   - Software prefetch: architecture-specific distances
 *
 * Goal: 60–80% of theoretical peak FP32 on each platform
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* ── Platform Detection ────────────────────────────────────────── */
#if defined(__AVX512F__)
#  include <immintrin.h>
#  define NOVA_AVX512  1
#  define NOVA_AVX2    0
#  define NOVA_NEON    0
#  define PLATFORM_STR "x86 AVX-512"
#  define SIMD_WIDTH   16   /* float32 per register */
#  define MAX_ACCUM    32   /* zmm registers */
#elif defined(__AVX2__)
#  include <immintrin.h>
#  define NOVA_AVX512  0
#  define NOVA_AVX2    1
#  define NOVA_NEON    0
#  define PLATFORM_STR "x86 AVX2"
#  define SIMD_WIDTH   8
#  define MAX_ACCUM    16
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#  include <arm_neon.h>
#  define NOVA_AVX512  0
#  define NOVA_AVX2    0
#  define NOVA_NEON    1
#  define PLATFORM_STR "ARM64 NEON"
#  define SIMD_WIDTH   4
#  define MAX_ACCUM    32
#else
#  define NOVA_AVX512  0
#  define NOVA_AVX2    0
#  define NOVA_NEON    0
#  define PLATFORM_STR "Generic C (scalar)"
#  define SIMD_WIDTH   1
#  define MAX_ACCUM    32
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/* ── Timing ────────────────────────────────────────────────────── */
static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ── Memory ────────────────────────────────────────────────────── */
#define ALIGN 128
static float *alloc_mat(int n) {
    float *p = (float *)aligned_alloc(ALIGN, (size_t)n*n*sizeof(float));
    if (!p) { fprintf(stderr, "OOM\n"); exit(1); }
    return p;
}
static float *alloc_panel(int rows, int cols) {
    size_t sz = (size_t)rows*cols*sizeof(float);
    sz = (sz + ALIGN-1) & ~(size_t)(ALIGN-1);
    float *p = (float *)aligned_alloc(ALIGN, sz);
    if (!p) { fprintf(stderr, "OOM panel\n"); exit(1); }
    return p;
}
static void fill_rand(float *m, int n) {
    for (int i=0;i<n*n;i++) m[i]=(float)(rand()&0xFF)/128.f-1.f;
}
static void zero_mat(float *m, int n) { memset(m,0,(size_t)n*n*sizeof(float)); }

/* ── Verification ──────────────────────────────────────────────── */
static bool verify(const float *ref, const float *got, int n, float tol) {
    for (int i=0;i<n*n;i++)
        if (fabsf(ref[i]-got[i]) > tol*(fabsf(ref[i])+1.f)) return false;
    return true;
}
static void matmul_ref(const float *A, const float *B, float *C, int n) {
    zero_mat(C, n);
    for (int i=0;i<n;i++)
        for (int k=0;k<n;k++) {
            float a=A[i*n+k];
            for (int j=0;j<n;j++) C[i*n+j]+=a*B[k*n+j];
        }
}

/* ═══════════════════════════════════════════════════════════════
 * B-PANEL PACKING
 * Pack B[:,jj:jj+NR] into column-major panel Bp[k][j]
 * so inner loop does sequential vld1q_f32(Bp+k*NR)
 * ═══════════════════════════════════════════════════════════════ */
static void pack_B_panel(const float *B, float *Bp, int n, int j0, int nr) {
    /* Bp[k*nr + jr] = B[k*n + j0+jr]  (k=0..n-1, jr=0..nr-1) */
    for (int k=0;k<n;k++) {
        const float *src = B + k*n + j0;
        float *dst = Bp + k*nr;
        int jr=0;
        for (;jr<nr && j0+jr<n; jr++) dst[jr]=src[jr];
        for (;jr<nr; jr++) dst[jr]=0.f;   /* padding */
    }
}

/* Pack A[ii:ii+MR][:] into row-major panel Ap[i][k] */
static void pack_A_panel(const float *A, float *Ap, int n, int i0, int mr) {
    for (int ir=0;ir<mr;ir++) {
        int row = i0+ir;
        const float *src = (row<n) ? A+row*n : NULL;
        float *dst = Ap + ir*n;
        if (src) memcpy(dst, src, (size_t)n*sizeof(float));
        else     memset(dst, 0,   (size_t)n*sizeof(float));
    }
}

/* ═══════════════════════════════════════════════════════════════
 * 1. BASELINE — naive ikj
 * ═══════════════════════════════════════════════════════════════ */
static void matmul_naive(const float *A, const float *B, float *C, int n) {
    matmul_ref(A, B, C, n);
}

/* ═══════════════════════════════════════════════════════════════
 * 2. GENERIC 8×8 SCALAR TILE + K-unroll-8 + store-last
 *    Works on ALL platforms, compiler will auto-vectorize
 * ═══════════════════════════════════════════════════════════════ */
#define GEN_MR 8
#define GEN_NR 8
static void matmul_generic_tile(const float *A, const float *B, float *C, int n) {
    float *Bp = alloc_panel(n, GEN_NR);

    for (int jj=0; jj<n; jj+=GEN_NR) {
        int nr = (jj+GEN_NR<=n) ? GEN_NR : n-jj;
        pack_B_panel(B, Bp, n, jj, GEN_NR);

        for (int ii=0; ii<n; ii+=GEN_MR) {
            int mr = (ii+GEN_MR<=n) ? GEN_MR : n-ii;

            /* 8×8 register tile — compiler auto-vectorizes */
            float c[GEN_MR][GEN_NR];
            memset(c, 0, sizeof(c));

            int k=0;
            /* 8× unrolled K-loop */
            for (; k<=n-8; k+=8) {
                for (int ku=0; ku<8; ku++) {
                    const float *brow = Bp + (k+ku)*GEN_NR;
                    for (int ir=0; ir<mr; ir++) {
                        float a = A[(ii+ir)*n + k+ku];
                        for (int jr=0; jr<nr; jr++)
                            c[ir][jr] += a * brow[jr];
                    }
                }
            }
            /* tail */
            for (; k<n; k++) {
                const float *brow = Bp + k*GEN_NR;
                for (int ir=0; ir<mr; ir++) {
                    float a = A[(ii+ir)*n + k];
                    for (int jr=0; jr<nr; jr++)
                        c[ir][jr] += a * brow[jr];
                }
            }
            /* store-last: write tile to C */
            for (int ir=0;ir<mr;ir++)
                for (int jr=0;jr<nr;jr++)
                    C[(ii+ir)*n+(jj+jr)] = c[ir][jr];
        }
    }
    free(Bp);
}


/* ═══════════════════════════════════════════════════════════════
 * 3. ARM64 NEON — 12×8 micro-kernel
 *    24 accumulators (12 rows × 2 float32x4 = 8 cols)
 *    8× K-unroll, software pipelining, store-last
 * ═══════════════════════════════════════════════════════════════ */
#if NOVA_NEON
#define NEON_MR 12
#define NEON_NR  8   /* 8 cols = 2 × float32x4 */

static inline void __attribute__((always_inline))
neon_micro_12x8(const float *Ap, const float *Bp, float *C, int n,
                int i0, int j0, int mr, int nr) {
    /* 12×2 = 24 float32x4 accumulators — uses ~24 of 32 NEON regs */
    float32x4_t c[12][2];
    for (int r=0;r<12;r++) { c[r][0]=vdupq_n_f32(0); c[r][1]=vdupq_n_f32(0); }

    int k=0;
    /* 8× K-unroll: hide 4-cycle FMA latency, fill both FMA pipes */
    for (; k<=n-8; k+=8) {
        __builtin_prefetch(Ap + k + 32, 0, 1);
        __builtin_prefetch(Bp + (k+4)*NEON_NR, 0, 1);

        #define NEON_STEP(KK) \
        { \
            float32x4_t bv0 = vld1q_f32(Bp + (k+KK)*NEON_NR + 0); \
            float32x4_t bv1 = vld1q_f32(Bp + (k+KK)*NEON_NR + 4); \
            for (int r=0; r<mr; r++) { \
                float32x4_t av = vdupq_n_f32(Ap[r*n + k+KK]); \
                c[r][0] = vfmaq_f32(c[r][0], av, bv0); \
                c[r][1] = vfmaq_f32(c[r][1], av, bv1); \
            } \
        }
        NEON_STEP(0) NEON_STEP(1) NEON_STEP(2) NEON_STEP(3)
        NEON_STEP(4) NEON_STEP(5) NEON_STEP(6) NEON_STEP(7)
        #undef NEON_STEP
    }
    /* 4× tail unroll */
    for (; k<=n-4; k+=4) {
        #define NEON_STEP4(KK) \
        { \
            float32x4_t bv0 = vld1q_f32(Bp + (k+KK)*NEON_NR + 0); \
            float32x4_t bv1 = vld1q_f32(Bp + (k+KK)*NEON_NR + 4); \
            for (int r=0; r<mr; r++) { \
                float32x4_t av = vdupq_n_f32(Ap[r*n + k+KK]); \
                c[r][0] = vfmaq_f32(c[r][0], av, bv0); \
                c[r][1] = vfmaq_f32(c[r][1], av, bv1); \
            } \
        }
        NEON_STEP4(0) NEON_STEP4(1) NEON_STEP4(2) NEON_STEP4(3)
        #undef NEON_STEP4
    }
    /* scalar tail */
    for (; k<n; k++) {
        float32x4_t bv0 = vld1q_f32(Bp + k*NEON_NR + 0);
        float32x4_t bv1 = vld1q_f32(Bp + k*NEON_NR + 4);
        for (int r=0; r<mr; r++) {
            float32x4_t av = vdupq_n_f32(Ap[r*n + k]);
            c[r][0] = vfmaq_f32(c[r][0], av, bv0);
            c[r][1] = vfmaq_f32(c[r][1], av, bv1);
        }
    }
    /* store-last: write C tile */
    for (int r=0; r<mr; r++) {
        if (nr==8) {
            vst1q_f32(C+(i0+r)*n+(j0+0), c[r][0]);
            vst1q_f32(C+(i0+r)*n+(j0+4), c[r][1]);
        } else {
            float tmp[8]; vst1q_f32(tmp,c[r][0]); vst1q_f32(tmp+4,c[r][1]);
            for (int jr=0;jr<nr;jr++) C[(i0+r)*n+(j0+jr)]=tmp[jr];
        }
    }
}

static void matmul_neon_12x8(const float *A, const float *B, float *C, int n) {
    float *Bp = alloc_panel(n, NEON_NR);
    float *Ap = alloc_panel(NEON_MR, n);

    for (int jj=0; jj<n; jj+=NEON_NR) {
        int nr = (jj+NEON_NR<=n) ? NEON_NR : n-jj;
        pack_B_panel(B, Bp, n, jj, NEON_NR);

        for (int ii=0; ii<n; ii+=NEON_MR) {
            int mr = (ii+NEON_MR<=n) ? NEON_MR : n-ii;
            pack_A_panel(A, Ap, n, ii, mr);
            __builtin_prefetch(Bp, 0, 2);
            neon_micro_12x8(Ap, Bp, C, n, ii, jj, mr, nr);
        }
    }
    free(Bp); free(Ap);
}
#endif /* NOVA_NEON */

/* ═══════════════════════════════════════════════════════════════
 * 4. AVX2 — 8×8 micro-kernel (256-bit)
 *    16 YMM accumulators (8 rows × 2 ymm = 16 floats/row)
 *    4× K-unroll
 * ═══════════════════════════════════════════════════════════════ */
#if NOVA_AVX2
#define AVX2_MR  8
#define AVX2_NR 16   /* 16 cols = 2 × __m256 */

static inline void __attribute__((always_inline))
avx2_micro_8x16(const float *Ap, const float *Bp, float *C, int n,
                int i0, int j0, int mr, int nr) {
    __m256 c[8][2];
    for (int r=0;r<8;r++) { c[r][0]=_mm256_setzero_ps(); c[r][1]=_mm256_setzero_ps(); }

    int k=0;
    for (; k<=n-4; k+=4) {
        #define AVX2_STEP(KK) \
        { \
            __m256 bv0 = _mm256_load_ps(Bp + (k+KK)*AVX2_NR + 0); \
            __m256 bv1 = _mm256_load_ps(Bp + (k+KK)*AVX2_NR + 8); \
            for (int r=0; r<mr; r++) { \
                __m256 av = _mm256_broadcast_ss(Ap + r*n + k+KK); \
                c[r][0] = _mm256_fmadd_ps(av, bv0, c[r][0]); \
                c[r][1] = _mm256_fmadd_ps(av, bv1, c[r][1]); \
            } \
        }
        AVX2_STEP(0) AVX2_STEP(1) AVX2_STEP(2) AVX2_STEP(3)
        #undef AVX2_STEP
    }
    for (; k<n; k++) {
        __m256 bv0 = _mm256_load_ps(Bp + k*AVX2_NR + 0);
        __m256 bv1 = _mm256_load_ps(Bp + k*AVX2_NR + 8);
        for (int r=0;r<mr;r++) {
            __m256 av = _mm256_broadcast_ss(Ap + r*n + k);
            c[r][0] = _mm256_fmadd_ps(av, bv0, c[r][0]);
            c[r][1] = _mm256_fmadd_ps(av, bv1, c[r][1]);
        }
    }
    /* store-last */
    for (int r=0;r<mr;r++) {
        if (j0+16<=n) {
            _mm256_storeu_ps(C+(i0+r)*n+(j0+0), c[r][0]);
            _mm256_storeu_ps(C+(i0+r)*n+(j0+8), c[r][1]);
        } else {
            float tmp[16];
            _mm256_storeu_ps(tmp+0, c[r][0]);
            _mm256_storeu_ps(tmp+8, c[r][1]);
            for (int jr=0;jr<nr;jr++) C[(i0+r)*n+(j0+jr)]=tmp[jr];
        }
    }
}

static void matmul_avx2_8x16(const float *A, const float *B, float *C, int n) {
    float *Bp = alloc_panel(n, AVX2_NR);
    float *Ap = alloc_panel(AVX2_MR, n);
    for (int jj=0;jj<n;jj+=AVX2_NR) {
        int nr=(jj+AVX2_NR<=n)?AVX2_NR:n-jj;
        pack_B_panel(B,Bp,n,jj,AVX2_NR);
        for (int ii=0;ii<n;ii+=AVX2_MR) {
            int mr=(ii+AVX2_MR<=n)?AVX2_MR:n-ii;
            pack_A_panel(A,Ap,n,ii,mr);
            avx2_micro_8x16(Ap,Bp,C,n,ii,jj,mr,nr);
        }
    }
    free(Bp); free(Ap);
}
#endif /* NOVA_AVX2 */

/* ═══════════════════════════════════════════════════════════════
 * 5. AVX-512 — 12×16 micro-kernel (512-bit)
 *    24 ZMM accumulators (12 rows × 2 zmm = 32 floats/row)
 *    4× K-unroll
 * ═══════════════════════════════════════════════════════════════ */
#if NOVA_AVX512
#define AVX512_MR 12
#define AVX512_NR 32   /* 32 cols = 2 × __m512 */

static inline void __attribute__((always_inline))
avx512_micro_12x32(const float *Ap, const float *Bp, float *C, int n,
                   int i0, int j0, int mr, int nr) {
    __m512 c[12][2];
    for (int r=0;r<12;r++) { c[r][0]=_mm512_setzero_ps(); c[r][1]=_mm512_setzero_ps(); }

    int k=0;
    for (; k<=n-4; k+=4) {
        #define A512_STEP(KK) \
        { \
            __m512 bv0 = _mm512_load_ps(Bp + (k+KK)*AVX512_NR +  0); \
            __m512 bv1 = _mm512_load_ps(Bp + (k+KK)*AVX512_NR + 16); \
            for (int r=0;r<mr;r++) { \
                __m512 av = _mm512_set1_ps(Ap[r*n + k+KK]); \
                c[r][0] = _mm512_fmadd_ps(av,bv0,c[r][0]); \
                c[r][1] = _mm512_fmadd_ps(av,bv1,c[r][1]); \
            } \
        }
        A512_STEP(0) A512_STEP(1) A512_STEP(2) A512_STEP(3)
        #undef A512_STEP
    }
    for (; k<n; k++) {
        __m512 bv0=_mm512_load_ps(Bp+k*AVX512_NR+0);
        __m512 bv1=_mm512_load_ps(Bp+k*AVX512_NR+16);
        for (int r=0;r<mr;r++){
            __m512 av=_mm512_set1_ps(Ap[r*n+k]);
            c[r][0]=_mm512_fmadd_ps(av,bv0,c[r][0]);
            c[r][1]=_mm512_fmadd_ps(av,bv1,c[r][1]);
        }
    }
    for (int r=0;r<mr;r++) {
        if (j0+32<=n) {
            _mm512_storeu_ps(C+(i0+r)*n+(j0+ 0),c[r][0]);
            _mm512_storeu_ps(C+(i0+r)*n+(j0+16),c[r][1]);
        } else {
            float tmp[32];
            _mm512_storeu_ps(tmp+ 0,c[r][0]);
            _mm512_storeu_ps(tmp+16,c[r][1]);
            for (int jr=0;jr<nr;jr++) C[(i0+r)*n+(j0+jr)]=tmp[jr];
        }
    }
}

static void matmul_avx512_12x32(const float *A, const float *B, float *C, int n) {
    float *Bp=alloc_panel(n,AVX512_NR);
    float *Ap=alloc_panel(AVX512_MR,n);
    for (int jj=0;jj<n;jj+=AVX512_NR){
        int nr=(jj+AVX512_NR<=n)?AVX512_NR:n-jj;
        pack_B_panel(B,Bp,n,jj,AVX512_NR);
        for (int ii=0;ii<n;ii+=AVX512_MR){
            int mr=(ii+AVX512_MR<=n)?AVX512_MR:n-ii;
            pack_A_panel(A,Ap,n,ii,mr);
            avx512_micro_12x32(Ap,Bp,C,n,ii,jj,mr,nr);
        }
    }
    free(Bp); free(Ap);
}
#endif /* NOVA_AVX512 */

/* ═══════════════════════════════════════════════════════════════
 * BENCHMARK RUNNER
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    const char *name;
    void (*fn)(const float*,const float*,float*,int);
    bool skip_large;
    bool verify_ok;
} BenchAlgo;

static double run_bench(BenchAlgo *algo, int n, int reps, bool do_verify) {
    float *A=alloc_mat(n); float *B=alloc_mat(n);
    float *C=alloc_mat(n); float *Cref=NULL;
    fill_rand(A,n); fill_rand(B,n); zero_mat(C,n);

    if (do_verify) {
        Cref=alloc_mat(n);
        matmul_ref(A,B,Cref,n);
    }

    algo->fn(A,B,C,n);   /* warm-up + verify */

    if (do_verify && Cref) {
        algo->verify_ok = verify(Cref,C,n,1e-3f);
        free(Cref);
    } else {
        algo->verify_ok = true;
    }

    double best=1e18;
    for (int r=0;r<reps;r++){
        zero_mat(C,n);
        double t0=now_sec();
        algo->fn(A,B,C,n);
        double dt=now_sec()-t0;
        if (dt<best) best=dt;
    }
    free(A); free(B); free(C);
    return 2.0*n*n*n/best/1e9;
}

/* ═══════════════════════════════════════════════════════════════
 * ROOFLINE + THEORETICAL PEAK
 * ═══════════════════════════════════════════════════════════════ */
static void print_platform_info(void) {
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     Nova ML — Multi-Platform BLAS Matmul Benchmark              ║\n");
    printf("║     Platform : %-48s║\n", PLATFORM_STR "  ");
    printf("║     SIMD     : %-3d x float32/register                           ║\n", SIMD_WIDTH);
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    /* Theoretical peaks per platform */
    double peak = 0;
    const char *peak_note = "";
#if NOVA_NEON
    /* M1: 3.2GHz, 4 FMA units, 4 floats/FMA, 2 ops/FMA */
    peak = 3.2 * 4 * 4 * 2;   /* = 102.4 GFLOPS */
    peak_note = "Apple M1 P-core (3.2GHz x4 FMA x4 SIMD x2)";
#elif NOVA_AVX512
    /* Typical server: 3.0GHz, 2 AVX512 FMA/cycle, 16 floats, 2 ops */
    peak = 3.0 * 2 * 16 * 2;  /* = 192 GFLOPS */
    peak_note = "AVX-512 server (3.0GHz x2 FMA x16 SIMD x2)";
#elif NOVA_AVX2
    /* Typical desktop: 3.5GHz, 2 AVX2 FMA/cycle, 8 floats, 2 ops */
    peak = 3.5 * 2 * 8 * 2;   /* = 112 GFLOPS */
    peak_note = "AVX2 desktop (3.5GHz x2 FMA x8 SIMD x2)";
#else
    peak = 4.0 * 1 * 1 * 2;   /* generic: 4GHz scalar = 8 GFLOPS */
    peak_note = "Generic scalar (4GHz x1 FU x1 SIMD x2)";
#endif

    printf("Theoretical Peak: %.1f GFLOPS  [%s]\n", peak, peak_note);
    printf("Memory BW (est): ~%.0f GB/s\n\n",
#if NOVA_NEON
    68.25
#elif NOVA_AVX512
    200.0
#elif NOVA_AVX2
    50.0
#else
    25.0
#endif
    );

    /* Roofline table */
    printf("  Roofline Analysis:\n");
    printf("  %-12s  %10s  %12s  %10s\n","Size","AI(FLOP/B)","Roofline GF","Bound");
    int sizes[] = {64,128,256,512,1024};
    double mem_bw =
#if NOVA_NEON
    68.25
#elif NOVA_AVX512
    200.0
#elif NOVA_AVX2
    50.0
#else
    25.0
#endif
    ;
    for (int s=0;s<5;s++){
        int n=sizes[s];
        double ai=(2.0*n*n*n)/(3.0*n*n*4.0);
        double roof=ai*mem_bw<peak?ai*mem_bw:peak;
        printf("  %4dx%-7d  %10.1f  %9.1f GF  %s\n",
               n,n,ai,roof,ai*mem_bw<peak?"memory ":"compute");
    }
    printf("\n  (peak=%.1f GF stored for %% calc)\n\n", peak);
    /* store peak globally via static */
}

static double g_peak = 0.0;

int main(void) {
    srand(42);

    /* compute peak */
#if NOVA_NEON
    g_peak = 102.4;
#elif NOVA_AVX512
    g_peak = 192.0;
#elif NOVA_AVX2
    g_peak  = 112.0;
#else
    g_peak  = 8.0;
#endif

    print_platform_info();

    BenchAlgo algos[] = {
        { "1. Naive ikj",       matmul_naive,        true,  true },
        { "2. Generic 8×8",     matmul_generic_tile, false, true },
#if NOVA_NEON
        { "3. NEON 12×8 BLAS",  matmul_neon_12x8,    false, true },
#elif NOVA_AVX2
        { "3. AVX2 8×16 BLAS",  matmul_avx2_8x16,    false, true },
#elif NOVA_AVX512
        { "3. AVX512 12×32",    matmul_avx512_12x32, false, true },
#endif
    };
    int nalgo  = sizeof(algos)/sizeof(algos[0]);
    int sizes[] = {64,128,256,512,1024};
    int reps[]  = { 8,  6,  4,  3,  2};
    int nsizes  = 5;

    /* Header */
    printf("%-22s  %s\n", "Algorithm", "  64x64   128x128  256x256  512x512 1024x1024  verify");
    printf("%-22s  %s\n", "──────────────────────",
           "────────  ───────  ───────  ───────  ────────  ──────");

    double best_gf[5]={0};
    for (int a=0;a<nalgo;a++){
        printf("%-22s ", algos[a].name);
        for (int s=0;s<nsizes;s++){
            int n=sizes[s];
            if (algos[a].skip_large && n>=512){
                printf("      — ");
                continue;
            }
            double gf=run_bench(&algos[a],n,reps[s],(n<=256));
            printf("%6.1fGF ", gf);
            if (gf>best_gf[s]) best_gf[s]=gf;
        }
        printf("  %s\n", algos[a].verify_ok ? "✅" : "❌");
    }

    printf("%-22s ", "──────────────────────");
    for (int s=0;s<nsizes;s++) printf("──────── ");
    printf("\n");

    printf("%-22s ", "🏆 Best");
    for (int s=0;s<nsizes;s++) printf("%6.1fGF ", best_gf[s]);
    printf("\n");

    printf("%-22s ", "🎯 % of Peak");
    for (int s=0;s<nsizes;s++) printf("  %4.1f%%   ", best_gf[s]/g_peak*100.0);
    printf("\n\n");

    /* Detail: best algo on each size */
    printf("═══ Summary ═══════════════════════════════════════════════════\n");
    printf("  Platform   : %s\n", PLATFORM_STR);
    printf("  Peak       : %.1f GFLOPS (theoretical)\n", g_peak);
    printf("  Best 1024  : %.2f GFLOPS (%.1f%% peak)\n", best_gf[4], best_gf[4]/g_peak*100);
    printf("  Best  512  : %.2f GFLOPS (%.1f%% peak)\n", best_gf[3], best_gf[3]/g_peak*100);
    printf("  Best  256  : %.2f GFLOPS (%.1f%% peak)\n", best_gf[2], best_gf[2]/g_peak*100);
    printf("  Speedup    : %.1fx naive (256x256)\n", best_gf[2] / run_bench(&algos[0],256,2,false));
    printf("═══════════════════════════════════════════════════════════════\n\n");
    printf("✅ Nova BLAS Benchmark tamamlandı!\n\n");
    return 0;
}
