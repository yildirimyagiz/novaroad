/**
 * test_nova_adamw_e2e.c
 * Nova ML — AdamW Optimizer End-to-End Tests
 *
 * Tests AdamW optimizer integration with:
 *   - Linear layers
 *   - ReLU activation
 *   - MSE loss
 *   - Comprehensive AdamW validation
 */

#include <stdlib.h>

#include "../nova_c_compat.h"
#include "ml/nova_tensor.h"
#include "nova_tensor_ops.h"
#include "nova_nn.h"
#include "nova_metrics.h"
#include "nova_optim.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Minimal test framework
 * ============================================================ */
static int g_passed = 0, g_failed = 0;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s\n    Reason: %s (line %d)\n", \
               current_test, msg, __LINE__); \
        g_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_LT(a, b, msg) do { \
    if (!((a) < (b))) { \
        printf("  FAIL: %s\n    Expected %f < %f — %s (line %d)\n", \
               current_test, (double)(a), (double)(b), msg, __LINE__); \
        g_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, tol, msg) do { \
    if (fabsf((a)-(b)) > (tol)) { \
        printf("  FAIL: %s\n    |%f - %f| > %f — %s (line %d)\n", \
               current_test, (double)(a), (double)(b), (double)(tol), msg, __LINE__); \
        g_failed++; \
        return; \
    } \
} while(0)

static const char *current_test = "";

#define RUN(fn) do { \
    current_test = #fn; \
    printf("  [TEST] %-50s", #fn); \
    int before = g_failed; \
    fn(); \
    if (g_failed == before) { printf("PASS\n"); g_passed++; } \
} while(0)

/* ============================================================
 * Helpers
 * ============================================================ */

/* MSE loss: mean((pred - target)^2) */
static float mse_loss(NovaTensor *pred, NovaTensor *target) {
    float *p = (float *)pred->data;
    float *t = (float *)target->data;
    float sum = 0.0f;
    for (size_t i = 0; i < pred->total_elements; i++) {
        float diff = p[i] - t[i];
        sum += diff * diff;
    }
    return sum / (float)pred->total_elements;
}

/* MSE gradient w.r.t. pred: 2*(pred - target) / N */
static NovaTensor *mse_grad(NovaTensor *pred, NovaTensor *target) {
    NovaTensor *grad = nova_tensor_create(NULL, pred->shape, pred->ndim, pred->dtype);
    float *p  = (float *)pred->data;
    float *t  = (float *)target->data;
    float *g  = (float *)grad->data;
    float n   = (float)pred->total_elements;
    for (size_t i = 0; i < pred->total_elements; i++)
        g[i] = 2.0f * (p[i] - t[i]) / n;
    return grad;
}

/* ReLU activation */
static float relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

/* ReLU gradient: 1 if x > 0 else 0 */
static float relu_grad(float x) {
    return x > 0.0f ? 1.0f : 0.0f;
}

/* ReLU backward: mask gradient by input sign */
static NovaTensor *relu_backward(NovaTensor *grad, NovaTensor *x_cache) {
    NovaTensor *out = nova_tensor_create(NULL, grad->shape, grad->ndim, grad->dtype);
    float *gi = (float *)grad->data;
    float *xi = (float *)x_cache->data;
    float *oi = (float *)out->data;
    for (size_t i = 0; i < grad->total_elements; i++)
        oi[i] = xi[i] > 0.0f ? gi[i] : 0.0f;
    return out;
}

/* Linear backward:
 *   dW  = grad^T @ input        [out_feat x in_feat]
 *   db  = sum(grad, axis=0)     [out_feat]
 *   d_input = grad @ W          [batch x in_feat]
 */
