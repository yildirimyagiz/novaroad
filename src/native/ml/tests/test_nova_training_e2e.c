/**
 * test_nova_training_e2e.c
 * Nova ML — End-to-End Training Pipeline Test
 *
 * Tests a complete forward + backward + weight-update cycle using:
 *   - Linear layers (nova_nn.c)
 *   - ReLU activation
 *   - MSE loss
 *   - Vanilla SGD weight update (manual gradient descent)
 *   - Checks that loss decreases over N training steps
 */

/* Memory stubs — nova_malloc/free are thin wrappers over system malloc */
#include <stdlib.h>
void *nova_malloc(size_t size)          { return malloc(size); }
void *nova_malloc_aligned(size_t size, size_t align) {
    void *ptr = NULL;
    (void)posix_memalign(&ptr, align < sizeof(void*) ? sizeof(void*) : align, size);
    return ptr;
}
void  nova_free(void *ptr)              { free(ptr); }

#include "../nova_c_compat.h"
#include "ml/nova_tensor.h"
#include "nova_tensor_ops.h"
#include "nova_nn.h"
#include "nova_metrics.h"
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

/* ReLU forward (element-wise, in-place copy) */
static NovaTensor *relu_forward(NovaTensor *x) {
    NovaTensor *out = nova_tensor_create(NULL, x->shape, x->ndim, x->dtype);
    float *xi = (float *)x->data;
    float *oi = (float *)out->data;
    for (size_t i = 0; i < x->total_elements; i++)
        oi[i] = xi[i] > 0.0f ? xi[i] : 0.0f;
    return out;
}

/* ReLU backward: pass grad through where x > 0 */
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

/* SGD update: param -= lr * grad */
static void sgd_update(NovaTensor *param, NovaTensor *grad, float lr) {
    float *p = (float *)param->data;
    float *g = (float *)grad->data;
    for (size_t i = 0; i < param->total_elements; i++)
        p[i] -= lr * g[i];
}

/* ============================================================
 * TEST 1: Single linear layer — loss decreases
 * ============================================================ */
static void test_single_layer_loss_decreases(void) {
    srand(42);
    const int BATCH = 4, IN = 3, OUT = 1;
    const float LR = 0.01f;
    const int STEPS = 100;

    /* Input: [4 x 3] */
    int64_t x_shape[] = {BATCH, IN};
    NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
    float xd[] = {1,2,3, 4,5,6, 7,8,9, 2,4,6};
    memcpy(X->data, xd, sizeof(xd));

    /* Target: [4 x 1] = sum of inputs */
    int64_t y_shape[] = {BATCH, OUT};
    NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
    float yd[] = {6, 15, 24, 12};
    memcpy(Y->data, yd, sizeof(yd));

    LinearLayer *layer = linear_create(IN, OUT);

    float first_loss = -1.0f, last_loss = -1.0f;

    for (int step = 0; step < STEPS; step++) {
        /* Forward */
        NovaTensor *pred = linear_forward(layer, X);

        float loss = mse_loss(pred, Y);
        if (step == 0) first_loss = loss;
        if (step == STEPS - 1) last_loss = loss;

        /* Backward */
        NovaTensor *dL = mse_grad(pred, Y);
        NovaTensor *dW, *db, *d_inp;
        linear_backward(layer, X, dL, &dW, &db, &d_inp);

        /* SGD update */
        sgd_update(layer->weight, dW, LR);
        sgd_update(layer->bias,   db,  LR);

        nova_tensor_destroy(pred);
        nova_tensor_destroy(dL);
        nova_tensor_destroy(dW);
        nova_tensor_destroy(db);
        nova_tensor_destroy(d_inp);
    }

    printf("\n    first_loss=%.4f  last_loss=%.4f  ", first_loss, last_loss);
    ASSERT_LT(last_loss, first_loss, "Loss should decrease after 100 SGD steps");

    linear_free(layer);
    nova_tensor_destroy(X);
    nova_tensor_destroy(Y);
}

/* ============================================================
 * TEST 2: Two-layer MLP with ReLU — XOR task
 * ============================================================ */
