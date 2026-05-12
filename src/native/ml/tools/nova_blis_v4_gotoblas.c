/**
 * Nova BLIS GEBP v4 — GotoBLAS loop order (CRITICAL FIX)
 * ═══════════════════════════════════════════════════════════════
 * v3 bug: pack_B called n/MR times per jj-block (85x overhead!)
 * v4 fix: GotoBLAS order:
 *   for jj → pack_B ONCE → for ii → pack_A → kernel
 * This way:
 *   pack_B: n/NR calls  (was n/NR × n/MR calls!)
 *   pack_A: n/MR calls per jj (amortized, A stays in L2)
 *
 * Additional: L2-blocking with KC (K-cache block)
 *   KC=256: A-panel (MR×KC) fits in L1, B-panel (KC×NR) fits in L1
 * ═══════════════════════════════════════════════════════════════
 */
#include <arm_neon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

static inline double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec+ts.tv_nsec*1e-9;
}
static float *amalloc(size_t bytes){
    float *p=(float*)aligned_alloc(128,bytes);
    if(!p){fprintf(stderr,"OOM\n");exit(1);}
    return p;
}

#define MR   12
#define NR    8
#define KC  256   /* K-cache block: A[MR×KC]+B[KC×NR] fit L1 (24KB) */
#define MC  120   /* M-cache block: multiple of MR, fits L2           */
#define NC  512   /* N-cache block: multiple of NR                    */

/* ── Pack B panel: Bp[k*NR+j] = B[k*n+j0+j], k=0..kc-1 ── */
static void pack_B(const float *B, float *Bp, int n, int j0, int nr, int k0, int kc){
    for(int k=0;k<kc;k++){
        const float *src=B+(k0+k)*n+j0;
        float *dst=Bp+k*NR;
        int j=0;
        for(;j<nr;j++) dst[j]=(j0+j<n)?src[j]:0.f;
        for(;j<NR;j++) dst[j]=0.f;
    }
}

/* ── Pack A panel: Ap[k*MR+r] = A[(i0+r)*n+k0+k], k=0..kc-1 ── */
static void pack_A(const float *A, float *Ap, int n, int i0, int mr, int k0, int kc){
    for(int k=0;k<kc;k++){
        float *dst=Ap+k*MR;
        for(int r=0;r<mr;r++) dst[r]=(i0+r<n)?A[(i0+r)*n+k0+k]:0.f;
        for(int r=mr;r<MR;r++) dst[r]=0.f;
    }
}

/* ── GEBP 12×8 micro-kernel (same as v3, proven correct) ── */
static inline void __attribute__((always_inline))
gebp_12x8(const float *__restrict__ Ap,
          const float *__restrict__ Bp,
          float       *__restrict__ C,
          int n, int i0, int j0, int mr, int nr, int kc)
{
    float32x4_t c00=vdupq_n_f32(0),c01=vdupq_n_f32(0);
    float32x4_t c10=vdupq_n_f32(0),c11=vdupq_n_f32(0);
    float32x4_t c20=vdupq_n_f32(0),c21=vdupq_n_f32(0);
    float32x4_t c30=vdupq_n_f32(0),c31=vdupq_n_f32(0);
    float32x4_t c40=vdupq_n_f32(0),c41=vdupq_n_f32(0);
    float32x4_t c50=vdupq_n_f32(0),c51=vdupq_n_f32(0);
    float32x4_t c60=vdupq_n_f32(0),c61=vdupq_n_f32(0);
    float32x4_t c70=vdupq_n_f32(0),c71=vdupq_n_f32(0);
    float32x4_t c80=vdupq_n_f32(0),c81=vdupq_n_f32(0);
    float32x4_t c90=vdupq_n_f32(0),c91=vdupq_n_f32(0);
    float32x4_t ca0=vdupq_n_f32(0),ca1=vdupq_n_f32(0);
    float32x4_t cb0=vdupq_n_f32(0),cb1=vdupq_n_f32(0);

    const float *ap=Ap, *bp=Bp;
    int k=0;

#define KSTEP(AP,BP) { \
    float32x4_t a0=vld1q_f32((AP)+0); \
    float32x4_t a1=vld1q_f32((AP)+4); \
    float32x4_t a2=vld1q_f32((AP)+8); \
    float32x4_t b0=vld1q_f32((BP)+0); \
    float32x4_t b1=vld1q_f32((BP)+4); \
    c00=vfmaq_laneq_f32(c00,b0,a0,0); c01=vfmaq_laneq_f32(c01,b1,a0,0); \
    c10=vfmaq_laneq_f32(c10,b0,a0,1); c11=vfmaq_laneq_f32(c11,b1,a0,1); \
    c20=vfmaq_laneq_f32(c20,b0,a0,2); c21=vfmaq_laneq_f32(c21,b1,a0,2); \
    c30=vfmaq_laneq_f32(c30,b0,a0,3); c31=vfmaq_laneq_f32(c31,b1,a0,3); \
    c40=vfmaq_laneq_f32(c40,b0,a1,0); c41=vfmaq_laneq_f32(c41,b1,a1,0); \
    c50=vfmaq_laneq_f32(c50,b0,a1,1); c51=vfmaq_laneq_f32(c51,b1,a1,1); \
    c60=vfmaq_laneq_f32(c60,b0,a1,2); c61=vfmaq_laneq_f32(c61,b1,a1,2); \
    c70=vfmaq_laneq_f32(c70,b0,a1,3); c71=vfmaq_laneq_f32(c71,b1,a1,3); \
    c80=vfmaq_laneq_f32(c80,b0,a2,0); c81=vfmaq_laneq_f32(c81,b1,a2,0); \
    c90=vfmaq_laneq_f32(c90,b0,a2,1); c91=vfmaq_laneq_f32(c91,b1,a2,1); \
    ca0=vfmaq_laneq_f32(ca0,b0,a2,2); ca1=vfmaq_laneq_f32(ca1,b1,a2,2); \
    cb0=vfmaq_laneq_f32(cb0,b0,a2,3); cb1=vfmaq_laneq_f32(cb1,b1,a2,3); \
}
    for(;k<=kc-8;k+=8,ap+=8*MR,bp+=8*NR){
        __builtin_prefetch(ap+16*MR,0,1);
        __builtin_prefetch(bp+ 8*NR,0,1);
        KSTEP(ap+0*MR,bp+0*NR) KSTEP(ap+1*MR,bp+1*NR)
        KSTEP(ap+2*MR,bp+2*NR) KSTEP(ap+3*MR,bp+3*NR)
        KSTEP(ap+4*MR,bp+4*NR) KSTEP(ap+5*MR,bp+5*NR)
        KSTEP(ap+6*MR,bp+6*NR) KSTEP(ap+7*MR,bp+7*NR)
    }
    for(;k<=kc-4;k+=4,ap+=4*MR,bp+=4*NR){
        KSTEP(ap+0*MR,bp+0*NR) KSTEP(ap+1*MR,bp+1*NR)
        KSTEP(ap+2*MR,bp+2*NR) KSTEP(ap+3*MR,bp+3*NR)
    }
    for(;k<kc;k++,ap+=MR,bp+=NR){ KSTEP(ap,bp) }
