#include "compute/nova_kernels.h"
#include "runtime/nova_context.h"
#include <arm_fp16.h>
#include <arm_neon.h>
#include <dispatch/dispatch.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// NOVA 2.2 MATH: Vectorized Fast Math Extensions
// ═══════════════════════════════════════════════════════════════════════════

// Approximates exp(x) in FP16 for 8 lanes simultaneously
// Optimized for attention score range roughly [-10, 0]
static inline float16x8_t nova_vfast_exp_f16(float16x8_t x) {
  // Convert to f32 for the core math to keep the code simple and stable,
  // then convert back to f16. This is still much cheaper than full expf
  // per lane and keeps the vectorized pipeline structure.
  float32x4_t x_lo = vcvt_f32_f16(vget_low_f16(x));
  float32x4_t x_hi = vcvt_f32_f16(vget_high_f16(x));

  const float32x4_t inv_ln2 = vdupq_n_f32(1.4426950408f);

  // y = x / ln(2)
  float32x4_t y_lo = vmulq_f32(x_lo, inv_ln2);
  float32x4_t y_hi = vmulq_f32(x_hi, inv_ln2);

  // Use exp2f on each lane. For the typical attention score range this is
  // reasonably accurate and much faster than scalar expf on the original f32
  // inputs everywhere in the kernel.
  float y_lo_arr[4], y_hi_arr[4];
  vst1q_f32(y_lo_arr, y_lo);
  vst1q_f32(y_hi_arr, y_hi);

  for (int i = 0; i < 4; ++i) {
    y_lo_arr[i] = exp2f(y_lo_arr[i]);
    y_hi_arr[i] = exp2f(y_hi_arr[i]);
  }

  float32x4_t e_lo = vld1q_f32(y_lo_arr);
  float32x4_t e_hi = vld1q_f32(y_hi_arr);

  float16x4_t e_lo_f16 = vcvt_f16_f32(e_lo);
  float16x4_t e_hi_f16 = vcvt_f16_f32(e_hi);

  float16x8_t result;
  result = vcombine_f16(e_lo_f16, e_hi_f16);
  return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// MEGAFUSION: RMSNorm + Quantized MatMul (Simulated)
// ═══════════════════════════════════════════════════════════════════════════

void nova_kernel_rmsnorm_f32(NovaTensor *x, NovaTensor *w, float eps) {
  int64_t M = x->shape[0], D = x->shape[1];
  float *xd = (float *)x->data, *wd = (float *)w->data;

  dispatch_apply(M, dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                 ^(size_t i) {
                   float *row = xd + i * D;
                   float ss = 0;
                   for (int j = 0; j < D; j++)
                     ss += row[j] * row[j];
                   float inv_rms = 1.0f / sqrtf(ss / D + eps);
                   for (int j = 0; j < D; j++)
                     row[j] = row[j] * inv_rms * wd[j];
                 });
}

// ═══════════════════════════════════════════════════════════════════════════
// NOVA 2.2 MATMUL: 12x16 Register Tiling (24 Accumulators)
// ═══════════════════════════════════════════════════════════════════════════

static void matmul_12x16_hyper_tile(const float16_t *A,
                                    const float16_t *B_packed, float16_t *C,
                                    int K, int N, int i_start, int i_end) {
  for (int i = i_start; i + 12 <= i_end; i += 12) {
    for (int j = 0; j < N; j += 16) {
      // Use 24 registers for accumulation (12 rows x 2 vectors of 8 elements)
      float16x8_t c00 = vdupq_n_f16(0), c01 = vdupq_n_f16(0);
      float16x8_t c10 = vdupq_n_f16(0), c11 = vdupq_n_f16(0);
      float16x8_t c20 = vdupq_n_f16(0), c21 = vdupq_n_f16(0);
      float16x8_t c30 = vdupq_n_f16(0), c31 = vdupq_n_f16(0);
      float16x8_t c40 = vdupq_n_f16(0), c41 = vdupq_n_f16(0);
      float16x8_t c50 = vdupq_n_f16(0), c51 = vdupq_n_f16(0);
      float16x8_t c60 = vdupq_n_f16(0), c61 = vdupq_n_f16(0);
      float16x8_t c70 = vdupq_n_f16(0), c71 = vdupq_n_f16(0);
      float16x8_t c80 = vdupq_n_f16(0), c81 = vdupq_n_f16(0);
      float16x8_t c90 = vdupq_n_f16(0), c91 = vdupq_n_f16(0);
      float16x8_t ca0 = vdupq_n_f16(0), ca1 = vdupq_n_f16(0);
      float16x8_t cb0 = vdupq_n_f16(0), cb1 = vdupq_n_f16(0);

      const float16_t *bp = B_packed + j * K;
      for (int k = 0; k < K; k++) {
        float16x8_t b0 = vld1q_f16(bp + k * 16 + 0);
        float16x8_t b1 = vld1q_f16(bp + k * 16 + 8);

        c00 = vfmaq_n_f16(c00, b0, A[(i + 0) * K + k]);
        c01 = vfmaq_n_f16(c01, b1, A[(i + 0) * K + k]);
        c10 = vfmaq_n_f16(c10, b0, A[(i + 1) * K + k]);
        c11 = vfmaq_n_f16(c11, b1, A[(i + 1) * K + k]);
        c20 = vfmaq_n_f16(c20, b0, A[(i + 2) * K + k]);
        c21 = vfmaq_n_f16(c21, b1, A[(i + 2) * K + k]);
        c30 = vfmaq_n_f16(c30, b0, A[(i + 3) * K + k]);
        c31 = vfmaq_n_f16(c31, b1, A[(i + 3) * K + k]);
        c40 = vfmaq_n_f16(c40, b0, A[(i + 4) * K + k]);
        c41 = vfmaq_n_f16(c41, b1, A[(i + 4) * K + k]);
        c50 = vfmaq_n_f16(c50, b0, A[(i + 5) * K + k]);
        c51 = vfmaq_n_f16(c51, b1, A[(i + 5) * K + k]);
        c60 = vfmaq_n_f16(c60, b0, A[(i + 6) * K + k]);
        c61 = vfmaq_n_f16(c61, b1, A[(i + 6) * K + k]);
        c70 = vfmaq_n_f16(c70, b0, A[(i + 7) * K + k]);
        c71 = vfmaq_n_f16(c71, b1, A[(i + 7) * K + k]);
        c80 = vfmaq_n_f16(c80, b0, A[(i + 8) * K + k]);
        c81 = vfmaq_n_f16(c81, b1, A[(i + 8) * K + k]);
        c90 = vfmaq_n_f16(c90, b0, A[(i + 9) * K + k]);
        c91 = vfmaq_n_f16(c91, b1, A[(i + 9) * K + k]);
        ca0 = vfmaq_n_f16(ca0, b0, A[(i + 10) * K + k]);
        ca1 = vfmaq_n_f16(ca1, b1, A[(i + 10) * K + k]);
        cb0 = vfmaq_n_f16(cb0, b0, A[(i + 11) * K + k]);
        cb1 = vfmaq_n_f16(cb1, b1, A[(i + 11) * K + k]);
      }
      vst1q_f16(C + (i + 0) * N + j, c00);
      vst1q_f16(C + (i + 0) * N + j + 8, c01);
      vst1q_f16(C + (i + 1) * N + j, c10);
      vst1q_f16(C + (i + 1) * N + j + 8, c11);
      vst1q_f16(C + (i + 2) * N + j, c20);
      vst1q_f16(C + (i + 2) * N + j + 8, c21);
      vst1q_f16(C + (i + 3) * N + j, c30);
      vst1q_f16(C + (i + 3) * N + j + 8, c31);
      vst1q_f16(C + (i + 4) * N + j, c40);
      vst1q_f16(C + (i + 4) * N + j + 8, c41);
      vst1q_f16(C + (i + 5) * N + j, c50);
      vst1q_f16(C + (i + 5) * N + j + 8, c51);
      vst1q_f16(C + (i + 6) * N + j, c60);
      vst1q_f16(C + (i + 6) * N + j + 8, c61);
      vst1q_f16(C + (i + 7) * N + j, c70);
      vst1q_f16(C + (i + 7) * N + j + 8, c71);
      vst1q_f16(C + (i + 8) * N + j, c80);
      vst1q_f16(C + (i + 8) * N + j + 8, c81);
      vst1q_f16(C + (i + 9) * N + j, c90);
      vst1q_f16(C + (i + 9) * N + j + 8, c91);
      vst1q_f16(C + (i + 10) * N + j, ca0);
      vst1q_f16(C + (i + 10) * N + j + 8, ca1);
      vst1q_f16(C + (i + 11) * N + j, cb0);
      vst1q_f16(C + (i + 11) * N + j + 8, cb1);
    }
  }
}

void nova_kernel_matmul_add_relu_f32(NovaTensor *A, NovaTensor *B,
                                     NovaTensor *Bias, NovaTensor *Out) {
  if (!A || !B || !Bias || !Out)
    return;
  int M = (int)A->shape[0], K = (int)A->shape[1], N = (int)B->shape[1];

  float16_t *a16 = malloc(M * K * sizeof(float16_t));
  float16_t *bp = malloc(K * N * sizeof(float16_t));
  float16_t *c16 = malloc(M * N * sizeof(float16_t));

  // Convert A to f16
  float *ad = (float *)A->data, *bd = (float *)B->data;
  for (int n = 0; n < M * K; n++)
    a16[n] = (float16_t)ad[n];

  // Pack B into 16-element wide panels
  for (int j = 0; j < N; j += 16) {
    for (int k = 0; k < K; k++) {
      bp[j * K + k * 16 + 0] = (float16_t)bd[k * N + j + 0];
      // ... (unroll for performance)
      for (int r = 1; r < 16; r++)
        bp[j * K + k * 16 + r] = (float16_t)bd[k * N + j + r];
    }
  }

  dispatch_apply((M + 11) / 12,
                 dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                 ^(size_t tid) {
                   int i_start = tid * 12;
                   int i_end = (i_start + 12 > M) ? M : i_start + 12;
                   matmul_12x16_hyper_tile(a16, bp, c16, K, N, i_start, i_end);
                 });

  float *od = (float *)Out->data, *bid = (float *)Bias->data;
  for (int n = 0; n < M * N; n++) {
    float v = (float)c16[n] + bid[n % N];
    od[n] = v > 0 ? v : 0;
  }

  free(a16);
  free(bp);
  free(c16);
}

// ═══════════════════════════════════════════════════════════════════════════
// NOVA 2.2 FLASH ATTENTION (STABILIZED FOR D=128)
// ═══════════════════════════════════════════════════════════════════════════

static void head_fp16_kernel(const float *Q, const float *K, const float *V,
                             float *O, int L, int D) {
  float scale = 1.0f / sqrtf((float)D);
  float16_t *q16 = malloc(L * D * sizeof(float16_t));
  float16_t *k16 = malloc(L * D * sizeof(float16_t));
  float16_t *v16 = malloc(L * D * sizeof(float16_t));
  float16_t *o16 = malloc(L * D * sizeof(float16_t));

  for (int n = 0; n < L * D; n++) {
    q16[n] = (float16_t)Q[n];
    k16[n] = (float16_t)K[n];
    v16[n] = (float16_t)V[n];
  }

  int num_vecs = D / 8;
  for (int i = 0; i < L; i++) {
    const float16_t *qi = q16 + i * D;
    float16_t *oi = o16 + i * D;
    float16x8_t o_acc[16];
    float16x8_t v_qi[16];
    for (int r = 0; r < num_vecs; r++) {
      v_qi[r] = vld1q_f16(qi + r * 8);
      o_acc[r] = vdupq_n_f16(0.0f);
    }

    float m = -1e30f, l = 0.0f;
    for (int j = 0; j < L; j++) {
      const float16_t *kj = k16 + j * D;
      float16x8_t s_vec = vmulq_f16(v_qi[0], vld1q_f16(kj));
      for (int r = 1; r < num_vecs; r++)
        s_vec = vfmaq_f16(s_vec, v_qi[r], vld1q_f16(kj + r * 8));

      float16x4_t h = vadd_f16(vget_low_f16(s_vec), vget_high_f16(s_vec));
      float score = (float)(vget_lane_f16(h, 0) + vget_lane_f16(h, 1) +
                            vget_lane_f16(h, 2) + vget_lane_f16(h, 3)) *
                    scale;

      float m_old = m;
      if (score > m)
        m = score;
      float alpha = expf(m_old - m), weight = expf(score - m);
      float16x8_t va = vdupq_n_f16((float16_t)alpha),
                  vw = vdupq_n_f16((float16_t)weight);
      const float16_t *vj = v16 + j * D;
      for (int r = 0; r < num_vecs; r++)
        o_acc[r] =
            vfmaq_f16(vmulq_f16(o_acc[r], va), vld1q_f16(vj + r * 8), vw);
      l = l * alpha + weight;
    }
    float16x8_t inv_lv = vdupq_n_f16((float16_t)(1.0f / l));
    for (int r = 0; r < num_vecs; r++)
      vst1q_f16(oi + r * 8, vmulq_f16(o_acc[r], inv_lv));
  }
  for (int n = 0; n < L * D; n++)
    O[n] = (float)o16[n];
  free(q16);
  free(k16);
  free(v16);
  free(o16);
}

void nova_kernel_attention(NovaTensor *Q, NovaTensor *K, NovaTensor *V,
                           NovaTensor *Out) {
  int B = Q->shape[0], H = Q->shape[1], L = Q->shape[2], D = Q->shape[3];
  dispatch_apply(B * H,
                 dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                 ^(size_t idx) {
                   int off = idx * L * D;
                   head_fp16_kernel(
                       (float *)Q->data + off, (float *)K->data + off,
                       (float *)V->data + off, (float *)Out->data + off, L, D);
                 });
}

void nova_kernel_matmul_int8_f16(NovaTensor *A, NovaTensor *W_int8,
                                 NovaTensor *Out_f16) {
  if (!A || !W_int8 || !Out_f16)
    return;
  int M = (int)A->shape[0], K = (int)A->shape[1], N = (int)W_int8->shape[1];
  float *ad = (float *)A->data;
  int8_t *wd = (int8_t *)W_int8->data;
  float *od = (float *)Out_f16->data;

  float16_t *a16 = malloc((size_t)M * K * sizeof(float16_t));
  for (int n = 0; n < M * K; n++)
    a16[n] = (float16_t)ad[n];

  dispatch_queue_t queue =
      dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);

  // Use 4-row tiling if M is large enough, otherwise single-row
  if (M >= 4) {
    int m_blocks = M / 4;
    dispatch_apply(m_blocks, queue, ^(size_t tid) {
      int i0 = (int)tid * 4;
      const float16_t *a0 = a16 + (size_t)i0 * K;
      const float16_t *a1 = a0 + K;
      const float16_t *a2 = a1 + K;
      const float16_t *a3 = a2 + K;

      for (int j = 0; j < N; j++) {
        const int8_t *wj = wd + (int64_t)j * K;
        float16x8_t acc0 = vdupq_n_f16(0.0f);
        float16x8_t acc1 = vdupq_n_f16(0.0f);
        float16x8_t acc2 = vdupq_n_f16(0.0f);
        float16x8_t acc3 = vdupq_n_f16(0.0f);

        for (int k = 0; k < K; k += 16) {
          // Load 16 weights (INT8) once
          int8x16_t w_vec = vld1q_s8(wj + k);
          int16x8_t w8_lo = vmovl_s8(vget_low_s8(w_vec));
          int16x8_t w8_hi = vmovl_s8(vget_high_s8(w_vec));
          float16x8_t wf_lo = vcvtq_f16_s16(w8_lo);
          float16x8_t wf_hi = vcvtq_f16_s16(w8_hi);

          // Row 0
          acc0 = vfmaq_f16(acc0, vld1q_f16(a0 + k), wf_lo);
          acc0 = vfmaq_f16(acc0, vld1q_f16(a0 + k + 8), wf_hi);
          // Row 1
          acc1 = vfmaq_f16(acc1, vld1q_f16(a1 + k), wf_lo);
          acc1 = vfmaq_f16(acc1, vld1q_f16(a1 + k + 8), wf_hi);
          // Row 2
          acc2 = vfmaq_f16(acc2, vld1q_f16(a2 + k), wf_lo);
          acc2 = vfmaq_f16(acc2, vld1q_f16(a2 + k + 8), wf_hi);
          // Row 3
          acc3 = vfmaq_f16(acc3, vld1q_f16(a3 + k), wf_lo);
          acc3 = vfmaq_f16(acc3, vld1q_f16(a3 + k + 8), wf_hi);
        }

        // Horizontal reduction & store (Manually expanded for C compliance)
        float16x4_t h0 = vadd_f16(vget_low_f16(acc0), vget_high_f16(acc0));
        od[(size_t)i0 * N + j] =
            (float)(vget_lane_f16(h0, 0) + vget_lane_f16(h0, 1) +
                    vget_lane_f16(h0, 2) + vget_lane_f16(h0, 3));
        float16x4_t h1 = vadd_f16(vget_low_f16(acc1), vget_high_f16(acc1));
        od[(size_t)(i0 + 1) * N + j] =
            (float)(vget_lane_f16(h1, 0) + vget_lane_f16(h1, 1) +
                    vget_lane_f16(h1, 2) + vget_lane_f16(h1, 3));
        float16x4_t h2 = vadd_f16(vget_low_f16(acc2), vget_high_f16(acc2));
        od[(size_t)(i0 + 2) * N + j] =
            (float)(vget_lane_f16(h2, 0) + vget_lane_f16(h2, 1) +
                    vget_lane_f16(h2, 2) + vget_lane_f16(h2, 3));
        float16x4_t h3 = vadd_f16(vget_low_f16(acc3), vget_high_f16(acc3));
        od[(size_t)(i0 + 3) * N + j] =
            (float)(vget_lane_f16(h3, 0) + vget_lane_f16(h3, 1) +
                    vget_lane_f16(h3, 2) + vget_lane_f16(h3, 3));
      }
    });

    // Remaining rows (M % 4)
    for (int i = (M / 4) * 4; i < M; i++) {
      const float16_t *ai = a16 + (size_t)i * K;
      float *oi = od + (size_t)i * N;
      for (int j = 0; j < N; j++) {
        const int8_t *wj = wd + (int64_t)j * K;
        float16x8_t acc = vdupq_n_f16(0.0f);
        for (int k = 0; k < K; k += 16) {
          int8x16_t w_vec = vld1q_s8(wj + k);
          int16x8_t w8_lo = vmovl_s8(vget_low_s8(w_vec));
          int16x8_t w8_hi = vmovl_s8(vget_high_s8(w_vec));
          float16x8_t wf_lo = vcvtq_f16_s16(w8_lo);
          float16x8_t wf_hi = vcvtq_f16_s16(w8_hi);
          acc = vfmaq_f16(acc, vld1q_f16(ai + k), wf_lo);
          acc = vfmaq_f16(acc, vld1q_f16(ai + k + 8), wf_hi);
        }
        float16x4_t h = vadd_f16(vget_low_f16(acc), vget_high_f16(acc));
        oi[j] = (float)(vget_lane_f16(h, 0) + vget_lane_f16(h, 1) +
                        vget_lane_f16(h, 2) + vget_lane_f16(h, 3));
      }
    }
  } else {
    // Single-row path (optimized for M=1)
    dispatch_apply(M, queue, ^(size_t i) {
      const float16_t *ai = a16 + (size_t)i * K;
      float *oi = od + (size_t)i * N;
      for (int j = 0; j < N; j++) {
        const int8_t *wj = wd + (int64_t)j * K;
        float16x8_t acc = vdupq_n_f16(0.0f);
        for (int k = 0; k < K; k += 16) {
          int8x16_t w_vec = vld1q_s8(wj + k);
          int16x8_t w8_lo = vmovl_s8(vget_low_s8(w_vec));
          int16x8_t w8_hi = vmovl_s8(vget_high_s8(w_vec));
          float16x8_t wf_lo = vcvtq_f16_s16(w8_lo);
          float16x8_t wf_hi = vcvtq_f16_s16(w8_hi);
          acc = vfmaq_f16(acc, vld1q_f16(ai + k), wf_lo);
          acc = vfmaq_f16(acc, vld1q_f16(ai + k + 8), wf_hi);
        }
        float16x4_t h = vadd_f16(vget_low_f16(acc), vget_high_f16(acc));
        oi[j] = (float)(vget_lane_f16(h, 0) + vget_lane_f16(h, 1) +
                        vget_lane_f16(h, 2) + vget_lane_f16(h, 3));
      }
    });
  }

  free(a16);
}