static void test_two_layer_mlp_xor(void) {
    srand(123);
    const int BATCH = 4, IN = 2, HIDDEN = 8, OUT = 1;
    const float LR = 0.05f;
    const int STEPS = 500;

    /* XOR input/output */
    int64_t x_shape[] = {BATCH, IN};
    NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
    float xd[] = {0,0, 0,1, 1,0, 1,1};
    memcpy(X->data, xd, sizeof(xd));

    int64_t y_shape[] = {BATCH, OUT};
    NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
    float yd[] = {0, 1, 1, 0};
    memcpy(Y->data, yd, sizeof(yd));

    LinearLayer *l1 = linear_create(IN,     HIDDEN);
    LinearLayer *l2 = linear_create(HIDDEN, OUT);

    float first_loss = -1.0f, last_loss = -1.0f;

    for (int step = 0; step < STEPS; step++) {
        /* === Forward === */
        NovaTensor *h1_pre  = linear_forward(l1, X);
        NovaTensor *h1      = relu_forward(h1_pre);
        NovaTensor *pred    = linear_forward(l2, h1);

        float loss = mse_loss(pred, Y);
        if (step == 0) first_loss = loss;
        if (step == STEPS - 1) last_loss = loss;

        /* === Backward === */
        /* dL/dpred */
        NovaTensor *dL = mse_grad(pred, Y);

        /* Layer 2 backward */
        NovaTensor *dW2, *db2, *d_h1;
        linear_backward(l2, h1, dL, &dW2, &db2, &d_h1);

        /* ReLU backward */
        NovaTensor *d_h1_pre = relu_backward(d_h1, h1_pre);

        /* Layer 1 backward */
        NovaTensor *dW1, *db1, *d_x;
        linear_backward(l1, X, d_h1_pre, &dW1, &db1, &d_x);

        /* === SGD update === */
        sgd_update(l2->weight, dW2, LR);
        sgd_update(l2->bias,   db2, LR);
        sgd_update(l1->weight, dW1, LR);
        sgd_update(l1->bias,   db1, LR);

        /* Cleanup */
        nova_tensor_destroy(pred);
        nova_tensor_destroy(h1);
        nova_tensor_destroy(h1_pre);
        nova_tensor_destroy(dL);
        nova_tensor_destroy(dW2); nova_tensor_destroy(db2); nova_tensor_destroy(d_h1);
        nova_tensor_destroy(d_h1_pre);
        nova_tensor_destroy(dW1); nova_tensor_destroy(db1); nova_tensor_destroy(d_x);
    }

    printf("\n    first_loss=%.4f  last_loss=%.4f  ", first_loss, last_loss);
    ASSERT_LT(last_loss, first_loss, "MLP XOR loss should decrease over 500 steps");
    ASSERT_LT(last_loss, 0.25f, "MLP XOR final loss should be < 0.25");

    linear_free(l1);
    linear_free(l2);
    nova_tensor_destroy(X);
    nova_tensor_destroy(Y);
}

/* ============================================================
 * TEST 3: Linear regression convergence — weights approach
 *         known ground-truth values
 * ============================================================ */
static void test_linear_regression_convergence(void) {
    srand(7);
    /* y = 2*x0 + 3*x1 + 1   (known weights W=[2,3], b=1) */
    const int BATCH = 8, IN = 2, OUT = 1;
    const float LR = 0.005f;
    const int STEPS = 600;

    int64_t x_shape[] = {BATCH, IN};
    NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
    float xd[] = {1,2, 3,4, 5,6, 7,8,
                  2,1, 4,3, 6,5, 8,7};
    memcpy(X->data, xd, sizeof(xd));

    int64_t y_shape[] = {BATCH, OUT};
    NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
    /* y = 2*x0 + 3*x1 + 1 */
    float *xp = (float *)X->data;
    float *yp = (float *)Y->data;
    for (int i = 0; i < BATCH; i++)
        yp[i] = 2.0f * xp[i*2] + 3.0f * xp[i*2+1] + 1.0f;

    LinearLayer *layer = linear_create(IN, OUT);

    for (int step = 0; step < STEPS; step++) {
        NovaTensor *pred = linear_forward(layer, X);
        NovaTensor *dL   = mse_grad(pred, Y);
        NovaTensor *dW, *db, *d_inp;
        linear_backward(layer, X, dL, &dW, &db, &d_inp);
        sgd_update(layer->weight, dW, LR);
        sgd_update(layer->bias,   db,  LR);
        nova_tensor_destroy(pred);
        nova_tensor_destroy(dL);
        nova_tensor_destroy(dW);
        nova_tensor_destroy(db);
        nova_tensor_destroy(d_inp);
    }

    float *W = (float *)layer->weight->data;
    float *b = (float *)layer->bias->data;
    printf("\n    W=[%.3f, %.3f] b=%.3f  ", W[0], W[1], b[0]);
    ASSERT_NEAR(W[0], 2.0f, 0.22f, "Weight[0] should converge near 2.0");
    ASSERT_NEAR(W[1], 3.0f, 0.22f, "Weight[1] should converge near 3.0");
    ASSERT_NEAR(b[0], 1.0f, 0.25f, "Bias should converge near 1.0");

    linear_free(layer);
    nova_tensor_destroy(X);
    nova_tensor_destroy(Y);
}