#undef KSTEP

    /* Accumulate into C (beta=1: C += tile) */
#define ACCUM(r,v0,v1) \
    if((r)<mr){ \
        float *crow=C+(i0+(r))*n+j0; \
        if(nr==NR){ \
            float32x4_t old0=vld1q_f32(crow+0); \
            float32x4_t old1=vld1q_f32(crow+4); \
            vst1q_f32(crow+0,vaddq_f32(old0,(v0))); \
            vst1q_f32(crow+4,vaddq_f32(old1,(v1))); \
        } else { \
            float _t[8]; vst1q_f32(_t,(v0)); vst1q_f32(_t+4,(v1)); \
            for(int _j=0;_j<nr;_j++) crow[_j]+=_t[_j]; \
        } \
    }
    ACCUM( 0,c00,c01) ACCUM( 1,c10,c11) ACCUM( 2,c20,c21) ACCUM( 3,c30,c31)
    ACCUM( 4,c40,c41) ACCUM( 5,c50,c51) ACCUM( 6,c60,c61) ACCUM( 7,c70,c71)
    ACCUM( 8,c80,c81) ACCUM( 9,c90,c91) ACCUM(10,ca0,ca1) ACCUM(11,cb0,cb1)
#undef ACCUM
}

/* ══════════════════════════════════════════════════════════════
 * GotoBLAS 5-loop GEMM
 * Loop 5: jc (N-block, NC)
 * Loop 4: kc (K-block, KC) → pack B panel ONCE per (jc,kc)
 * Loop 3: ic (M-block, MC)
 * Loop 2: jr (jc-inner, NR tiles) ← already in kc-panel
 * Loop 1: ir (ic-inner, MR tiles) → pack A + kernel
 * ══════════════════════════════════════════════════════════════ */