static void linear_backward(LinearLayer *layer, NovaTensor *input,
                             NovaTensor *grad_out,
                             NovaTensor **dW_out, NovaTensor **db_out,
                             NovaTensor **d_input_out) {
    int batch      = (int)input->shape[0];
    int in_feat    = (int)input->shape[1];
    int out_feat   = (int)grad_out->shape[1];

    /* dW = grad_out^T @ input  → [out_feat x in_feat] */
    int64_t dw_shape[] = {out_feat, in_feat};
    NovaTensor *dW = nova_tensor_create(NULL, dw_shape, 2, NOVA_DTYPE_FP32);
    float *go = (float *)grad_out->data;
    float *inp = (float *)input->data;
    float *dw  = (float *)dW->data;
    memset(dw, 0, out_feat * in_feat * sizeof(float));
    for (int b = 0; b < batch; b++)
        for (int o = 0; o < out_feat; o++)
            for (int i = 0; i < in_feat; i++)
                dw[o * in_feat + i] += go[b * out_feat + o] * inp[b * in_feat + i];

    /* db = sum(grad_out, axis=0) → [out_feat] */
    int64_t db_shape[] = {out_feat};
    NovaTensor *db = nova_tensor_create(NULL, db_shape, 1, NOVA_DTYPE_FP32);
    float *db_d = (float *)db->data;
    memset(db_d, 0, out_feat * sizeof(float));
    for (int b = 0; b < batch; b++)
        for (int o = 0; o < out_feat; o++)
            db_d[o] += go[b * out_feat + o];

    /* d_input = grad_out @ W  → [batch x in_feat] */
    int64_t di_shape[] = {batch, in_feat};
    NovaTensor *d_input = nova_tensor_create(NULL, di_shape, 2, NOVA_DTYPE_FP32);
    float *di  = (float *)d_input->data;
    float *W   = (float *)layer->weight->data;
    memset(di, 0, batch * in_feat * sizeof(float));
    for (int b = 0; b < batch; b++)
        for (int i = 0; i < in_feat; i++)
            for (int o = 0; o < out_feat; o++)
                di[b * in_feat + i] += go[b * out_feat + o] * W[o * in_feat + i];

    *dW_out = dW;
    *db_out = db;
    *d_input_out = d_input;
}

/* ============================================================
 * TEST 1: AdamW state initialization
 * ============================================================ */
static void test_adamw_state_init(void) {
    const int n_params = 2;
    int64_t shape[] = {3, 4};
    
    NovaTensor *params[2];
    params[0] = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    params[1] = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    /* Fill with non-zero values */
    float *p0 = (float *)params[0]->data;
    float *p1 = (float *)params[1]->data;
    for (size_t i = 0; i < params[0]->total_elements; i++) {
        p0[i] = 0.5f;
        p1[i] = -0.3f;
    }
    
    NovaAdamWOptimizer *opt = nova_adamw_create(1e-3f, 0.9f, 0.999f, 1e-8f, 1e-4f);
    ASSERT(opt != NULL, "AdamW optimizer creation failed");
    ASSERT(opt->step == 0, "Initial step should be 0");
    ASSERT(opt->num_params == 0, "Initial num_params should be 0");
    
    nova_adamw_init_state(opt, params, n_params);
    ASSERT(opt->num_params == n_params, "num_params should be set");
    ASSERT(opt->m_states != NULL, "m_states should be allocated");
    ASSERT(opt->v_states != NULL, "v_states should be allocated");
    
    /* Check that m and v are zero-initialized */
    for (int i = 0; i < n_params; i++) {
        float *m_data = (float *)opt->m_states[i]->data;
        float *v_data = (float *)opt->v_states[i]->data;
        for (size_t j = 0; j < params[i]->total_elements; j++) {
            ASSERT(fabsf(m_data[j]) < 1e-9f, "m should be zero-initialized");
            ASSERT(fabsf(v_data[j]) < 1e-9f, "v should be zero-initialized");
        }
    }
    
    nova_adamw_free(opt);
    nova_tensor_destroy(params[0]);
    nova_tensor_destroy(params[1]);
}

/* ============================================================
 * TEST 2: Single step with known gradient
 * ============================================================ */
static void test_adamw_single_step(void) {
    int64_t shape[] = {1, 1};
    NovaTensor *param = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *grad = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    float *p_data = (float *)param->data;
    float *g_data = (float *)grad->data;
    
    p_data[0] = 1.0f;
    g_data[0] = 0.5f;
    param->grad = grad;
    
    NovaTensor *params[] = {param};
    NovaAdamWOptimizer *opt = nova_adamw_create(0.1f, 0.9f, 0.999f, 1e-8f, 0.0f);
    
    /* First step */
    nova_adamw_step(opt, params, 1);
    float p_after_step1 = p_data[0];
    
    /* Parameter should decrease (step > 0) */
    ASSERT(p_after_step1 < 1.0f, "Parameter should decrease after gradient step");
    ASSERT(opt->step == 1, "Step counter should increment");
    
    nova_adamw_free(opt);
    nova_tensor_destroy(param);
    nova_tensor_destroy(grad);
}

/* ============================================================
 * TEST 3: Weight decay effect
 * ============================================================ */