/* ============================================================
 * TEST 4: Tensor operations used in training are correct
 * ============================================================ */
static void test_tensor_ops_correctness(void) {
    /* matmul: [2x3] @ [3x2] = [2x2] */
    int64_t sa[] = {2, 3}, sb[] = {3, 2};
    NovaTensor *A = nova_tensor_create(NULL, sa, 2, NOVA_DTYPE_FP32);
    NovaTensor *B = nova_tensor_create(NULL, sb, 2, NOVA_DTYPE_FP32);
    float ad[] = {1,2,3, 4,5,6};
    float bd[] = {7,8, 9,10, 11,12};
    memcpy(A->data, ad, sizeof(ad));
    memcpy(B->data, bd, sizeof(bd));

    NovaTensor *C = nova_op_matmul(A, B);
    float *cd = (float *)C->data;
    /* C[0,0] = 1*7 + 2*9 + 3*11 = 7+18+33 = 58 */
    /* C[0,1] = 1*8 + 2*10+ 3*12 = 8+20+36 = 64 */
    /* C[1,0] = 4*7 + 5*9 + 6*11 = 28+45+66=139 */
    /* C[1,1] = 4*8 + 5*10+ 6*12 = 32+50+72=154 */
    ASSERT_NEAR(cd[0], 58.0f,  0.01f, "matmul C[0,0]");
    ASSERT_NEAR(cd[1], 64.0f,  0.01f, "matmul C[0,1]");
    ASSERT_NEAR(cd[2], 139.0f, 0.01f, "matmul C[1,0]");
    ASSERT_NEAR(cd[3], 154.0f, 0.01f, "matmul C[1,1]");

    nova_tensor_destroy(A);
    nova_tensor_destroy(B);
    nova_tensor_destroy(C);
}

/* ============================================================
 * TEST 5: ReLU correctness
 * ============================================================ */
static void test_relu_correctness(void) {
    int64_t sh[] = {1, 6};
    NovaTensor *x = nova_tensor_create(NULL, sh, 2, NOVA_DTYPE_FP32);
    float xd[] = {-3, -1, 0, 1, 2, 5};
    memcpy(x->data, xd, sizeof(xd));
    NovaTensor *out = relu_forward(x);
    float *od = (float *)out->data;
    ASSERT_NEAR(od[0], 0.0f, 1e-6f, "ReLU(-3)=0");
    ASSERT_NEAR(od[1], 0.0f, 1e-6f, "ReLU(-1)=0");
    ASSERT_NEAR(od[2], 0.0f, 1e-6f, "ReLU(0)=0");
    ASSERT_NEAR(od[3], 1.0f, 1e-6f, "ReLU(1)=1");
    ASSERT_NEAR(od[4], 2.0f, 1e-6f, "ReLU(2)=2");
    ASSERT_NEAR(od[5], 5.0f, 1e-6f, "ReLU(5)=5");
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
}

/* ============================================================
 * TEST 6: MSE loss and gradient sanity
 * ============================================================ */
static void test_mse_loss_and_grad(void) {
    int64_t sh[] = {1, 4};
    NovaTensor *pred   = nova_tensor_create(NULL, sh, 2, NOVA_DTYPE_FP32);
    NovaTensor *target = nova_tensor_create(NULL, sh, 2, NOVA_DTYPE_FP32);
    float pd[] = {2, 3, 4, 5};
    float td[] = {1, 1, 1, 1};
    memcpy(pred->data,   pd, sizeof(pd));
    memcpy(target->data, td, sizeof(td));

    /* MSE = ((1^2 + 2^2 + 3^2 + 4^2) / 4) = (1+4+9+16)/4 = 7.5 */
    float loss = mse_loss(pred, target);
    ASSERT_NEAR(loss, 7.5f, 0.01f, "MSE loss = 7.5");

    NovaTensor *grad = mse_grad(pred, target);
    float *gd = (float *)grad->data;
    /* grad[0] = 2*(2-1)/4 = 0.5 */
    ASSERT_NEAR(gd[0], 0.5f, 0.01f, "MSE grad[0]");
    /* grad[1] = 2*(3-1)/4 = 1.0 */
    ASSERT_NEAR(gd[1], 1.0f, 0.01f, "MSE grad[1]");

    nova_tensor_destroy(pred);
    nova_tensor_destroy(target);
    nova_tensor_destroy(grad);
}

