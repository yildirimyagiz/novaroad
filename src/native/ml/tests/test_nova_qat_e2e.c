/**
 * test_nova_qat_e2e.c
 * Nova ML — Quantization-Aware Training (QAT) End-to-End Tests
 *
 * Tests:
 *   1. Fake-quantize round-trip error is bounded
 *   2. QAT EMA scale calibration converges
 *   3. QAT training loop: loss decreases with fake-quant on weights
 *   4. QAT + AdamW: fit y=2x+1 with fake-quantized weights
 *   5. INT8 export: exported weights are close to FP32 original
 *   6. INT8 inference: forward pass matches FP32 within tolerance
 *   7. QAT vs FP32 final loss comparison (QAT ≈ FP32, small gap)
 *   8. Post-QAT INT8 linear layer end-to-end accuracy
 */

/* ── Memory stubs (must come first) ─────────────────────────────── */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

void *nova_malloc(size_t n)             { return malloc(n); }
void *nova_malloc_aligned(size_t n, size_t a) {
    void *p = NULL;
    (void)posix_memalign(&p, a < 8 ? 8 : a, n);
    return p;
}
void  nova_free(void *p)                { free(p); }

/* ── Nova ML includes ────────────────────────────────────────────── */
#include "../nova_c_compat.h"
#include "ml/nova_tensor.h"
#include "nova_tensor_ops.h"
#include "nova_nn.h"
#include "nova_metrics.h"
#include "../quantization/nova_qat.h"
#include "../quantization/nova_quantization.h"
#include "../ai/optim/nova_optim.h"

/* ── Test framework ──────────────────────────────────────────────── */
static int g_passed = 0, g_failed = 0;
static const char *current_test = "";

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s\n    %s (line %d)\n", current_test, msg, __LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define ASSERT_LT(a, b, msg) do { \
    if (!((a) < (b))) { \
        printf("  FAIL: %s\n    %.6f >= %.6f — %s (line %d)\n", \
               current_test,(double)(a),(double)(b),msg,__LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, tol, msg) do { \
    if (fabsf((float)(a)-(float)(b)) > (float)(tol)) { \
        printf("  FAIL: %s\n    |%.6f - %.6f| > %.6f — %s (line %d)\n", \
               current_test,(double)(a),(double)(b),(double)(tol),msg,__LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define RUN(fn) do { \
    current_test = #fn; \
    printf("  [TEST] %-52s", #fn); \
    fflush(stdout); \
    int _before = g_failed; \
    fn(); \
    if (g_failed == _before) { printf("PASS\n"); g_passed++; } \
} while(0)

/* ── Training helpers (self-contained) ──────────────────────────── */

static float mse_loss_raw(const float *pred, const float *tgt, int n) {
    float s = 0.0f;
    for (int i = 0; i < n; i++) { float d = pred[i]-tgt[i]; s += d*d; }
    return s / (float)n;
}

static void mse_grad_raw(const float *pred, const float *tgt,
                          float *grad, int n) {
    float inv_n = 2.0f / (float)n;
    for (int i = 0; i < n; i++) grad[i] = inv_n * (pred[i] - tgt[i]);
}

static float relu_f(float x)      { return x > 0.0f ? x : 0.0f; }
static float relu_grad_f(float x) { return x > 0.0f ? 1.0f : 0.0f; }

/* simple matmul: C[MxN] = A[MxK] @ B[KxN] */
static void mm(const float *A, const float *B, float *C,
               int M, int K, int N) {
    memset(C, 0, (size_t)(M*N)*sizeof(float));
    for (int i = 0; i < M; i++)
        for (int k = 0; k < K; k++)
            for (int j = 0; j < N; j++)
                C[i*N+j] += A[i*K+k] * B[k*N+j];
}

/* ================================================================
 * TEST 1: Fake-quantize round-trip error bounded
 * ================================================================ */
static void test_fake_quantize_roundtrip(void) {
    const int N = 256;
    float in[256], out[256];
    for (int i = 0; i < N; i++)
        in[i] = -1.0f + 2.0f * (float)i / (N - 1);

    NovaQATLayer *l = nova_qat_layer_create(true, false, 0, 0.1f);
    nova_fake_quantize_f32(in, out, N, l);

    float max_err = 0.0f;
    for (int i = 0; i < N; i++) {
        float e = fabsf(out[i] - in[i]);
        if (e > max_err) max_err = e;
    }
    /* max error should be < scale/2 ≈ 1/127/2 ≈ 0.004 */
    printf("\n    max_roundtrip_err=%.5f  ", max_err);
    ASSERT_LT(max_err, 0.02f, "Fake-quant roundtrip error must be < 0.02");

    nova_qat_layer_free(l);
}