static void test_adamw_weight_decay(void) {
    int64_t shape[] = {1, 1};
    NovaTensor *param = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *grad = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    float *p_data = (float *)param->data;
    float *g_data = (float *)grad->data;
    
    p_data[0] = 1.0f;
    g_data[0] = 0.0f;  /* Zero gradient */
    param->grad = grad;
    
    NovaTensor *params[] = {param};
    float lr = 0.1f;
    float wd = 0.1f;
    NovaAdamWOptimizer *opt = nova_adamw_create(lr, 0.9f, 0.999f, 1e-8f, wd);
    
    /* With weight decay, param should decrease even with zero gradient */
    float p_before = p_data[0];
    nova_adamw_step(opt, params, 1);
    float p_after = p_data[0];
    
    ASSERT(p_after < p_before, "Weight decay should reduce param even with zero gradient");
    
    /* Expected decay factor: 1 - lr * wd = 1 - 0.1 * 0.1 = 0.99 */
    float expected = p_before * 0.99f;
    ASSERT(fabsf(p_after - expected) < 1e-6f, "Weight decay amount should match formula");
    
    nova_adamw_free(opt);
    nova_tensor_destroy(param);
    nova_tensor_destroy(grad);
}

/* ============================================================
 * TEST 4: Bias correction in early steps
 * ============================================================ */
static void test_adamw_bias_correction(void) {
    int64_t shape[] = {1, 1};
    NovaTensor *param = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *grad = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    
    float *p_data = (float *)param->data;
    float *g_data = (float *)grad->data;
    
    p_data[0] = 0.0f;
    g_data[0] = 1.0f;
    param->grad = grad;
    
    NovaTensor *params[] = {param};
    NovaAdamWOptimizer *opt = nova_adamw_create(0.1f, 0.9f, 0.999f, 1e-8f, 0.0f);
    
    /* Verify that step counter increments and bias correction is applied */
    ASSERT(opt->step == 0, "Initial step should be 0");
    nova_adamw_step(opt, params, 1);
    ASSERT(opt->step == 1, "Step counter should increment to 1");
    
    /* Verify that second step happens without error */
    g_data[0] = 1.0f;
    nova_adamw_step(opt, params, 1);
    ASSERT(opt->step == 2, "Step counter should increment to 2");
    
    nova_adamw_free(opt);
    nova_tensor_destroy(param);
    nova_tensor_destroy(grad);
}

/* ============================================================
 * TEST 5: AdamW convergence on simple optimization
 * ============================================================ */
static void test_adamw_vs_sgd(void) {
    srand(42);
    const int STEPS = 150;
    
    /* Simple quadratic: minimize (x-2)^2 starting from x=0 */
    int64_t shape[] = {1, 1};
    
    /* AdamW version (should converge well with adaptive lr) */
    NovaTensor *param_adam = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    NovaTensor *grad_adam = nova_tensor_create(NULL, shape, 2, NOVA_DTYPE_FP32);
    float *p_adam = (float *)param_adam->data;
    float *g_adam = (float *)grad_adam->data;
    p_adam[0] = 0.0f;
    param_adam->grad = grad_adam;
    
    NovaTensor *params[] = {param_adam};
    NovaAdamWOptimizer *opt = nova_adamw_create(0.1f, 0.9f, 0.999f, 1e-8f, 0.0f);
    
    float first_loss_adam = 0.0f;
    for (int step = 0; step < STEPS; step++) {
        float x = p_adam[0];
        g_adam[0] = 2.0f * (x - 2.0f);  /* d/dx (x-2)^2 = 2(x-2) */
        if (step == 0) first_loss_adam = powf(x - 2.0f, 2.0f);
        nova_adamw_step(opt, params, 1);
        nova_adamw_zero_grad(params, 1);
    }
    float last_loss_adam = powf(p_adam[0] - 2.0f, 2.0f);
    
    printf("\n    first_loss=%.6f  final_loss=%.6f  ", first_loss_adam, last_loss_adam);
    ASSERT_LT(last_loss_adam, first_loss_adam, "AdamW should decrease loss over time");
    ASSERT_LT(last_loss_adam, 0.01f, "AdamW should converge to near-optimal value");
    
    nova_adamw_free(opt);
    nova_tensor_destroy(param_adam);
    nova_tensor_destroy(grad_adam);
}

/* ============================================================
 * TEST 6: Linear regression with AdamW
 * ============================================================ */
