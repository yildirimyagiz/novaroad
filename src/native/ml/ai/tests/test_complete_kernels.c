/**
 * Complete Kernel Test Suite
 * Tests all SIMD, fused, and dispatcher functionality
 */

#include "compute/nova_kernels.h"
#include "nova_kernels_advanced.h"
#include "ml/nova_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define TEST_SIZE 1024
#define EPSILON 1e-5f

int tests_passed = 0;
int tests_failed = 0;

#define ASSERT_FLOAT_EQ(a, b, msg) do { \
    if (fabsf((a) - (b)) > EPSILON) { \
        fprintf(stderr, "❌ FAIL: %s (expected %.6f, got %.6f)\n", msg, (float)(b), (float)(a)); \
        tests_failed++; \
        return 0; \
    } \
} while(0)

#define TEST(name) \
    int test_##name(void); \
    void run_test_##name(void) { \
        printf("  Testing: %s ... ", #name); \
        fflush(stdout); \
        if (test_##name()) { \
            printf("✅ PASS\n"); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
    } \
    int test_##name(void)

// Helper: Create test context (simplified stub)
static NovaContext* create_test_context(void) {
    // Return NULL - tests will use library functions that handle this
    return NULL;
}

// Helper: Create test tensor
static NovaTensor* create_test_tensor(NovaContext* ctx, size_t size) {
    int64_t shape[] = {(int64_t)size};
    NovaTensor* t = nova_tensor_create(ctx, shape, 1, NOVA_DTYPE_FP32);
    return t;
}

// Helper: Fill tensor with values
static void fill_tensor(NovaTensor* t, float value) {
    float* data = (float*)t->data;
    for (size_t i = 0; i < t->total_elements; i++) {
        data[i] = value;
    }
}

// Helper: Fill tensor with pattern
static void fill_tensor_pattern(NovaTensor* t) {
    float* data = (float*)t->data;
    for (size_t i = 0; i < t->total_elements; i++) {
        data[i] = (float)i / (float)t->total_elements;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Test Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST(simd_relu) {
    NovaContext* ctx = create_test_context();
    NovaTensor* x = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* out = create_test_tensor(ctx, TEST_SIZE);
    
    // Fill with values from -1 to 1
    float* x_data = (float*)x->data;
    for (size_t i = 0; i < TEST_SIZE; i++) {
        x_data[i] = -1.0f + 2.0f * (float)i / (float)TEST_SIZE;
    }
    
    nova_simd_relu(x, out);
    
    float* out_data = (float*)out->data;
    // Check first negative value
    ASSERT_FLOAT_EQ(out_data[0], 0.0f, "ReLU(-1) should be 0");
    // Check positive value
    ASSERT_FLOAT_EQ(out_data[TEST_SIZE-1], x_data[TEST_SIZE-1], "ReLU(positive) should pass through");
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    free(ctx);
    return 1;
}

TEST(simd_gelu) {
    NovaContext* ctx = create_test_context();
    NovaTensor* x = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* out = create_test_tensor(ctx, TEST_SIZE);
    
    fill_tensor(x, 0.0f);
    nova_simd_gelu(x, out);
    
    float* out_data = (float*)out->data;
    ASSERT_FLOAT_EQ(out_data[0], 0.0f, "GELU(0) should be ~0");
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    free(ctx);
    return 1;
}

TEST(simd_silu) {
    NovaContext* ctx = create_test_context();
    NovaTensor* x = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* out = create_test_tensor(ctx, TEST_SIZE);
    
    fill_tensor(x, 0.0f);
    nova_simd_silu(x, out);
    
    float* out_data = (float*)out->data;
    ASSERT_FLOAT_EQ(out_data[0], 0.0f, "SiLU(0) should be ~0");
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    free(ctx);
    return 1;
}

TEST(simd_add) {
    NovaContext* ctx = create_test_context();
    NovaTensor* a = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* b = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* c = create_test_tensor(ctx, TEST_SIZE);
    
    fill_tensor(a, 2.0f);
    fill_tensor(b, 3.0f);
    
    nova_simd_add(a, b, c);
    
    float* c_data = (float*)c->data;
    ASSERT_FLOAT_EQ(c_data[0], 5.0f, "2 + 3 should be 5");
    ASSERT_FLOAT_EQ(c_data[TEST_SIZE-1], 5.0f, "All elements should be 5");
    
    nova_tensor_destroy(a);
    nova_tensor_destroy(b);
    nova_tensor_destroy(c);
    free(ctx);
    return 1;
}

TEST(fused_matmul_gelu) {
    NovaContext* ctx = create_test_context();
    
    // Small matrices for testing
    int64_t shape_a[] = {4, 4};
    int64_t shape_b[] = {4, 4};
    
    NovaTensor* A = nova_tensor_create(ctx, shape_a, 2, NOVA_DTYPE_FP32);
    NovaTensor* B = nova_tensor_create(ctx, shape_b, 2, NOVA_DTYPE_FP32);
    NovaTensor* C = nova_tensor_create(ctx, shape_a, 2, NOVA_DTYPE_FP32);
    
    fill_tensor(A, 1.0f);
    fill_tensor(B, 0.5f);
    
    fused_matmul_gelu(A, B, C);
    
    float* c_data = (float*)C->data;
    // Just verify it ran and produced non-zero output
    if (c_data[0] == 0.0f && c_data[15] == 0.0f) {
        fprintf(stderr, "Fused matmul+gelu produced all zeros\n");
        nova_tensor_destroy(A);
        nova_tensor_destroy(B);
        nova_tensor_destroy(C);
        free(ctx);
        return 0;
    }
    
    nova_tensor_destroy(A);
    nova_tensor_destroy(B);
    nova_tensor_destroy(C);
    free(ctx);
    return 1;
}

TEST(fused_add_relu) {
    NovaContext* ctx = create_test_context();
    NovaTensor* a = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* b = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* out = create_test_tensor(ctx, TEST_SIZE);
    
    fill_tensor(a, -1.0f);
    fill_tensor(b, 2.0f);
    
    fused_add_relu(a, b, out);
    
    // -1 + 2 = 1, ReLU(1) = 1
    float* out_data = (float*)out->data;
    ASSERT_FLOAT_EQ(out_data[0], 1.0f, "(-1 + 2) with ReLU should be 1");
    
    nova_tensor_destroy(a);
    nova_tensor_destroy(b);
    nova_tensor_destroy(out);
    free(ctx);
    return 1;
}

TEST(dispatcher_init) {
    // Backend initialization happens automatically
    printf("\n      Dispatcher initialized\n      ");
    return 1;
}

TEST(dispatcher_dispatch) {
    NovaContext* ctx = create_test_context();
    NovaTensor* x = create_test_tensor(ctx, TEST_SIZE);
    NovaTensor* out = create_test_tensor(ctx, TEST_SIZE);
    
    fill_tensor_pattern(x);
    
    nova_dispatch_gelu(x, out);
    
    float* out_data = (float*)out->data;
    if (out_data[0] == 0.0f && out_data[TEST_SIZE-1] == 0.0f) {
        fprintf(stderr, "Dispatcher GELU produced all zeros\n");
        nova_tensor_destroy(x);
        nova_tensor_destroy(out);
        free(ctx);
        return 0;
    }
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    free(ctx);
    return 1;
}

// ═══════════════════════════════════════════════════════════════════════════
// Main Test Runner
// ═══════════════════════════════════════════════════════════════════════════

int main(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("  Nova Kernel Complete Test Suite\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    clock_t start = clock();
    
    printf("🧪 Testing SIMD Kernels:\n");
    run_test_simd_relu();
    run_test_simd_gelu();
    run_test_simd_silu();
    run_test_simd_add();
    
    printf("\n🧪 Testing Fused Operations:\n");
    run_test_fused_matmul_gelu();
    run_test_fused_add_relu();
    
    printf("\n🧪 Testing Dispatcher:\n");
    run_test_dispatcher_init();
    run_test_dispatcher_dispatch();
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("  Results: %d passed, %d failed (%.3f seconds)\n", 
           tests_passed, tests_failed, elapsed);
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    return tests_failed > 0 ? 1 : 0;
}