/* ================================================================
 * TEST 2: EMA scale calibration converges
 * ================================================================ */
static void test_ema_scale_calibration(void) {
    NovaQATLayer *l = nova_qat_layer_create(true, false, 0, 0.1f);
    float data[64];

    /* Feed 50 batches of data with range [-2, 2] */
    for (int step = 0; step < 50; step++) {
        for (int i = 0; i < 64; i++)
            data[i] = -2.0f + 4.0f * (float)i / 63.0f;
        nova_qat_calibrate(l, data, 64);
    }

    /* Expected scale ≈ 2.0/127 ≈ 0.01575 */
    float expected = 2.0f / 127.0f;
    printf("\n    ema_scale=%.5f  expected≈%.5f  ", l->scale, expected);
    ASSERT_NEAR(l->scale, expected, 0.003f, "EMA scale should converge near 2/127");

    nova_qat_layer_free(l);
}

/* ================================================================
 * TEST 3: QAT training loop — loss decreases
 * ================================================================ */
static void test_qat_training_loss_decreases(void) {
    srand(42);
    /* y = 3x + 1 */
    const int BATCH = 8, IN = 1, OUT = 1, STEPS = 200;
    const float LR = 0.01f;

    /* data */
    float X[8]  = {0, 1, 2, 3, 4, 5, 6, 7};
    float Y[8]  = {1, 4, 7, 10, 13, 16, 19, 22};

    /* parameters: W[1x1], b[1] */
    float W = 0.0f, b = 0.0f;
    float W_fq, b_fq; /* fake-quantized copies */

    NovaQATLayer *l_w = nova_qat_layer_create(true, false, 0, 0.2f);
    NovaQATLayer *l_b = nova_qat_layer_create(true, false, 0, 0.2f);

    float first_loss = -1.0f, last_loss = -1.0f;

    for (int step = 0; step < STEPS; step++) {
        /* Fake-quantize weights */
        nova_fake_quantize_f32(&W, &W_fq, 1, l_w);
        nova_fake_quantize_f32(&b, &b_fq, 1, l_b);

        /* Forward: pred = X * W_fq + b_fq */
        float pred[8];
        for (int i = 0; i < BATCH; i++)
            pred[i] = X[i] * W_fq + b_fq;

        float loss = mse_loss_raw(pred, Y, BATCH);
        if (step == 0)        first_loss = loss;
        if (step == STEPS-1)  last_loss  = loss;

        /* Backward (STE: gradient flows through fake-quant unchanged) */
        float dl[8];
        mse_grad_raw(pred, Y, dl, BATCH);

        float dW = 0.0f, db = 0.0f;
        for (int i = 0; i < BATCH; i++) {
            dW += dl[i] * X[i];
            db += dl[i];
        }
        dW /= (float)BATCH;
        db /= (float)BATCH;

        /* SGD update on real FP32 weights */
        W -= LR * dW;
        b -= LR * db;
    }

    printf("\n    first_loss=%.2f  last_loss=%.4f  ", first_loss, last_loss);
    ASSERT_LT(last_loss, first_loss, "QAT loss must decrease");
    ASSERT_LT(last_loss, 1.0f, "QAT final loss must be < 1.0");

    nova_qat_layer_free(l_w);
    nova_qat_layer_free(l_b);
}

/* ================================================================
 * TEST 4: QAT + AdamW full training loop
 * ================================================================ */