static void test_adamw_linear_regression(void) {
    srand(123);
    const int BATCH = 8, IN = 2, OUT = 1;
    const float LR = 0.01f;
    const int STEPS = 500;
    
    /* y = 3*x0 + 2*x1 */
    int64_t x_shape[] = {BATCH, IN};
    NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
    float *xp = (float *)X->data;
    for (int i = 0; i < BATCH * IN; i++) {
        xp[i] = (i % 10) / 10.0f;  /* Random-ish values */
    }
    
    int64_t y_shape[] = {BATCH, OUT};
    NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
    float *yp = (float *)Y->data;
    for (int i = 0; i < BATCH; i++) {
        yp[i] = 3.0f * xp[i*2] + 2.0f * xp[i*2+1];
    }
    
    LinearLayer *layer = linear_create(IN, OUT);
    NovaTensor *params[] = {layer->weight, layer->bias};
    NovaAdamWOptimizer *opt = nova_adamw_create(LR, 0.9f, 0.999f, 1e-8f, 0.0f);
    
    float first_loss = 0.0f;
    float last_loss = 0.0f;
    
    for (int step = 0; step < STEPS; step++) {
        NovaTensor *pred = linear_forward(layer, X);
        NovaTensor *dL = mse_grad(pred, Y);
        
        NovaTensor *dW, *db, *d_inp;
        linear_backward(layer, X, dL, &dW, &db, &d_inp);
        
        layer->weight->grad = dW;
        layer->bias->grad = db;
        
        if (step == 0) first_loss = mse_loss(pred, Y);
        if (step == STEPS - 1) last_loss = mse_loss(pred, Y);
        
        nova_adamw_step(opt, params, 2);
        nova_adamw_zero_grad(params, 2);
        
        nova_tensor_destroy(pred);
        nova_tensor_destroy(dL);
        nova_tensor_destroy(dW);
        nova_tensor_destroy(db);
        nova_tensor_destroy(d_inp);
    }
    
    printf("\n    first_loss=%.6f  last_loss=%.6f  ", first_loss, last_loss);
    ASSERT_LT(last_loss, first_loss, "Loss should decrease over training");
    
    nova_adamw_free(opt);
    linear_free(layer);
    nova_tensor_destroy(X);
    nova_tensor_destroy(Y);
}

/* ============================================================
 * TEST 7: XOR with 2-layer MLP using AdamW
 * ============================================================ */
static void test_adamw_xor_mlp(void) {
    srand(456);
    const int BATCH = 4, H = 8, STEPS = 500;
    const float LR = 0.001f;
    
    /* XOR training data */
    int64_t x_shape[] = {BATCH, 2};
    NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
    float *xd = (float *)X->data;
    float xor_data[] = {0,0, 0,1, 1,0, 1,1};
    memcpy(xd, xor_data, sizeof(xor_data));
    
    int64_t y_shape[] = {BATCH, 1};
    NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
    float *yd = (float *)Y->data;
    float xor_labels[] = {0, 1, 1, 0};
    memcpy(yd, xor_labels, sizeof(xor_labels));
    
    LinearLayer *l1 = linear_create(2, H);
    LinearLayer *l2 = linear_create(H, 1);
    
    NovaTensor *params[] = {l1->weight, l1->bias, l2->weight, l2->bias};
    NovaAdamWOptimizer *opt = nova_adamw_create(LR, 0.9f, 0.999f, 1e-8f, 0.0f);
    
    float first_loss = 0.0f, last_loss = 0.0f;
    
    for (int step = 0; step < STEPS; step++) {
        /* Forward */
        NovaTensor *h1_pre = linear_forward(l1, X);
        NovaTensor *h1 = nova_tensor_create(NULL, h1_pre->shape, h1_pre->ndim, 
                                             h1_pre->dtype);
        float *h1_p = (float *)h1->data;
        float *h1_pre_p = (float *)h1_pre->data;
        for (size_t i = 0; i < h1_pre->total_elements; i++)
            h1_p[i] = relu(h1_pre_p[i]);
        
        NovaTensor *out = linear_forward(l2, h1);
        
        /* Loss */
        NovaTensor *dL = mse_grad(out, Y);
        float loss = mse_loss(out, Y);
        if (step == 0) first_loss = loss;
        if (step == STEPS - 1) last_loss = loss;
        
        /* Backward - simplified manual backprop */
        NovaTensor *dW2, *db2, *d_h1;
        linear_backward(l2, h1, dL, &dW2, &db2, &d_h1);
        
        NovaTensor *d_h1_pre = nova_tensor_create(NULL, h1_pre->shape, 
                                                   h1_pre->ndim, h1_pre->dtype);
        float *d_h1_pre_p = (float *)d_h1_pre->data;
        float *d_h1_p = (float *)d_h1->data;
        for (size_t i = 0; i < h1_pre->total_elements; i++)
            d_h1_pre_p[i] = d_h1_p[i] * relu_grad(h1_pre_p[i]);
        
        NovaTensor *dW1, *db1, *d_x;
        linear_backward(l1, X, d_h1_pre, &dW1, &db1, &d_x);
        
        /* Update with AdamW */
        l1->weight->grad = dW1;
        l1->bias->grad = db1;
        l2->weight->grad = dW2;
        l2->bias->grad = db2;
        
        nova_adamw_step(opt, params, 4);
        nova_adamw_zero_grad(params, 4);
        
        nova_tensor_destroy(h1_pre);
        nova_tensor_destroy(h1);
        nova_tensor_destroy(out);
        nova_tensor_destroy(dL);
        nova_tensor_destroy(dW2);
        nova_tensor_destroy(db2);
        nova_tensor_destroy(d_h1);
        nova_tensor_destroy(d_h1_pre);
        nova_tensor_destroy(dW1);
        nova_tensor_destroy(db1);
        nova_tensor_destroy(d_x);
    }
    
    printf("\n    first_loss=%.6f  last_loss=%.6f  ", first_loss, last_loss);
    ASSERT_LT(last_loss, first_loss, "XOR loss should decrease during training");
    
    nova_adamw_free(opt);
    linear_free(l1);
    linear_free(l2);
    nova_tensor_destroy(X);
    nova_tensor_destroy(Y);
}

