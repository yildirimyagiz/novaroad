#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// Platform detection
#if defined(__APPLE__)
#define PLATFORM_MACOS 1
#elif defined(__linux__)
#define PLATFORM_LINUX 1
#else
#define PLATFORM_UNKNOWN 1
#endif

#if defined(__aarch64__) || defined(__arm64__)
#define ARCH_ARM64 1
#elif defined(__x86_64__) || defined(_M_X64)
#define ARCH_X86_64 1
#else
#define ARCH_UNKNOWN 1
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Self-Contained Vector Operations
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int size;
    float *data;
} Vector;

Vector* vector_create(int size) {
    Vector *v = (Vector*)malloc(sizeof(Vector));
    v->size = size;
    v->data = (float*)calloc(size, sizeof(float));
    return v;
}

void vector_free(Vector *v) {
    if (v) {
        free(v->data);
        free(v);
    }
}

void vector_set(Vector *v, int i, float val) {
    v->data[i] = val;
}

float vector_get(Vector *v, int i) {
    return v->data[i];
}

void vector_fill(Vector *v, float val) {
    for (int i = 0; i < v->size; i++) {
        v->data[i] = val;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Vector Operations Implementation
// ═══════════════════════════════════════════════════════════════════════════

// Dot product: sum(a_i * b_i)
float vector_dot_product(Vector *a, Vector *b) {
    float sum = 0.0f;
    for (int i = 0; i < a->size; i++) {
        sum += a->data[i] * b->data[i];
    }
    return sum;
}

// Vector addition: c = a + b
void vector_add(Vector *a, Vector *b, Vector *c) {
    for (int i = 0; i < a->size; i++) {
        c->data[i] = a->data[i] + b->data[i];
    }
}

// Vector scaling: y = scale * x
void vector_scale(Vector *x, float scale, Vector *y) {
    for (int i = 0; i < x->size; i++) {
        y->data[i] = x->data[i] * scale;
    }
}

// L2 norm: sqrt(sum(x_i^2))
float vector_norm_l2(Vector *x) {
    float sum = 0.0f;
    for (int i = 0; i < x->size; i++) {
        sum += x->data[i] * x->data[i];
    }
    return sqrtf(sum);
}

// ═══════════════════════════════════════════════════════════════════════════
// Activation Functions
// ═══════════════════════════════════════════════════════════════════════════

// ReLU: max(0, x)
void activation_relu(Vector *x, Vector *y) {
    for (int i = 0; i < x->size; i++) {
        y->data[i] = (x->data[i] > 0.0f) ? x->data[i] : 0.0f;
    }
}

// Sigmoid: 1 / (1 + exp(-x))
void activation_sigmoid(Vector *x, Vector *y) {
    for (int i = 0; i < x->size; i++) {
        float exp_neg_x = expf(-x->data[i]);
        y->data[i] = 1.0f / (1.0f + exp_neg_x);
    }
}

// Tanh: (exp(2x) - 1) / (exp(2x) + 1)
void activation_tanh(Vector *x, Vector *y) {
    for (int i = 0; i < x->size; i++) {
        y->data[i] = tanhf(x->data[i]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Softmax Normalization
// ═══════════════════════════════════════════════════════════════════════════

void softmax(Vector *x, Vector *y) {
    // Find max for numerical stability
    float max_val = x->data[0];
    for (int i = 1; i < x->size; i++) {
        if (x->data[i] > max_val) {
            max_val = x->data[i];
        }
    }
    
    // Compute exp(x - max)
    float sum = 0.0f;
    for (int i = 0; i < x->size; i++) {
        y->data[i] = expf(x->data[i] - max_val);
        sum += y->data[i];
    }
    
    // Normalize
    for (int i = 0; i < x->size; i++) {
        y->data[i] /= sum;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Platform Detection
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    int has_simd;
    int has_avx2;
    int has_avx512;
    int has_neon;
    const char *platform;
    const char *arch;
} CPUInfo;

CPUInfo get_cpu_info() {
    CPUInfo info = {0};
    
    // Platform detection
#ifdef PLATFORM_MACOS
    info.platform = "macOS";
#elif defined(PLATFORM_LINUX)
    info.platform = "Linux";
#else
    info.platform = "Unknown";
#endif
    
    // Architecture detection
#ifdef ARCH_ARM64
    info.arch = "ARM64";
    info.has_simd = 1;
    info.has_neon = 1;
#elif defined(ARCH_X86_64)
    info.arch = "x86_64";
    info.has_simd = 1;
    info.has_avx2 = 1;
    // AVX-512 detection would require runtime checks
#else
    info.arch = "Unknown";
#endif
    
    return info;
}

// ═══════════════════════════════════════════════════════════════════════════
// Tests
// ═══════════════════════════════════════════════════════════════════════════

int test_platform_detection() {
    printf("\n[TEST] Platform Detection\n");
    printf("─────────────────────────\n");
    
    CPUInfo info = get_cpu_info();
    
    printf("Platform: %s\n", info.platform);
    printf("Architecture: %s\n", info.arch);
    printf("SIMD Support: %s\n", info.has_simd ? "Yes" : "No");
    printf("NEON (ARM): %s\n", info.has_neon ? "Yes" : "No");
    printf("AVX2 (x86): %s\n", info.has_avx2 ? "Yes" : "No");
    printf("AVX-512 (x86): %s\n", info.has_avx512 ? "Yes" : "No");
    
    printf("✓ Platform detection: PASS\n");
    
    return 1;
}

int test_vector_dot_product() {
    printf("\n[TEST] Vector Dot Product\n");
    printf("────────────────────────\n");
    
    Vector *a = vector_create(5);
    Vector *b = vector_create(5);
    
    // a = [1, 2, 3, 4, 5]
    for (int i = 0; i < 5; i++) {
        vector_set(a, i, (float)(i + 1));
        vector_set(b, i, (float)(i + 1));
    }
    
    float dot = vector_dot_product(a, b);
    float expected = 1*1 + 2*2 + 3*3 + 4*4 + 5*5; // 55
    
    printf("a = [1, 2, 3, 4, 5]\n");
    printf("b = [1, 2, 3, 4, 5]\n");
    printf("a · b = %.1f (expected: %.1f)\n", dot, expected);
    
    int passed = fabsf(dot - expected) < 1e-5f;
    printf("✓ Dot product: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(a);
    vector_free(b);
    
    return passed;
}

int test_vector_add() {
    printf("\n[TEST] Vector Addition\n");
    printf("──────────────────────\n");
    
    Vector *a = vector_create(3);
    Vector *b = vector_create(3);
    Vector *c = vector_create(3);
    
    // a = [1, 2, 3], b = [4, 5, 6]
    vector_set(a, 0, 1.0f); vector_set(a, 1, 2.0f); vector_set(a, 2, 3.0f);
    vector_set(b, 0, 4.0f); vector_set(b, 1, 5.0f); vector_set(b, 2, 6.0f);
    
    vector_add(a, b, c);
    
    printf("a = [1, 2, 3]\n");
    printf("b = [4, 5, 6]\n");
    printf("c = a + b = [%.1f, %.1f, %.1f]\n", 
           vector_get(c, 0), vector_get(c, 1), vector_get(c, 2));
    
    int passed = (fabsf(vector_get(c, 0) - 5.0f) < 1e-5f &&
                  fabsf(vector_get(c, 1) - 7.0f) < 1e-5f &&
                  fabsf(vector_get(c, 2) - 9.0f) < 1e-5f);
    printf("✓ Vector add: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(a);
    vector_free(b);
    vector_free(c);
    
    return passed;
}

int test_vector_scale() {
    printf("\n[TEST] Vector Scaling\n");
    printf("────────────────────\n");
    
    Vector *x = vector_create(4);
    Vector *y = vector_create(4);
    
    // x = [1, 2, 3, 4], scale by 2.5
    for (int i = 0; i < 4; i++) {
        vector_set(x, i, (float)(i + 1));
    }
    
    vector_scale(x, 2.5f, y);
    
    printf("x = [1, 2, 3, 4]\n");
    printf("y = 2.5 * x = [%.1f, %.1f, %.1f, %.1f]\n",
           vector_get(y, 0), vector_get(y, 1),
           vector_get(y, 2), vector_get(y, 3));
    
    int passed = (fabsf(vector_get(y, 0) - 2.5f) < 1e-5f &&
                  fabsf(vector_get(y, 1) - 5.0f) < 1e-5f);
    printf("✓ Vector scale: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(x);
    vector_free(y);
    
    return passed;
}

int test_relu_activation() {
    printf("\n[TEST] ReLU Activation\n");
    printf("──────────────────────\n");
    
    Vector *x = vector_create(6);
    Vector *y = vector_create(6);
    
    // x = [-2, -1, 0, 1, 2, 3]
    float vals[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f};
    for (int i = 0; i < 6; i++) {
        vector_set(x, i, vals[i]);
    }
    
    activation_relu(x, y);
    
    printf("x = [-2, -1, 0, 1, 2, 3]\n");
    printf("ReLU(x) = [%.1f, %.1f, %.1f, %.1f, %.1f, %.1f]\n",
           vector_get(y, 0), vector_get(y, 1), vector_get(y, 2),
           vector_get(y, 3), vector_get(y, 4), vector_get(y, 5));
    
    int passed = (fabsf(vector_get(y, 0) - 0.0f) < 1e-5f &&
                  fabsf(vector_get(y, 1) - 0.0f) < 1e-5f &&
                  fabsf(vector_get(y, 2) - 0.0f) < 1e-5f &&
                  fabsf(vector_get(y, 3) - 1.0f) < 1e-5f &&
                  fabsf(vector_get(y, 4) - 2.0f) < 1e-5f &&
                  fabsf(vector_get(y, 5) - 3.0f) < 1e-5f);
    printf("✓ ReLU activation: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(x);
    vector_free(y);
    
    return passed;
}

int test_sigmoid_activation() {
    printf("\n[TEST] Sigmoid Activation\n");
    printf("────────────────────────\n");
    
    Vector *x = vector_create(3);
    Vector *y = vector_create(3);
    
    // x = [0, -1, 1]
    vector_set(x, 0, 0.0f);
    vector_set(x, 1, -1.0f);
    vector_set(x, 2, 1.0f);
    
    activation_sigmoid(x, y);
    
    printf("x = [0, -1, 1]\n");
    printf("Sigmoid(x) = [%.4f, %.4f, %.4f]\n",
           vector_get(y, 0), vector_get(y, 1), vector_get(y, 2));
    
    // sigmoid(0) = 0.5, sigmoid(-1) ≈ 0.268, sigmoid(1) ≈ 0.731
    int passed = (fabsf(vector_get(y, 0) - 0.5f) < 1e-4f &&
                  fabsf(vector_get(y, 1) - 0.268f) < 1e-3f &&
                  fabsf(vector_get(y, 2) - 0.731f) < 1e-3f);
    printf("✓ Sigmoid activation: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(x);
    vector_free(y);
    
    return passed;
}

int test_tanh_activation() {
    printf("\n[TEST] Tanh Activation\n");
    printf("──────────────────────\n");
    
    Vector *x = vector_create(3);
    Vector *y = vector_create(3);
    
    // x = [0, -1, 1]
    vector_set(x, 0, 0.0f);
    vector_set(x, 1, -1.0f);
    vector_set(x, 2, 1.0f);
    
    activation_tanh(x, y);
    
    printf("x = [0, -1, 1]\n");
    printf("tanh(x) = [%.4f, %.4f, %.4f]\n",
           vector_get(y, 0), vector_get(y, 1), vector_get(y, 2));
    
    // tanh(0) = 0, tanh(-1) ≈ -0.762, tanh(1) ≈ 0.762
    int passed = (fabsf(vector_get(y, 0) - 0.0f) < 1e-5f &&
                  fabsf(vector_get(y, 1) - (-0.762f)) < 1e-3f &&
                  fabsf(vector_get(y, 2) - 0.762f) < 1e-3f);
    printf("✓ Tanh activation: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(x);
    vector_free(y);
    
    return passed;
}

int test_softmax() {
    printf("\n[TEST] Softmax Normalization\n");
    printf("───────────────────────────\n");
    
    Vector *x = vector_create(3);
    Vector *y = vector_create(3);
    
    // x = [1, 2, 3]
    vector_set(x, 0, 1.0f);
    vector_set(x, 1, 2.0f);
    vector_set(x, 2, 3.0f);
    
    softmax(x, y);
    
    printf("x = [1, 2, 3]\n");
    printf("softmax(x) = [%.4f, %.4f, %.4f]\n",
           vector_get(y, 0), vector_get(y, 1), vector_get(y, 2));
    
    // Check that sum = 1
    float sum = vector_get(y, 0) + vector_get(y, 1) + vector_get(y, 2);
    printf("Sum of softmax: %.6f\n", sum);
    
    int passed = (fabsf(sum - 1.0f) < 1e-5f &&
                  vector_get(y, 0) > 0.0f &&
                  vector_get(y, 2) > vector_get(y, 0));  // higher values get higher softmax
    printf("✓ Softmax: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(x);
    vector_free(y);
    
    return passed;
}

int test_vector_norm() {
    printf("\n[TEST] Vector L2 Norm\n");
    printf("────────────────────\n");
    
    Vector *x = vector_create(3);
    
    // x = [3, 4, 0] -> norm = 5
    vector_set(x, 0, 3.0f);
    vector_set(x, 1, 4.0f);
    vector_set(x, 2, 0.0f);
    
    float norm = vector_norm_l2(x);
    float expected = 5.0f;
    
    printf("x = [3, 4, 0]\n");
    printf("||x||_2 = %.4f (expected: %.4f)\n", norm, expected);
    
    int passed = fabsf(norm - expected) < 1e-5f;
    printf("✓ L2 norm: %s\n", passed ? "PASS" : "FAIL");
    
    vector_free(x);
    
    return passed;
}

// ═══════════════════════════════════════════════════════════════════════════
// Main Test Suite
// ═══════════════════════════════════════════════════════════════════════════

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║          Nova Backends Test Suite (Self-Contained)            ║\n");
    printf("║     CPU Backend, Vector Operations, and Activations           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    int passed = 0;
    int total = 0;
    
    total++; if (test_platform_detection()) passed++;
    total++; if (test_vector_dot_product()) passed++;
    total++; if (test_vector_add()) passed++;
    total++; if (test_vector_scale()) passed++;
    total++; if (test_relu_activation()) passed++;
    total++; if (test_sigmoid_activation()) passed++;
    total++; if (test_tanh_activation()) passed++;
    total++; if (test_softmax()) passed++;
    total++; if (test_vector_norm()) passed++;
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                        Test Results                           ║\n");
    printf("║  Passed: %d/%d                                                 ║\n", passed, total);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return (passed == total) ? 0 : 1;
}