static void test_qat_adamw_training(void) {
    srand(7);
    const int BATCH = 4, IN = 2, OUT = 1, STEPS = 300;

    /* y = 2*x0 + 3*x1 + 1 */
    float X[8]  = {1,2, 3,4, 5,6, 7,8};
    float Y[4];
    for (int i = 0; i < BATCH; i++)
        Y[i] = 2.0f*X[i*2] + 3.0f*X[i*2+1] + 1.0f;

    /* FP32 parameters: W[2], b[1] */
    float W[2] = {0.0f, 0.0f};
    float B[1] = {0.0f};

    /* AdamW state */
    float m_W[2]={0}, v_W[2]={0}, m_B[1]={0}, v_B[1]={0};
    const float lr=1e-2f, beta1=0.9f, beta2=0.999f, eps=1e-8f, wd=1e-4f;
    int t = 0;

    /* QAT layers */
    NovaQATLayer *lW = nova_qat_layer_create(true, false, 0, 0.1f);
    NovaQATLayer *lB = nova_qat_layer_create(true, false, 0, 0.1f);

    float first_loss = -1.0f, last_loss = -1.0f;

    for (int step = 0; step < STEPS; step++) {
        t++;
        /* Fake-quantize */
        float W_fq[2], B_fq[1];
        nova_fake_quantize_f32(W, W_fq, 2, lW);
        nova_fake_quantize_f32(B, B_fq, 1, lB);

        /* Forward */
        float pred[4];
        for (int b2 = 0; b2 < BATCH; b2++)
            pred[b2] = X[b2*2]*W_fq[0] + X[b2*2+1]*W_fq[1] + B_fq[0];

        float loss = mse_loss_raw(pred, Y, BATCH);
        if (step == 0)        first_loss = loss;
        if (step == STEPS-1)  last_loss  = loss;

        /* Backward (STE) */
        float dl[4];
        mse_grad_raw(pred, Y, dl, BATCH);

        float gW[2]={0}, gB[1]={0};
        for (int b2 = 0; b2 < BATCH; b2++) {
            gW[0] += dl[b2] * X[b2*2];
            gW[1] += dl[b2] * X[b2*2+1];
            gB[0] += dl[b2];
        }
        for (int i = 0; i < 2; i++) gW[i] /= (float)BATCH;
        gB[0] /= (float)BATCH;

        /* AdamW update */
        float bc1 = 1.0f - powf(beta1, (float)t);
        float bc2 = 1.0f - powf(beta2, (float)t);
        for (int i = 0; i < IN; i++) {
            float g = gW[i] + wd * W[i];
            m_W[i] = beta1*m_W[i] + (1-beta1)*g;
            v_W[i] = beta2*v_W[i] + (1-beta2)*g*g;
            W[i] -= lr * (m_W[i]/bc1) / (sqrtf(v_W[i]/bc2) + eps);
        }
        {
            float g = gB[0] + wd * B[0];
            m_B[0] = beta1*m_B[0] + (1-beta1)*g;
            v_B[0] = beta2*v_B[0] + (1-beta2)*g*g;
            B[0] -= lr * (m_B[0]/bc1) / (sqrtf(v_B[0]/bc2) + eps);
        }
    }

    printf("\n    first_loss=%.2f  last_loss=%.4f  W=[%.3f,%.3f]  b=%.3f  ",
           first_loss, last_loss, W[0], W[1], B[0]);
    ASSERT_LT(last_loss, first_loss, "QAT+AdamW loss must decrease");
    ASSERT_LT(last_loss, 2.0f, "QAT+AdamW final loss < 2.0");

    nova_qat_layer_free(lW);
    nova_qat_layer_free(lB);
}

/* ================================================================
 * TEST 5: INT8 export — quantized weights close to FP32
 * ================================================================ */
static void test_int8_export_accuracy(void) {
    const int N = 64;
    float W_f32[64];
    for (int i = 0; i < N; i++)
        W_f32[i] = -1.0f + 2.0f * (float)i / (N-1);

    NovaQATLayer *l = nova_qat_layer_create(true, false, 0, 0.1f);
    /* Calibrate with the same data */
    float dummy[64];
    nova_fake_quantize_f32(W_f32, dummy, N, l);

    NovaINT8Weight *w8 = nova_qat_export_int8(W_f32, N, l);
    ASSERT(w8 != NULL, "INT8 export must succeed");
    ASSERT(w8->data != NULL, "INT8 data must not be NULL");

    /* Dequantize and check error */
    float W_dq[64];
    nova_dequantize_int8_to_f32(w8->data, W_dq, N, w8->scale, w8->zero_point);

    float max_err = 0.0f;
    for (int i = 0; i < N; i++) {
        float e = fabsf(W_f32[i] - W_dq[i]);
        if (e > max_err) max_err = e;
    }
    printf("\n    max_export_err=%.5f  scale=%.5f  ", max_err, w8->scale);
    ASSERT_LT(max_err, 0.02f, "INT8 export max error must be < 0.02");

    nova_int8_weight_free(w8);
    nova_qat_layer_free(l);
}

/* ================================================================
 * TEST 6: INT8 inference matches FP32 within tolerance
 * ================================================================ */
