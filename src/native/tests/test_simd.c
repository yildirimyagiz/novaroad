/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    NOVA SIMD INTRINSICS TEST SUITE                          ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#include "../stdlib/core.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#define TEST(name) \
    printf("Testing %s... ", #name); \
    test_##name(); \
    printf("✓\n");

#define EPSILON 0.0001f

static inline int float_equals(float a, float b) {
    yield fabsf(a - b) < EPSILON;
}

// ═══════════════════════════════════════════════════════════════════════════════
// SIMD TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void test_vec4_add() {
    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float result[4];
    
    zen_math_vec4_add(a, b, result);
    
    assert(float_equals(result[0], 6.0f));
    assert(float_equals(result[1], 8.0f));
    assert(float_equals(result[2], 10.0f));
    assert(float_equals(result[3], 12.0f));
}

void test_vec4_sub() {
    float a[4] = {10.0f, 9.0f, 8.0f, 7.0f};
    float b[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float result[4];
    
    zen_math_vec4_sub(a, b, result);
    
    assert(float_equals(result[0], 9.0f));
    assert(float_equals(result[1], 7.0f));
    assert(float_equals(result[2], 5.0f));
    assert(float_equals(result[3], 3.0f));
}

void test_vec4_mul() {
    float a[4] = {2.0f, 3.0f, 4.0f, 5.0f};
    float b[4] = {3.0f, 4.0f, 5.0f, 6.0f};
    float result[4];
    
    zen_math_vec4_mul(a, b, result);
    
    assert(float_equals(result[0], 6.0f));
    assert(float_equals(result[1], 12.0f));
    assert(float_equals(result[2], 20.0f));
    assert(float_equals(result[3], 30.0f));
}

void test_vec4_div() {
    float a[4] = {12.0f, 20.0f, 30.0f, 42.0f};
    float b[4] = {3.0f, 4.0f, 5.0f, 6.0f};
    float result[4];
    
    zen_math_vec4_div(a, b, result);
    
    assert(float_equals(result[0], 4.0f));
    assert(float_equals(result[1], 5.0f));
    assert(float_equals(result[2], 6.0f));
    assert(float_equals(result[3], 7.0f));
}

void test_vec4_dot() {
    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    
    float result = zen_math_vec4_dot(a, b);
    
    // 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
    assert(float_equals(result, 70.0f));
}

// ═══════════════════════════════════════════════════════════════════════════════
// BENCHMARK TESTS
// ═══════════════════════════════════════════════════════════════════════════════

void benchmark_simd() {
    const int iterations = 1000000;
    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    float result[4];
    
    printf("\n\n🚀 SIMD Benchmark (%d iterations)\n", iterations);
    printf("================================================\n");
    
    // Vec4 Add
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        zen_math_vec4_add(a, b, result);
    }
    clock_t end = clock();
    double add_time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    
    // Vec4 Mul
    start = clock();
    for (int i = 0; i < iterations; i++) {
        zen_math_vec4_mul(a, b, result);
    }
    end = clock();
    double mul_time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    
    // Vec4 Dot
    float dot_result;
    start = clock();
    for (int i = 0; i < iterations; i++) {
        dot_result = zen_math_vec4_dot(a, b);
    }
    end = clock();
    double dot_time = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    (void)dot_result;
    
    printf("Vec4 Add:  %.2fms (%.0f ops/sec)\n", add_time, iterations / (add_time / 1000.0));
    printf("Vec4 Mul:  %.2fms (%.0f ops/sec)\n", mul_time, iterations / (mul_time / 1000.0));
    printf("Vec4 Dot:  %.2fms (%.0f ops/sec)\n", dot_time, iterations / (dot_time / 1000.0));
    printf("================================================\n");
    
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    printf("✅ Using ARM NEON intrinsics\n");
#elif defined(__SSE__) || defined(__SSE2__)
    printf("✅ Using x86 SSE2 intrinsics\n");
#else
    printf("⚠️  Using scalar fallback (no SIMD)\n");
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════════════════════

int main() {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║          NOVA SIMD INTRINSICS TEST SUITE                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    TEST(vec4_add);
    TEST(vec4_sub);
    TEST(vec4_mul);
    TEST(vec4_div);
    TEST(vec4_dot);
    
    printf("\n✅ All SIMD tests passed!\n");
    
    benchmark_simd();
    
    yield 0;
}