void nova_kernel_matmul(NovaTensor *A, NovaTensor *B, NovaTensor *C) {
  (void)A;
  (void)B;
  (void)C;
}
void nova_kernel_add(NovaTensor *A, NovaTensor *B, NovaTensor *C) {
  (void)A;
  (void)B;
  (void)C;
}
void nova_kernel_mul(NovaTensor *A, NovaTensor *B, NovaTensor *C) {
  (void)A;
  (void)B;
  (void)C;
}
void nova_kernel_relu(NovaTensor *x, NovaTensor *out) {
  (void)x;
  (void)out;
}
void nova_kernel_sigmoid(NovaTensor *x, NovaTensor *out) {
  (void)x;
  (void)out;
}
void nova_kernel_softmax(NovaTensor *x, int dim, NovaTensor *out) {
  (void)x;
  (void)dim;
  (void)out;
}
void nova_kernel_scalar_mul(NovaTensor *A, float s, NovaTensor *C) {
  (void)A;
  (void)s;
  (void)C;
}
void nova_kernel_transpose(NovaTensor *A, NovaTensor *C) {
  (void)A;
  (void)C;
}
void nova_kernel_gelu(NovaTensor *x) { (void)x; }
void nova_kernel_layernorm(NovaTensor *x, NovaTensor *g, NovaTensor *b,
                           float e) {
  (void)x;
  (void)g;
  (void)b;
  (void)e;
}
void nova_kernel_conv2d(NovaTensor *i, NovaTensor *w, NovaTensor *b,
                        NovaTensor *o, int s, int p) {
  (void)i;
  (void)w;
  (void)b;
  (void)o;
  (void)s;
  (void)p;
}
void nova_kernel_matmul_delta_sparse_f32(NovaTensor *Delta, NovaTensor *Weights,
                                         NovaTensor *Out, float threshold) {
  if (!Delta || !Weights || !Out)
    return;
  int M = (int)Delta->shape[0], K = (int)Delta->shape[1],
      N = (int)Weights->shape[1];
  float *dd = (float *)Delta->data;
  float *wd = (float *)Weights->data;
  float *od = (float *)Out->data;

  // For each token/row i
  for (int i = 0; i < M; i++) {
    float *di = dd + i * K;
    float *oi = od + i * N;

    // Scan for significant deltas in the activation row
    for (int k = 0; k < K; k++) {
      float delta_val = di[k];
      if (fabsf(delta_val) > threshold) {
        // Significant change detected! Perform rank-1 update for this k
        float *wk = wd + k * N;

        // Multi-threaded or SIMD inner update
        // We use 4-lane NEON vectors for the update
        float32x4_t dv = vdupq_n_f32(delta_val);
        for (int j = 0; j < N; j += 4) {
          float32x4_t ov = vld1q_f32(oi + j);
          float32x4_t wv = vld1q_f32(wk + j);
          vst1q_f32(oi + j, vfmaq_f32(ov, dv, wv));
        }
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// NOVA HYPER-SYNTHESIS: Vectorized Positional Encoding (NEON)
// ═══════════════════════════════════════════════════════════════════════════

void nova_kernel_synthetic_pe_f32(NovaTensor *Output) {
  if (!Output)
    return;
  int SEQ = (int)Output->shape[0];
  int DIM = (int)Output->shape[1];
  float *od = (float *)Output->data;

  // Pre-calculate inverse frequency constants to avoid powf in inner loop
  float *inv_freq = malloc((DIM / 2) * sizeof(float));
  for (int j = 0; j < DIM / 2; j++) {
    inv_freq[j] = 1.0f / powf(10000.0f, (2.0f * j) / DIM);
  }

  dispatch_apply(
      SEQ, dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
      ^(size_t i) {
        float *oi = od + i * DIM;
        float pos = (float)i;

        // Vectorized PE Synthesis (4 lanes at a time)
        for (int j = 0; j < DIM; j += 4) {
          float32x4_t fv = vld1q_f32(inv_freq + (j / 2));
          float32x4_t angles = vmulq_n_f32(fv, pos);

          // Fast Sin/Cos Approximation (Taylor Series / Minimax)
          // Approx: sin(x) ~ x - x^3/6 + x^5/120
          float32x4_t x2 = vmulq_f32(angles, angles);
          float32x4_t x3 = vmulq_f32(x2, angles);
          float32x4_t x5 = vmulq_f32(x3, x2);

          // Coefficients
          float32x4_t c3 = vdupq_n_f32(-1.0f / 6.0f);
          float32x4_t c5 = vdupq_n_f32(1.0f / 120.0f);

          float32x4_t s_vec = vaddq_f32(
              angles, vaddq_f32(vmulq_f32(x3, c3), vmulq_f32(x5, c5)));

          // Simplified: interleaving sin/cos in output
          // In a real optimized kernel, we'd use a more robust sin/cos vector
          // op
          float temp[4];
          vst1q_f32(temp, s_vec);

          oi[j] = temp[0];
          oi[j + 1] =
              cosf(pos * inv_freq[j / 2]); // Still some scalar for cos for now
          oi[j + 2] = temp[1];
          oi[j + 3] = cosf(pos * inv_freq[j / 2 + 1]);
        }
      });

  free(inv_freq);
}

void nova_kernel_recurrent_pe_f32(NovaTensor *Output) {
  if (!Output)
    return;
  int SEQ = (int)Output->shape[0];
  int DIM = (int)Output->shape[1];
  float *od = (float *)Output->data;

  // Cache check logic
  NovaContext *ctx = (NovaContext *)Output->context;
  if (ctx && ctx->pe_cache.valid && ctx->pe_cache.seq == SEQ &&
      ctx->pe_cache.dim == DIM) {
    memcpy(Output->data, ctx->pe_cache.data, (size_t)SEQ * DIM * sizeof(float));
    return;
  }

  // Recurrence constants for each frequency component
  float *sin_delta = malloc((DIM / 2) * sizeof(float));
  float *cos_delta = malloc((DIM / 2) * sizeof(float));
  for (int j = 0; j < DIM / 2; j++) {
    float freq = 1.0f / powf(10000.0f, (2.0f * j) / DIM);
    sin_delta[j] = sinf(freq);
    cos_delta[j] = cosf(freq);
  }

  dispatch_apply(1, dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0),
                 ^(size_t tid) {
                   (void)tid;
                   float *sin_curr = calloc(DIM / 2, sizeof(float));
                   float *cos_curr = malloc((DIM / 2) * sizeof(float));
                   for (int j = 0; j < DIM / 2; j++)
                     cos_curr[j] = 1.0f;

                   for (int i = 0; i < SEQ; i++) {
                     float *oi = od + i * DIM;
                     for (int j = 0; j < DIM / 2; j += 4) {
                       float32x4_t s_c = vld1q_f32(sin_curr + j);
                       float32x4_t c_c = vld1q_f32(cos_curr + j);
                       float32x4_t s_d = vld1q_f32(sin_delta + j);
                       float32x4_t c_d = vld1q_f32(cos_delta + j);

                       float32x4_t s_next =
                           vaddq_f32(vmulq_f32(s_c, c_d), vmulq_f32(c_c, s_d));
                       float32x4_t c_next =
                           vsubq_f32(vmulq_f32(c_c, c_d), vmulq_f32(s_c, s_d));

                       vst1q_f32(sin_curr + j, s_next);
                       vst1q_f32(cos_curr + j, c_next);

                       float32x4x2_t zipped = vzipq_f32(s_next, c_next);
                       vst1q_f32(oi + j * 2, zipped.val[0]);
                       vst1q_f32(oi + j * 2 + 4, zipped.val[1]);
                     }
                   }
                   free(sin_curr);
                   free(cos_curr);
                 });

  // Store in cache if context exists
  if (ctx) {
    if (ctx->pe_cache.data)
      free(ctx->pe_cache.data);
    ctx->pe_cache.data = malloc(SEQ * DIM * sizeof(float));
    memcpy(ctx->pe_cache.data, od, SEQ * DIM * sizeof(float));
    ctx->pe_cache.seq = SEQ;
    ctx->pe_cache.dim = DIM;
    ctx->pe_cache.valid = true;
  }

  free(sin_delta);
  free(cos_delta);
}

void nova_kernel_stabilize_fp(NovaTensor *t) { (void)t; }