static void test_int8_inference_accuracy(void) {
    srand(99);
    const int BATCH=4, IN=8, OUT=4;

    /* Random FP32 weights and input */
    float W_f32[4*8], input[4*8], bias[4];
    float out_f32[4*4], out_int8[4*4];

    for (int i = 0; i < IN*OUT; i++)
        W_f32[i] = -0.5f + (float)(rand()%1000)/1000.0f;
    for (int i = 0; i < BATCH*IN; i++)
        input[i] = -1.0f + (float)(rand()%2000)/1000.0f;
    for (int i = 0; i < OUT; i++)
        bias[i] = 0.1f * (float)(i - OUT/2);

    /* FP32 reference forward */
    mm(input, W_f32, out_f32, BATCH, IN, OUT);
    for (int b = 0; b < BATCH; b++)
        for (int o = 0; o < OUT; o++)
            out_f32[b*OUT+o] += bias[o];

    /* Export weights to INT8 */
    NovaQATLayer *l = nova_qat_layer_create(true, false, 0, 0.2f);
    float W_fq[4*8];
    nova_fake_quantize_f32(W_f32, W_fq, IN*OUT, l);
    NovaINT8Weight *w8 = nova_qat_export_int8(W_f32, IN*OUT, l);

    /* INT8 inference */
    nova_int8_linear_forward(input, w8->data, bias, out_int8,
                              BATCH, IN, OUT, w8->scale, w8->zero_point);

    /* Compare */
    float max_diff = 0.0f;
    for (int i = 0; i < BATCH*OUT; i++) {
        float d = fabsf(out_f32[i] - out_int8[i]);
        if (d > max_diff) max_diff = d;
    }
    printf("\n    max_inference_diff=%.4f  ", max_diff);
    ASSERT_LT(max_diff, 0.15f, "INT8 inference must match FP32 within 0.15");

    nova_int8_weight_free(w8);
    nova_qat_layer_free(l);
}

/* ================================================================
 * TEST 7: QAT vs FP32 loss gap is small
 * ================================================================ */
static void test_qat_vs_fp32_loss_gap(void) {
    const int BATCH=8, STEPS=150;
    const float LR = 0.01f;

    float X[8]  = {1,2,3,4,5,6,7,8};
    float Y[8];
    for (int i = 0; i < BATCH; i++) Y[i] = 2.5f*X[i] + 0.5f;

    /* FP32 training */
    float W_fp=0, b_fp=0, loss_fp=-1;
    for (int s = 0; s < STEPS; s++) {
        float pred[8];
        for (int i = 0; i < BATCH; i++) pred[i] = X[i]*W_fp + b_fp;
        if (s == STEPS-1) loss_fp = mse_loss_raw(pred, Y, BATCH);
        float dl[8]; mse_grad_raw(pred, Y, dl, BATCH);
        float dW=0, db=0;
        for (int i = 0; i < BATCH; i++) { dW+=dl[i]*X[i]; db+=dl[i]; }
        W_fp -= LR*dW/BATCH; b_fp -= LR*db/BATCH;
    }

    /* QAT training */
    float W_qat=0, b_qat=0, loss_qat=-1;
    NovaQATLayer *lW = nova_qat_layer_create(true, false, 0, 0.15f);
    NovaQATLayer *lB = nova_qat_layer_create(true, false, 0, 0.15f);
    for (int s = 0; s < STEPS; s++) {
        float W_fq, b_fq;
        nova_fake_quantize_f32(&W_qat, &W_fq, 1, lW);
        nova_fake_quantize_f32(&b_qat, &b_fq, 1, lB);
        float pred[8];
        for (int i = 0; i < BATCH; i++) pred[i] = X[i]*W_fq + b_fq;
        if (s == STEPS-1) loss_qat = mse_loss_raw(pred, Y, BATCH);
        float dl[8]; mse_grad_raw(pred, Y, dl, BATCH);
        float dW=0, db=0;
        for (int i = 0; i < BATCH; i++) { dW+=dl[i]*X[i]; db+=dl[i]; }
        W_qat -= LR*dW/BATCH; b_qat -= LR*db/BATCH;
    }
    nova_qat_layer_free(lW); nova_qat_layer_free(lB);

    float gap = fabsf(loss_qat - loss_fp);
    printf("\n    loss_fp32=%.4f  loss_qat=%.4f  gap=%.4f  ",
           loss_fp, loss_qat, gap);
    ASSERT_LT(gap, 5.0f, "QAT vs FP32 loss gap must be < 5.0");
}