/* ============================================================
 * TEST 7: Xavier initialization check (weight variance)
 * ============================================================ */
static void test_xavier_init_variance(void) {
    srand(999);
    const int IN = 64, OUT = 64;
    LinearLayer *layer = linear_create(IN, OUT);
    float *W = (float *)layer->weight->data;
    size_t n = layer->weight->total_elements;

    float mean = 0.0f, var = 0.0f;
    for (size_t i = 0; i < n; i++) mean += W[i];
    mean /= (float)n;
    for (size_t i = 0; i < n; i++) var += (W[i]-mean)*(W[i]-mean);
    var /= (float)n;

    /* Xavier: Var ~ 2/(in+out) = 2/128 ≈ 0.0156 */
    float expected_var = 2.0f / (IN + OUT);
    printf("\n    var=%.4f expected~%.4f  ", var, expected_var);
    ASSERT_NEAR(var, expected_var, 0.01f, "Xavier init variance");

    linear_free(layer);
}

/* ============================================================
 * TEST 8: Training is deterministic with same seed
 * ============================================================ */
static void test_training_deterministic(void) {
    const int BATCH = 4, IN = 2, OUT = 1;
    const float LR = 0.01f;
    const int STEPS = 20;

    int64_t x_shape[] = {BATCH, IN};
    int64_t y_shape[] = {BATCH, OUT};

    float xd[] = {1,2, 3,4, 5,6, 7,8};
    float yd[] = {3, 7, 11, 15};

    float final_loss_run1, final_loss_run2;

    for (int run = 0; run < 2; run++) {
        srand(42);
        NovaTensor *X = nova_tensor_create(NULL, x_shape, 2, NOVA_DTYPE_FP32);
        NovaTensor *Y = nova_tensor_create(NULL, y_shape, 2, NOVA_DTYPE_FP32);
        memcpy(X->data, xd, sizeof(xd));
        memcpy(Y->data, yd, sizeof(yd));

        LinearLayer *layer = linear_create(IN, OUT);
        float loss = 0.0f;
        for (int step = 0; step < STEPS; step++) {
            NovaTensor *pred = linear_forward(layer, X);
            loss = mse_loss(pred, Y);
            NovaTensor *dL = mse_grad(pred, Y);
            NovaTensor *dW, *db, *d_inp;
            linear_backward(layer, X, dL, &dW, &db, &d_inp);
            sgd_update(layer->weight, dW, LR);
            sgd_update(layer->bias,   db,  LR);
            nova_tensor_destroy(pred); nova_tensor_destroy(dL);
            nova_tensor_destroy(dW); nova_tensor_destroy(db);
            nova_tensor_destroy(d_inp);
        }
        if (run == 0) final_loss_run1 = loss;
        else          final_loss_run2 = loss;
        linear_free(layer);
        nova_tensor_destroy(X);
        nova_tensor_destroy(Y);
    }

    printf("\n    run1=%.6f  run2=%.6f  ", final_loss_run1, final_loss_run2);
    ASSERT_NEAR(final_loss_run1, final_loss_run2, 1e-4f,
                "Training should be deterministic with same seed");
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   Nova ML — End-to-End Training Pipeline Tests          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    RUN(test_tensor_ops_correctness);
    RUN(test_relu_correctness);
    RUN(test_mse_loss_and_grad);
    RUN(test_xavier_init_variance);
    RUN(test_single_layer_loss_decreases);
    RUN(test_linear_regression_convergence);
    RUN(test_two_layer_mlp_xor);
    RUN(test_training_deterministic);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Results: %d/%d passed%-34s║\n",
           g_passed, g_passed + g_failed, "");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    return g_failed > 0 ? 1 : 0;
}
