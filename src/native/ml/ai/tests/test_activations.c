#include "ml/nova_tensor.h"
#include "compute/nova_kernels.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA ACTIVATION FUNCTIONS TEST SUITE
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define ASSERT_NEAR(a, b, epsilon) \
    if (fabsf((a) - (b)) > (epsilon)) { \
        printf("❌ FAILED: %f != %f (diff: %f)\n", (a), (b), fabsf((a) - (b))); \
        return 1; \
    }

int test_relu() {
    printf("🧪 Testing ReLU...\n");
    
    int64_t shape[] = {4};
    NovaTensor *x = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    float *x_data = (float *)x->data;
    x_data[0] = -2.0f;
    x_data[1] = -0.5f;
    x_data[2] = 0.5f;
    x_data[3] = 2.0f;
    
    nova_kernel_relu(x, out);
    
    float *out_data = (float *)out->data;
    ASSERT_NEAR(out_data[0], 0.0f, 1e-6f);
    ASSERT_NEAR(out_data[1], 0.0f, 1e-6f);
    ASSERT_NEAR(out_data[2], 0.5f, 1e-6f);
    ASSERT_NEAR(out_data[3], 2.0f, 1e-6f);
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    printf("✅ ReLU passed\n");
    return 0;
}

int test_sigmoid() {
    printf("🧪 Testing Sigmoid...\n");
    
    int64_t shape[] = {3};
    NovaTensor *x = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    float *x_data = (float *)x->data;
    x_data[0] = 0.0f;
    x_data[1] = 1.0f;
    x_data[2] = -1.0f;
    
    nova_kernel_sigmoid(x, out);
    
    float *out_data = (float *)out->data;
    ASSERT_NEAR(out_data[0], 0.5f, 1e-5f);
    ASSERT_NEAR(out_data[1], 0.7311f, 1e-3f); // sigmoid(1) ≈ 0.7311
    ASSERT_NEAR(out_data[2], 0.2689f, 1e-3f); // sigmoid(-1) ≈ 0.2689
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    printf("✅ Sigmoid passed\n");
    return 0;
}

int test_gelu() {
    printf("🧪 Testing GELU...\n");
    
    int64_t shape[] = {3};
    NovaTensor *x = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    float *x_data = (float *)x->data;
    x_data[0] = 0.0f;
    x_data[1] = 1.0f;
    x_data[2] = -1.0f;
    
    nova_kernel_gelu(x, out);
    
    float *out_data = (float *)out->data;
    ASSERT_NEAR(out_data[0], 0.0f, 1e-5f);      // GELU(0) = 0
    ASSERT_NEAR(out_data[1], 0.8411f, 1e-2f);   // GELU(1) ≈ 0.8411
    ASSERT_NEAR(out_data[2], -0.1589f, 1e-2f);  // GELU(-1) ≈ -0.1589
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    printf("✅ GELU passed\n");
    return 0;
}

int test_silu() {
    printf("🧪 Testing SiLU...\n");
    
    int64_t shape[] = {3};
    NovaTensor *x = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    float *x_data = (float *)x->data;
    x_data[0] = 0.0f;
    x_data[1] = 1.0f;
    x_data[2] = -1.0f;
    
    nova_kernel_silu(x, out);
    
    float *out_data = (float *)out->data;
    ASSERT_NEAR(out_data[0], 0.0f, 1e-5f);      // SiLU(0) = 0
    ASSERT_NEAR(out_data[1], 0.7311f, 1e-3f);   // SiLU(1) = 1 * sigmoid(1)
    ASSERT_NEAR(out_data[2], -0.2689f, 1e-3f);  // SiLU(-1) = -1 * sigmoid(-1)
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    printf("✅ SiLU passed\n");
    return 0;
}

int test_clamp() {
    printf("🧪 Testing Clamp...\n");
    
    int64_t shape[] = {5};
    NovaTensor *x = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    float *x_data = (float *)x->data;
    x_data[0] = -2.0f;
    x_data[1] = -0.5f;
    x_data[2] = 0.0f;
    x_data[3] = 0.5f;
    x_data[4] = 2.0f;
    
    nova_kernel_clamp(x, -1.0f, 1.0f, out);
    
    float *out_data = (float *)out->data;
    ASSERT_NEAR(out_data[0], -1.0f, 1e-6f);
    ASSERT_NEAR(out_data[1], -0.5f, 1e-6f);
    ASSERT_NEAR(out_data[2], 0.0f, 1e-6f);
    ASSERT_NEAR(out_data[3], 0.5f, 1e-6f);
    ASSERT_NEAR(out_data[4], 1.0f, 1e-6f);
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    printf("✅ Clamp passed\n");
    return 0;
}

int test_pow() {
    printf("🧪 Testing Pow...\n");
    
    int64_t shape[] = {3};
    NovaTensor *x = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    float *x_data = (float *)x->data;
    x_data[0] = 2.0f;
    x_data[1] = 3.0f;
    x_data[2] = 4.0f;
    
    nova_kernel_pow(x, 2.0f, out);
    
    float *out_data = (float *)out->data;
    ASSERT_NEAR(out_data[0], 4.0f, 1e-5f);
    ASSERT_NEAR(out_data[1], 9.0f, 1e-5f);
    ASSERT_NEAR(out_data[2], 16.0f, 1e-5f);
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    printf("✅ Pow passed\n");
    return 0;
}

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║        NOVA ACTIVATION FUNCTIONS TEST SUITE                ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    int result = 0;
    
    result |= test_relu();
    result |= test_sigmoid();
    result |= test_gelu();
    result |= test_silu();
    result |= test_clamp();
    result |= test_pow();
    
    printf("\n");
    if (result == 0) {
        printf("🎉 ALL TESTS PASSED! (%d tests)\n", 6);
    } else {
        printf("❌ SOME TESTS FAILED!\n");
    }
    printf("\n");
    
    return result;
}