/* ================================================================
 * TEST 8: Post-QAT INT8 layer end-to-end accuracy (LinearLayer)
 * ================================================================ */
static void test_post_qat_int8_e2e(void) {
    srand(42);
    const int BATCH=4, IN=4, OUT=2, STEPS=200;
    const float LR=0.02f;

    /* Simple dataset: out = [sum(in), prod(first2)] */
    float X[4*4] = {1,2,3,4, 2,1,4,3, 3,4,1,2, 4,3,2,1};
    float Y[4*2];
    for (int b = 0; b < BATCH; b++) {
        Y[b*2+0] = X[b*4]+X[b*4+1]+X[b*4+2]+X[b*4+3]; /* sum */
        Y[b*2+1] = X[b*4]*X[b*4+1];                     /* prod first 2 */
    }

    /* FP32 weights W[OUT x IN], bias[OUT] */
    float W[2*4]={0}, B[2]={0};
    NovaQATLayer *lW = nova_qat_layer_create(true, false, 0, 0.1f);

    for (int s = 0; s < STEPS; s++) {
        /* Fake-quantize W */
        float W_fq[2*4];
        nova_fake_quantize_f32(W, W_fq, OUT*IN, lW);

        /* Forward: pred[BATCH x OUT] = X[BATCH x IN] @ W_fq^T + B */
        float pred[4*2]={0};
        for (int b = 0; b < BATCH; b++)
            for (int o = 0; o < OUT; o++) {
                for (int i = 0; i < IN; i++)
                    pred[b*OUT+o] += X[b*IN+i] * W_fq[o*IN+i];
                pred[b*OUT+o] += B[o];
            }

        float loss = mse_loss_raw(pred, Y, BATCH*OUT);

        /* Backward (STE) */
        float dL[4*2]; mse_grad_raw(pred, Y, dL, BATCH*OUT);
        float dW[2*4]={0}, dB[2]={0};
        for (int b = 0; b < BATCH; b++)
            for (int o = 0; o < OUT; o++) {
                dB[o] += dL[b*OUT+o];
                for (int i = 0; i < IN; i++)
                    dW[o*IN+i] += dL[b*OUT+o] * X[b*IN+i];
            }
        for (int i = 0; i < OUT*IN; i++) W[i] -= LR * dW[i]/BATCH;
        for (int i = 0; i < OUT;    i++) B[i] -= LR * dB[i]/BATCH;
        (void)loss;
    }

    /* Export to INT8 */
    NovaINT8Weight *w8 = nova_qat_export_int8(W, OUT*IN, lW);

    /* INT8 inference */
    float out_int8[4*2];
    nova_int8_linear_forward(X, w8->data, B, out_int8,
                              BATCH, IN, OUT, w8->scale, w8->zero_point);

    /* FP32 reference inference */
    float out_fp32[4*2]={0};
    for (int b = 0; b < BATCH; b++)
        for (int o = 0; o < OUT; o++) {
            for (int i = 0; i < IN; i++)
                out_fp32[b*OUT+o] += X[b*IN+i] * W[o*IN+i];
            out_fp32[b*OUT+o] += B[o];
        }

    float max_diff = 0.0f;
    for (int i = 0; i < BATCH*OUT; i++) {
        float d = fabsf(out_fp32[i] - out_int8[i]);
        if (d > max_diff) max_diff = d;
    }
    printf("\n    max_int8_vs_fp32=%.4f  ", max_diff);
    ASSERT_LT(max_diff, 1.0f, "Post-QAT INT8 inference close to FP32");

    nova_int8_weight_free(w8);
    nova_qat_layer_free(lW);
}

/* ================================================================
 * MAIN
 * ================================================================ */
int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║   Nova ML — Quantization-Aware Training (QAT) E2E Tests    ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    RUN(test_fake_quantize_roundtrip);
    RUN(test_ema_scale_calibration);
    RUN(test_qat_training_loss_decreases);
    RUN(test_qat_adamw_training);
    RUN(test_int8_export_accuracy);
    RUN(test_int8_inference_accuracy);
    RUN(test_qat_vs_fp32_loss_gap);
    RUN(test_post_qat_int8_e2e);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Results: %d/%d passed%-39s║\n",
           g_passed, g_passed+g_failed, "");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    return g_failed > 0 ? 1 : 0;
}