static void matmul_goto(const float *A, const float *B, float *C, int n){
    memset(C, 0, (size_t)n*n*sizeof(float));

    float *Bp = amalloc((size_t)KC * NC * sizeof(float));
    float *Ap = amalloc((size_t)MC * KC * sizeof(float));

    for(int jc=0; jc<n; jc+=NC){
      int nc=(jc+NC<=n)?NC:n-jc;

      for(int kk=0; kk<n; kk+=KC){
        int kc=(kk+KC<=n)?KC:n-kk;

        /* Loop 4: pack entire B panel [kc × nc] ONCE */
        for(int jr=0; jr<nc; jr+=NR){
            int nr=(jr+NR<=nc)?NR:nc-jr;
            pack_B(B, Bp+(jr/NR)*kc*NR, n, jc+jr, nr, kk, kc);
        }

        for(int ic=0; ic<n; ic+=MC){
          int mc=(ic+MC<=n)?MC:n-ic;

          /* Loop 3: pack A panel [mc × kc] ONCE per ic-block */
          for(int ir=0; ir<mc; ir+=MR){
              int mr=(ir+MR<=mc)?MR:mc-ir;
              pack_A(A, Ap+(ir/MR)*kc*MR, n, ic+ir, mr, kk, kc);
          }

          /* Loop 2+1: jr × ir micro-kernels */
          for(int jr=0; jr<nc; jr+=NR){
            int nr=(jr+NR<=nc)?NR:nc-jr;
            const float *Bp_panel = Bp + (jr/NR)*kc*NR;

            for(int ir=0; ir<mc; ir+=MR){
              int mr=(ir+MR<=mc)?MR:mc-ir;
              const float *Ap_panel = Ap + (ir/MR)*kc*MR;

              gebp_12x8(Ap_panel, Bp_panel, C, n,
                        ic+ir, jc+jr, mr, nr, kc);
            }
          }
        }
      }
    }
    free(Bp); free(Ap);
}

/* ── Reference ── */
static void matmul_ref(const float *A,const float *B,float *C,int n){
    memset(C,0,(size_t)n*n*sizeof(float));
    for(int i=0;i<n;i++) for(int k=0;k<n;k++){
        float a=A[i*n+k];
        for(int j=0;j<n;j++) C[i*n+j]+=a*B[k*n+j];
    }
}
static bool verify(const float *ref,const float *got,int n){
    int errs=0;
    for(int i=0;i<n*n;i++)
        if(fabsf(ref[i]-got[i])>1e-2f*(fabsf(ref[i])+1.f)){
            if(errs<3) printf("  MISMATCH [%d,%d]: ref=%.4f got=%.4f\n",i/n,i%n,ref[i],got[i]);
            errs++;
        }
    return errs==0;
}

int main(void){
    srand(42);
    double peak=102.4;

    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Nova BLIS GEBP v4 — GotoBLAS 5-Loop + KC/MC/NC Blocking    ║\n");
    printf("║  pack_B: once per (jc,kc)  pack_A: once per (ic,kc)         ║\n");
    printf("║  MR=%-2d NR=%-2d KC=%-3d MC=%-3d NC=%-3d                        ║\n",MR,NR,KC,MC,NC);
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    int sizes[]={64,128,256,512,1024};
    int reps[] ={10,  8,  5,  4,  3};

    printf("  %-16s","Size");
    for(int s=0;s<5;s++) printf("  %4dx%-4d",sizes[s],sizes[s]);
    printf("\n  %-16s","────────────────");
    for(int s=0;s<5;s++) printf("  ─────────");
    printf("\n");

    double best[5]={0};
    printf("  %-16s","GotoBLAS v4");
    bool all_ok=true;
    for(int s=0;s<5;s++){
        int n=sizes[s];
        float *A=amalloc((size_t)n*n*sizeof(float));
        float *B=amalloc((size_t)n*n*sizeof(float));
        float *C=amalloc((size_t)n*n*sizeof(float));
        float *Cref=amalloc((size_t)n*n*sizeof(float));
        for(int i=0;i<n*n;i++){A[i]=(float)(rand()&0xFF)/128.f-1.f;B[i]=(float)(rand()&0xFF)/128.f-1.f;}

        /* verify */
        memset(C,0,(size_t)n*n*sizeof(float));
        matmul_goto(A,B,C,n);
        if(n<=256){ matmul_ref(A,B,Cref,n); if(!verify(Cref,C,n)) all_ok=false; }

        double best_t=1e18;
        for(int r=0;r<reps[s];r++){
            memset(C,0,(size_t)n*n*sizeof(float));
            double t0=now_sec(); matmul_goto(A,B,C,n);
            double dt=now_sec()-t0; if(dt<best_t)best_t=dt;
        }
        double gf=2.0*n*n*n/best_t/1e9;
        best[s]=gf;
        printf("  %6.1fGF  ",gf);
        free(A);free(B);free(C);free(Cref);
    }
    printf("  %s\n\n",all_ok?"✅":"❌");

    printf("  %-16s","% of Peak");
    for(int s=0;s<5;s++) printf("  %7.1f%%  ",best[s]/peak*100);
    printf("\n\n");

    printf("  ┌─────────────────────────────────────────────────────┐\n");
    printf("  │  Peak  : %6.1f GFLOPS (Apple M1 single P-core)     │\n",peak);
    printf("  │  Best 1024×1024: %6.2f GF  → %5.1f%% peak          │\n",best[4],best[4]/peak*100);
    printf("  │  Best  512×512:  %6.2f GF  → %5.1f%% peak          │\n",best[3],best[3]/peak*100);
    printf("  │  Best  256×256:  %6.2f GF  → %5.1f%% peak          │\n",best[2],best[2]/peak*100);
    printf("  └─────────────────────────────────────────────────────┘\n\n");
    return 0;
}
