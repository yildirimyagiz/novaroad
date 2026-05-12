/**
 * test_nova_metal_accelerate.c - Unit tests for Apple Accelerate Metal backend
 */

#ifdef __APPLE__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Forward declarations */
int64_t nova_metal_init(void);
int64_t nova_metal_get_device_count(void);
void nova_metal_cleanup(void);
void nova_metal_print_info(void);
int64_t nova_metal_add(const float *a, const float *b, float *c, int64_t count);
int64_t nova_metal_mul(const float *a, const float *b, float *c, int64_t count);
int64_t nova_metal_relu(const float *x, float *out, int64_t count);
int64_t nova_metal_softmax(const float *x, float *out, int64_t count);
int64_t nova_metal_matmul(const float *A, const float *B, float *C,
                           int64_t M, int64_t N, int64_t K);

#define EPSILON 1e-5f
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { printf("FAIL: %s\n", msg); return 0; } } while(0)
#define ASSERT_FLOAT_EQ(a, b, msg) do { if (fabsf((a) - (b)) > EPSILON) { printf("FAIL: %s (got %.6f, expected %.6f)\n", msg, a, b); return 0; } } while(0)

/* Test vector addition */
static int test_vector_add(void) {
    printf("Testing vector addition...\n");
    
    float a[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[] = {5.0f, 6.0f, 7.0f, 8.0f};
    float c[4];
    float expected[] = {6.0f, 8.0f, 10.0f, 12.0f};
    
    int64_t result = nova_metal_add(a, b, c, 4);
    ASSERT_EQ(result, 0, "nova_metal_add should return 0");
    
    for (int i = 0; i < 4; i++) {
        ASSERT_FLOAT_EQ(c[i], expected[i], "vector_add element mismatch");
    }
    
    printf("  PASS: vector addition\n");
    return 1;
}

/* Test element-wise multiplication */
static int test_vector_mul(void) {
    printf("Testing element-wise multiplication...\n");
    
    float a[] = {2.0f, 3.0f, 4.0f, 5.0f};
    float b[] = {2.0f, 3.0f, 4.0f, 5.0f};
    float c[4];
    float expected[] = {4.0f, 9.0f, 16.0f, 25.0f};
    
    int64_t result = nova_metal_mul(a, b, c, 4);
    ASSERT_EQ(result, 0, "nova_metal_mul should return 0");
    
    for (int i = 0; i < 4; i++) {
        ASSERT_FLOAT_EQ(c[i], expected[i], "vector_mul element mismatch");
    }
    
    printf("  PASS: element-wise multiplication\n");
    return 1;
}

/* Test matrix multiplication */
static int test_matmul(void) {
    printf("Testing matrix multiplication...\n");
    
    /* 2x2 * 2x2 = 2x2
       A = [[1, 2],
            [3, 4]]
       B = [[5, 6],
            [7, 8]]
       C = A*B = [[1*5+2*7, 1*6+2*8],
                   [3*5+4*7, 3*6+4*8]]
              = [[19, 22],
                 [43, 50]]
    */
    float A[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float B[] = {5.0f, 6.0f, 7.0f, 8.0f};
    float C[4];
    float expected[] = {19.0f, 22.0f, 43.0f, 50.0f};
    
    int64_t result = nova_metal_matmul(A, B, C, 2, 2, 2);
    ASSERT_EQ(result, 0, "nova_metal_matmul should return 0");
    
    for (int i = 0; i < 4; i++) {
        ASSERT_FLOAT_EQ(C[i], expected[i], "matmul element mismatch");
    }
    
    printf("  PASS: matrix multiplication\n");
    return 1;
}

/* Test ReLU activation */
static int test_relu(void) {
    printf("Testing ReLU activation...\n");
    
    float x[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 2.0f};
    float out[6];
    float expected[] = {0.0f, 0.0f, 0.0f, 0.5f, 1.0f, 2.0f};
    
    int64_t result = nova_metal_relu(x, out, 6);
    ASSERT_EQ(result, 0, "nova_metal_relu should return 0");
    
    for (int i = 0; i < 6; i++) {
        ASSERT_FLOAT_EQ(out[i], expected[i], "ReLU element mismatch");
    }
    
    printf("  PASS: ReLU activation\n");
    return 1;
}

/* Test softmax normalization */
static int test_softmax(void) {
    printf("Testing softmax normalization...\n");
    
    float x[] = {1.0f, 2.0f, 3.0f};
    float out[3];
    
    int64_t result = nova_metal_softmax(x, out, 3);
    ASSERT_EQ(result, 0, "nova_metal_softmax should return 0");
    
    /* Check that sum is approximately 1.0 */
    float sum = out[0] + out[1] + out[2];
    ASSERT_FLOAT_EQ(sum, 1.0f, "softmax should sum to 1.0");
    
    /* Check that all values are in [0, 1] */
    for (int i = 0; i < 3; i++) {
        if (out[i] < 0.0f || out[i] > 1.0f) {
            printf("FAIL: softmax output out of range: %f\n", out[i]);
            return 0;
        }
    }
    
    /* Check that larger input produces larger output */
    if (out[2] <= out[1] || out[1] <= out[0]) {
        printf("FAIL: softmax should preserve ordering\n");
        return 0;
    }
    
    printf("  PASS: softmax normalization\n");
    return 1;
}

/* Test initialization */
static int test_init(void) {
    printf("Testing initialization...\n");
    
    int64_t result = nova_metal_init();
    ASSERT_EQ(result, 1, "nova_metal_init should return 1 (available)");
    
    int64_t device_count = nova_metal_get_device_count();
    ASSERT_EQ(device_count, 1, "nova_metal_get_device_count should return 1");
    
    printf("  PASS: initialization\n");
    return 1;
}

/* Run all tests */
int main(void) {
    printf("========================================\n");
    printf("Nova Metal Accelerate Backend Tests\n");
    printf("========================================\n\n");
    
    nova_metal_print_info();
    printf("\n");
    
    int tests_passed = 0;
    int tests_failed = 0;
    
    if (test_init()) tests_passed++;
    else tests_failed++;
    
    if (test_vector_add()) tests_passed++;
    else tests_failed++;
    
    if (test_vector_mul()) tests_passed++;
    else tests_failed++;
    
    if (test_matmul()) tests_passed++;
    else tests_failed++;
    
    if (test_relu()) tests_passed++;
    else tests_failed++;
    
    if (test_softmax()) tests_passed++;
    else tests_failed++;
    
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");
    
    nova_metal_cleanup();
    
    return tests_failed == 0 ? 0 : 1;
}

#else /* __APPLE__ */

int main(void) {
    printf("Metal Accelerate tests require macOS platform\n");
    return 0;
}

#endif /* __APPLE__ */
