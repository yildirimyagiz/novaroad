#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

/* Include the quantization header */
#include "../../../include/nova_quantization.h"

/* ============================================================================
 * Test framework
 * ============================================================================ */
static int g_passed = 0, g_failed = 0;

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("  ❌ FAIL: %s (line %d)\n", #cond, __LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define ASSERT_NEAR_F(a, b, tol) do { \
    float _a=(a), _b=(b), _t=(tol); \
    if (fabsf(_a-_b) > _t) { \
        printf("  ❌ FAIL: |%f - %f| > %f (line %d)\n",_a,_b,_t,__LINE__); \
        g_failed++; return; \
    } \
} while(0)

#define RUN(fn) do { \
    printf("  [TEST] %-45s", #fn); \
    fn(); \
    if (g_failed == 0) { printf(" PASS\n"); g_passed++; } \
    else { g_failed = 0; } \
} while(0)

/* ============================================================================
 * Test: Basic quantization
 * ============================================================================ */
void test_quantize_basic(void) {
    /* Quantize [0.0, 0.5, 1.0, -1.0] with scale=1/127, zp=0 */
    float in[] = {0.0f, 0.5f, 1.0f, -1.0f};
    int8_t out[4];
    float scale = 1.0f / 127.0f;
    int32_t zp = 0;
    
    nova_quantize_f32_to_int8(in, out, 4, scale, zp);
    
    /* Expected: 0, 63/64 ≈ 63, 127, -127 */
    ASSERT(out[0] == 0);
    ASSERT(out[1] >= 62 && out[1] <= 64);  /* ~63 */
    ASSERT(out[2] == 127);
    ASSERT(out[3] == -127);
}

/* ============================================================================
 * Test: Basic dequantization
 * ============================================================================ */
void test_dequantize_basic(void) {
    int8_t in[] = {0, 64, 127, -127};
    float out[4];
    float scale = 1.0f / 127.0f;
    int32_t zp = 0;
    
    nova_dequantize_int8_to_f32(in, out, 4, scale, zp);
    
    ASSERT_NEAR_F(out[0], 0.0f, 1e-6);
    ASSERT_NEAR_F(out[1], 64.0f / 127.0f, 1e-5);
    ASSERT_NEAR_F(out[2], 1.0f, 1e-5);
    ASSERT_NEAR_F(out[3], -1.0f, 1e-5);
}

/* ============================================================================
 * Test: Round-trip accuracy
 * ============================================================================ */
void test_roundtrip_accuracy(void) {
    float orig[256];
    int8_t quantized[256];
    float dequantized[256];
    
    /* Generate test data in [-1, 1] */
    for (int i = 0; i < 256; i++) {
        orig[i] = -1.0f + (2.0f * i / 255.0f);
    }
    
    float scale = 1.0f / 127.0f;
    int32_t zp = 0;
    
    nova_quantize_f32_to_int8(orig, quantized, 256, scale, zp);
    nova_dequantize_int8_to_f32(quantized, dequantized, 256, scale, zp);
    
    /* Check max error < 1/127 */
    float max_error = 0.0f;
    for (int i = 0; i < 256; i++) {
        float error = fabsf(orig[i] - dequantized[i]);
        if (error > max_error) max_error = error;
    }
    
    ASSERT(max_error < (1.0f / 127.0f + 1e-5));
}

/* ============================================================================
 * Test: Calibrate min-max (symmetric)
 * ============================================================================ */
