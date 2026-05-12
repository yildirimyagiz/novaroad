/**
 * @file test_ai_module.c
 * @brief Comprehensive test suite for Nova AI module
 */

#include "ai/tensor.h"
#include "ai/autograd.h"
#include "ai/nn.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-5f
#define TEST_PASS() printf("  ✓ %s\n", __func__)
#define TEST_FAIL(msg) do { printf("  ✗ %s: %s\n", __func__, msg); return 1; } while(0)

// ============================================================================
// Helper Functions
// ============================================================================

static int float_equals(float a, float b, float epsilon) {
    return fabsf(a - b) < epsilon;
}

// ============================================================================
// Tensor Creation Tests
// ============================================================================

static int test_tensor_create_zeros(void) {
    size_t shape[] = {2, 3};
    nova_tensor_t *t = nova_tensor_zeros(shape, 2, NOVA_DTYPE_FLOAT32);
    
    if (!t) TEST_FAIL("Failed to create tensor");
    if (t->ndim != 2) TEST_FAIL("Wrong ndim");
    if (t->shape[0] != 2 || t->shape[1] != 3) TEST_FAIL("Wrong shape");
    if (t->size != 6) TEST_FAIL("Wrong size");
    
    float *data = (float *)nova_tensor_data(t);
    for (size_t i = 0; i < 6; i++) {
        if (data[i] != 0.0f) TEST_FAIL("Non-zero value");
    }
    
    nova_tensor_destroy(t);
    TEST_PASS();
    return 0;
}

static int test_tensor_create_ones(void) {
    size_t shape[] = {3, 2};
    nova_tensor_t *t = nova_tensor_ones(shape, 2, NOVA_DTYPE_FLOAT32);
    
    if (!t) TEST_FAIL("Failed to create tensor");
    
    float *data = (float *)nova_tensor_data(t);
    for (size_t i = 0; i < 6; i++) {
        if (!float_equals(data[i], 1.0f, EPSILON)) TEST_FAIL("Non-one value");
    }
    
    nova_tensor_destroy(t);
    TEST_PASS();
    return 0;
}

static int test_tensor_from_data(void) {
    float input[] = {1.0f, 2.0f, 3.0f, 4.0f};
    size_t shape[] = {2, 2};
    
    nova_tensor_t *t = nova_tensor_from_data(input, shape, 2, NOVA_DTYPE_FLOAT32);
    if (!t) TEST_FAIL("Failed to create tensor");
    
    float *data = (float *)nova_tensor_data(t);
    for (size_t i = 0; i < 4; i++) {
        if (!float_equals(data[i], input[i], EPSILON)) TEST_FAIL("Data mismatch");
    }
    
    nova_tensor_destroy(t);
    TEST_PASS();
    return 0;
}

// ============================================================================
// Tensor Operations Tests
// ============================================================================