/* ============================================================
 * TEST 8: Deterministic training
 * ============================================================ */
static void test_adamw_deterministic(void) {
    /* Train twice with same seed and verify same final loss */
    const int STEPS = 50;
    const float LR = 1e-3f;
    
    float final_loss[2];
    
    for (int run = 0; run < 2; run++) {
        srand(789);
        
        int64_t x_shape[] = {4, 2};
        NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
        float *xp = (float *)X->data;
        for (int i = 0; i < 8; i++) xp[i] = (i % 5) / 5.0f;
        
        int64_t y_shape[] = {4, 1};
        NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
        float *yp = (float *)Y->data;
        for (int i = 0; i < 4; i++) yp[i] = xp[i*2] + xp[i*2+1];
        
        LinearLayer *layer = linear_create(2, 1);
        NovaTensor *params[] = {layer->weight, layer->bias};
        NovaAdamWOptimizer *opt = nova_adamw_create(LR, 0.9f, 0.999f, 1e-8f, 0.0f);
        
        for (int step = 0; step < STEPS; step++) {
            NovaTensor *pred = linear_forward(layer, X);
            NovaTensor *dL = mse_grad(pred, Y);
            
            NovaTensor *dW, *db, *d_inp;
            linear_backward(layer, X, dL, &dW, &db, &d_inp);
            
            layer->weight->grad = dW;
            layer->bias->grad = db;
            
            nova_adamw_step(opt, params, 2);
            nova_adamw_zero_grad(params, 2);
            
            if (step == STEPS - 1) {
                final_loss[run] = mse_loss(pred, Y);
            }
            
            nova_tensor_destroy(pred);
            nova_tensor_destroy(dL);
            nova_tensor_destroy(dW);
            nova_tensor_destroy(db);
            nova_tensor_destroy(d_inp);
        }
        
        nova_adamw_free(opt);
        linear_free(layer);
        nova_tensor_destroy(X);
        nova_tensor_destroy(Y);
    }
    
    printf("\n    run1_loss=%.8f  run2_loss=%.8f  ", final_loss[0], final_loss[1]);
    ASSERT_NEAR(final_loss[0], final_loss[1], 1e-6f, "Deterministic training should yield same loss");
}

/* ============================================================
 * Main
 * ============================================================ */
int main(void) {
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("Nova ML — AdamW Optimizer E2E Tests\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    RUN(test_adamw_state_init);
    RUN(test_adamw_single_step);
    RUN(test_adamw_weight_decay);
    RUN(test_adamw_bias_correction);
    RUN(test_adamw_vs_sgd);
    RUN(test_adamw_linear_regression);
    RUN(test_adamw_xor_mlp);
    RUN(test_adamw_deterministic);
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("Results: %d passed, %d failed\n", g_passed, g_failed);
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    return g_failed > 0 ? 1 : 0;
}