void test_calibrate_minmax_symmetric(void) {
    float data[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
    float scale;
    int32_t zp;
    
    nova_quantize_calibrate_minmax(data, 5, 1, &scale, &zp);
    
    /* For symmetric: scale = 2/127 */
    ASSERT_NEAR_F(scale, 2.0f / 127.0f, 1e-5);
    ASSERT(zp == 0);
}

/* ============================================================================
 * Test: Calibrate min-max (asymmetric)
 * ============================================================================ */
void test_calibrate_minmax_asymmetric(void) {
    float data[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    float scale;
    int32_t zp;
    
    nova_quantize_calibrate_minmax(data, 5, 0, &scale, &zp);
    
    /* For asymmetric: scale = (4.0 - 0.0) / 255 */
    ASSERT_NEAR_F(scale, 4.0f / 255.0f, 1e-5);
    /* zp should map 0 to -128 or similar */
    ASSERT(zp >= -128 && zp <= 127);
}

/* ============================================================================
 * Test: Per-channel calibration
 * ============================================================================ */
void test_calibrate_per_channel(void) {
    /* 2x3 matrix with different ranges per column */
    float data[] = {
        0.0f, -1.0f, 0.0f,
        2.0f,  1.0f, 4.0f
    };
    float scales[3];
    int32_t zps[3];
    
    /* Calibrate per column (channel_dim=1) */
    nova_quantize_calibrate_per_channel(data, 2, 3, 1, 1, scales, zps);
    
    /* Column 0: [-2, 2], symmetric -> scale = 2/127 */
    ASSERT_NEAR_F(scales[0], 2.0f / 127.0f, 1e-5);
    /* Column 1: [-1, 1], symmetric -> scale = 1/127 */
    ASSERT_NEAR_F(scales[1], 1.0f / 127.0f, 1e-5);
    /* Column 2: [0, 4], symmetric -> scale = 4/127 */
    ASSERT_NEAR_F(scales[2], 4.0f / 127.0f, 1e-5);
    
    ASSERT(zps[0] == 0);
    ASSERT(zps[1] == 0);
    ASSERT(zps[2] == 0);
}

/* ============================================================================
 * Test: Zero scale guard
 * ============================================================================ */
void test_quantize_zero_scale_guard(void) {
    float in[] = {1.0f, 2.0f, 3.0f};
    int8_t out[] = {-1, -1, -1};
    
    /* Should not crash with scale=0 */
    nova_quantize_f32_to_int8(in, out, 3, 0.0f, 0);
    
    /* Output should be unchanged (function returns early) */
    ASSERT(out[0] == -1 && out[1] == -1 && out[2] == -1);
}

/* ============================================================================
 * Test: NULL pointer guard
 * ============================================================================ */
void test_quantize_null_guard(void) {
    float in[] = {1.0f};
    int8_t out[1];
    
    /* Should not crash with NULL inputs */
    nova_quantize_f32_to_int8(NULL, out, 1, 1.0f, 0);
    nova_quantize_f32_to_int8(in, NULL, 1, 1.0f, 0);
    
    /* If we get here, no crash occurred */
    ASSERT(1);
}

/* ============================================================================
 * Test: Per-channel quantization
 * ============================================================================ */
void test_quantize_per_channel(void) {
    /* 2x2 matrix */
    float in[] = {
        1.0f, 2.0f,
        3.0f, 4.0f
    };
    int8_t out[4];
    
    float scales[] = {1.0f / 127.0f, 1.0f / 127.0f};
    int32_t zps[] = {0, 0};
    
    nova_quantize_f32_to_int8_per_channel(in, out, 2, 2, 1, scales, zps);
    
    /* Verify quantization was applied */
    ASSERT(out[0] == 127);      /* 1.0 / (1/127) = 127 */
    ASSERT(out[1] == 127);      /* 2.0 / (1/127) = 254 → clamped to 127 */
}

/* ============================================================================
 * Test: Dequantize per-channel
 * ============================================================================ */
void test_dequantize_per_channel(void) {
    int8_t in[] = {
        64, 127,
        -64, 0
    };
    float out[4];
    
    float scales[] = {1.0f / 127.0f, 1.0f / 127.0f};
    int32_t zps[] = {0, 0};
    
    nova_dequantize_int8_to_f32_per_channel(in, out, 2, 2, 1, scales, zps);
    
    ASSERT_NEAR_F(out[0], 64.0f / 127.0f, 1e-5);
    ASSERT_NEAR_F(out[1], 1.0f, 1e-5);
    ASSERT_NEAR_F(out[2], -64.0f / 127.0f, 1e-5);
    ASSERT_NEAR_F(out[3], 0.0f, 1e-5);
}

/* ============================================================================
 * Main: Run all tests
 * ============================================================================ */
int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Nova Quantization Unit Tests            ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
    
    RUN(test_quantize_basic);
    RUN(test_dequantize_basic);
    RUN(test_roundtrip_accuracy);
    RUN(test_calibrate_minmax_symmetric);
    RUN(test_calibrate_minmax_asymmetric);
    RUN(test_calibrate_per_channel);
    RUN(test_quantize_zero_scale_guard);
    RUN(test_quantize_null_guard);
    RUN(test_quantize_per_channel);
    RUN(test_dequantize_per_channel);
    
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Results: %d/%d passed                     ║\n", g_passed, g_passed + g_failed);
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
    
    return (g_failed == 0) ? 0 : 1;
}