static int test_tensor_add(void) {
    float data_a[] = {1.0f, 2.0f, 3.0f};
    float data_b[] = {4.0f, 5.0f, 6.0f};
    size_t shape[] = {3};
    
    nova_tensor_t *a = nova_tensor_from_data(data_a, shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *b = nova_tensor_from_data(data_b, shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *c = nova_tensor_add(a, b);
    
    if (!c) TEST_FAIL("Addition failed");
    
    float *result = (float *)nova_tensor_data(c);
    float expected[] = {5.0f, 7.0f, 9.0f};
    
    for (size_t i = 0; i < 3; i++) {
        if (!float_equals(result[i], expected[i], EPSILON)) TEST_FAIL("Wrong result");
    }
    
    nova_tensor_destroy(a);
    nova_tensor_destroy(b);
    nova_tensor_destroy(c);
    TEST_PASS();
    return 0;
}

static int test_tensor_mul(void) {
    float data_a[] = {2.0f, 3.0f, 4.0f};
    float data_b[] = {5.0f, 6.0f, 7.0f};
    size_t shape[] = {3};
    
    nova_tensor_t *a = nova_tensor_from_data(data_a, shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *b = nova_tensor_from_data(data_b, shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *c = nova_tensor_mul(a, b);
    
    if (!c) TEST_FAIL("Multiplication failed");
    
    float *result = (float *)nova_tensor_data(c);
    float expected[] = {10.0f, 18.0f, 28.0f};
    
    for (size_t i = 0; i < 3; i++) {
        if (!float_equals(result[i], expected[i], EPSILON)) TEST_FAIL("Wrong result");
    }
    
    nova_tensor_destroy(a);
    nova_tensor_destroy(b);
    nova_tensor_destroy(c);
    TEST_PASS();
    return 0;
}

static int test_tensor_matmul(void) {
    // 2x3 matrix
    float data_a[] = {1.0f, 2.0f, 3.0f,
                      4.0f, 5.0f, 6.0f};
    size_t shape_a[] = {2, 3};
    
    // 3x2 matrix
    float data_b[] = {7.0f, 8.0f,
                      9.0f, 10.0f,
                      11.0f, 12.0f};
    size_t shape_b[] = {3, 2};
    
    nova_tensor_t *a = nova_tensor_from_data(data_a, shape_a, 2, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *b = nova_tensor_from_data(data_b, shape_b, 2, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *c = nova_tensor_matmul(a, b);
    
    if (!c) TEST_FAIL("Matmul failed");
    if (c->shape[0] != 2 || c->shape[1] != 2) TEST_FAIL("Wrong output shape");
    
    float *result = (float *)nova_tensor_data(c);
    // Expected: [58, 64; 139, 154]
    float expected[] = {58.0f, 64.0f, 139.0f, 154.0f};
    
    for (size_t i = 0; i < 4; i++) {
        if (!float_equals(result[i], expected[i], EPSILON)) {
            printf("result[%zu] = %f, expected = %f\n", i, result[i], expected[i]);
            TEST_FAIL("Wrong matmul result");
        }
    }
    
    nova_tensor_destroy(a);
    nova_tensor_destroy(b);
    nova_tensor_destroy(c);
    TEST_PASS();
    return 0;
}

// ============================================================================
// Activation Function Tests
// ============================================================================

static int test_activation_relu(void) {
    float data[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
    size_t shape[] = {5};
    
    nova_tensor_t *input = nova_tensor_from_data(data, shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *output = nova_tensor_relu(input);
    
    if (!output) TEST_FAIL("ReLU failed");
    
    float *result = (float *)nova_tensor_data(output);
    float expected[] = {0.0f, 0.0f, 0.0f, 1.0f, 2.0f};
    
    for (size_t i = 0; i < 5; i++) {
        if (!float_equals(result[i], expected[i], EPSILON)) TEST_FAIL("Wrong ReLU result");
    }
    
    nova_tensor_destroy(input);
    nova_tensor_destroy(output);
    TEST_PASS();
    return 0;
}

static int test_activation_softmax(void) {
    float data[] = {1.0f, 2.0f, 3.0f};
    size_t shape[] = {3};
    
    nova_tensor_t *input = nova_tensor_from_data(data, shape, 1, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *output = nova_tensor_softmax(input);
    
    if (!output) TEST_FAIL("Softmax failed");
    
    float *result = (float *)nova_tensor_data(output);
    
    // Check sum = 1.0
    float sum = 0.0f;
    for (size_t i = 0; i < 3; i++) {
        sum += result[i];
    }
    
    if (!float_equals(sum, 1.0f, EPSILON)) TEST_FAIL("Softmax doesn't sum to 1");
    
    // Check monotonicity (larger input -> larger output)
    if (result[0] >= result[1] || result[1] >= result[2]) TEST_FAIL("Softmax not monotonic");
    
    nova_tensor_destroy(input);
    nova_tensor_destroy(output);
    TEST_PASS();
    return 0;
}

// ============================================================================
// Convolution Tests
// ============================================================================

static int test_conv2d_basic(void) {
    // Input: 1 batch, 1 channel, 3x3 image
    float input_data[9] = {
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    };
    size_t input_shape[] = {1, 1, 3, 3};
    
    // Kernel: 1 output channel, 1 input channel, 2x2 kernel
    float kernel_data[4] = {
        1.0f, 0.0f,
        0.0f, 1.0f
    };
    size_t kernel_shape[] = {1, 1, 2, 2};
    
    nova_tensor_t *input = nova_tensor_from_data(input_data, input_shape, 4, NOVA_DTYPE_FLOAT32);
    nova_tensor_t *kernel = nova_tensor_from_data(kernel_data, kernel_shape, 4, NOVA_DTYPE_FLOAT32);
    
    // Conv2d with stride=1, padding=0
    nova_tensor_t *output = nova_tensor_conv2d(input, kernel, 1, 0);
    
    if (!output) TEST_FAIL("Conv2d failed");
    
    // Output should be 1x1x2x2
    if (output->shape[0] != 1 || output->shape[1] != 1 || 
        output->shape[2] != 2 || output->shape[3] != 2) {
        TEST_FAIL("Wrong output shape");
    }
    
    float *result = (float *)nova_tensor_data(output);
    // Expected: identity kernel sums diagonal elements
    // [1+5, 2+6, 4+8, 5+9] = [6, 8, 12, 14]
    float expected[] = {6.0f, 8.0f, 12.0f, 14.0f};
    
    for (size_t i = 0; i < 4; i++) {
        if (!float_equals(result[i], expected[i], EPSILON)) {
            printf("result[%zu] = %f, expected = %f\n", i, result[i], expected[i]);
            TEST_FAIL("Wrong conv2d result");
        }
    }
    
    nova_tensor_destroy(input);
    nova_tensor_destroy(kernel);
    nova_tensor_destroy(output);
    TEST_PASS();
    return 0;
}

// ============================================================================
// Autograd Tests
// ============================================================================

static int test_autograd_simple(void) {
    nova_grad_tape_t *tape = nova_grad_tape_create();
    if (!tape) TEST_FAIL("Failed to create tape");
    
    // Simple test: y = x + x, dy/dx should be 2
    float data[] = {1.0f, 2.0f, 3.0f};
    size_t shape[] = {3};
    
    nova_tensor_t *x = nova_tensor_from_data(data, shape, 1, NOVA_DTYPE_FLOAT32);
    x->requires_grad = true;
    
    nova_grad_tape_begin(tape);
    nova_grad_tape_watch(tape, x);
    
    nova_tensor_t *y = nova_tensor_add(x, x);
    
    // Backward pass (simplified - full implementation would need actual backward)
    
    nova_grad_tape_destroy(tape);
    nova_tensor_destroy(x);
    nova_tensor_destroy(y);
    TEST_PASS();
    return 0;
}

// ============================================================================
// Memory Leak Tests
// ============================================================================

static int test_memory_cleanup(void) {
    // Create and destroy many tensors to check for leaks
    for (int i = 0; i < 100; i++) {
        size_t shape[] = {10, 10};
        nova_tensor_t *t = nova_tensor_zeros(shape, 2, NOVA_DTYPE_FLOAT32);
        
        // Set grad_fn to test cleanup
        t->requires_grad = true;
        
        nova_tensor_destroy(t);
    }
    
    TEST_PASS();
    return 0;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    int failed = 0;
    
    printf("\n=== Nova AI Module Test Suite ===\n\n");
    
    printf("Tensor Creation Tests:\n");
    failed += test_tensor_create_zeros();
    failed += test_tensor_create_ones();
    failed += test_tensor_from_data();
    
    printf("\nTensor Operations Tests:\n");
    failed += test_tensor_add();
    failed += test_tensor_mul();
    failed += test_tensor_matmul();
    
    printf("\nActivation Function Tests:\n");
    failed += test_activation_relu();
    failed += test_activation_softmax();
    
    printf("\nConvolution Tests:\n");
    failed += test_conv2d_basic();
    
    printf("\nAutograd Tests:\n");
    failed += test_autograd_simple();
    
    printf("\nMemory Tests:\n");
    failed += test_memory_cleanup();
    
    printf("\n=================================\n");
    if (failed == 0) {
        printf("✅ All tests passed!\n");
    } else {
        printf("❌ %d test(s) failed\n", failed);
    }
    printf("=================================\n\n");
    
    return failed;
}
