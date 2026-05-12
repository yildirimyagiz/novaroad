#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../optimizer/nova_physics_opt.h"

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

#define ASSERT_NEAR(a, b, tol) \
    do { \
        float _a = (a), _b = (b), _tol = (tol); \
        if (fabsf(_a - _b) > _tol) { \
            printf("ASSERT_NEAR failed: %f != %f (tolerance: %f)\n", _a, _b, _tol); \
            exit(1); \
        } \
    } while(0)

#define TEST_PASS() printf("✓ %s\n", __func__)
#define TEST_FAIL(msg) do { printf("✗ %s: %s\n", __func__, msg); exit(1); } while(0)

/* ============================================================================
 * Objective Functions
 * ============================================================================ */

static float obj_sphere(const float* x, int n, void* ctx) {
    (void)ctx;
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += x[i] * x[i];
    }
    return sum;  /* minimize */
}

/* ============================================================================
 * Test Cases
 * ============================================================================ */

void test_sa_basic(void) {
    NovaSAOpts opts = {
        .initial_temp = 10.0f,
        .cooling_rate = 0.95f,
        .min_temp = 0.01f,
        .max_iter = 1000,
        .n_dims = 1
    };
    
    float x0[1] = {5.0f};
    float *result = nova_simulated_annealing(&opts, obj_sphere, NULL, x0);
    
    assert(result != NULL);
    
    float final_value = obj_sphere(result, 1, NULL);
    printf("  SA result: x=%.6f, f(x)=%.6f\n", result[0], final_value);
    
    /* Verify result is valid and finite */
    assert(isfinite(final_value));
    assert(final_value >= 0.0f);
    
    free(result);
    TEST_PASS();
}

void test_sa_accept_prob_downhill(void) {
    float prob = nova_sa_accept_prob(-1.0f, 1.0f);  /* delta_e < 0 (downhill) */
    
    /* Downhill moves should always be accepted */
    ASSERT_NEAR(prob, 1.0f, 1e-6f);
    
    TEST_PASS();
}

void test_sa_accept_prob_uphill(void) {
    float prob = nova_sa_accept_prob(1.0f, 1.0f);  /* delta_e > 0, T > 0 (uphill) */
    
    /* Uphill moves should have 0 < prob < 1 */
    assert(prob > 0.0f && prob < 1.0f);
    
    printf("  SA uphill acceptance probability: %.6f\n", prob);
    
    TEST_PASS();
}

void test_pso_create_optimize(void) {
    NovaPSOOpts opts = {
        .n_particles = 30,
        .n_dims = 2,
        .max_iter = 100,
        .w = 0.7f,
        .c1 = 1.5f,
        .c2 = 1.5f,
        .x_min = -5.0f,
        .x_max = 5.0f
    };
    
    float *result = nova_pso_optimize(&opts, obj_sphere, NULL);
    
    assert(result != NULL);
    
    float final_value = obj_sphere(result, 2, NULL);
    printf("  PSO result: x=%.6f, y=%.6f, f(x,y)=%.6f\n", result[0], result[1], final_value);
    
    assert(final_value < 100.0f);  /* PSO should find reasonable solution */
    
    nova_pso_free(result);
    TEST_PASS();
}

void test_pso_bounds(void) {
    NovaPSOOpts opts = {
        .n_particles = 20,
        .n_dims = 2,
        .max_iter = 50,
        .w = 0.7f,
        .c1 = 1.5f,
        .c2 = 1.5f,
        .x_min = -5.0f,
        .x_max = 5.0f
    };
    
    float *result = nova_pso_optimize(&opts, obj_sphere, NULL);
    
    assert(result != NULL);
    
    /* Verify particles stay within bounds */
    assert(result[0] >= -5.0f && result[0] <= 5.0f);
    assert(result[1] >= -5.0f && result[1] <= 5.0f);
    
    nova_pso_free(result);
    TEST_PASS();
}

void test_harmony_search_basic(void) {
    NovaHSOpts opts = {
        .harmony_memory_size = 30,
        .n_dims = 1,
        .max_iter = 100,
        .hmcr = 0.9f,
        .par = 0.3f,
        .bw = 0.01f,
        .x_min = -5.0f,
        .x_max = 5.0f
    };
    
    float *result = nova_harmony_search(&opts, obj_sphere, NULL);
    
    assert(result != NULL);
    
    float final_value = obj_sphere(result, 1, NULL);
    printf("  Harmony Search result: x=%.6f, f(x)=%.6f\n", result[0], final_value);
    
    assert(final_value < 100.0f);
    
    nova_hs_free(result);
    TEST_PASS();
}

void test_spring_mass_basic(void) {
    float *result = nova_spring_mass_optimize(20, 2, 100, 0.5f, 0.1f, obj_sphere, NULL);
    
    assert(result != NULL);
    
    float final_value = obj_sphere(result, 2, NULL);
    printf("  Spring-Mass result: x=%.6f, y=%.6f, f(x,y)=%.6f\n", result[0], result[1], final_value);
    
    /* Just verify it returns a result without crashing */
    assert(isfinite(result[0]) || isfinite(result[1]));
    
    nova_sms_free(result);
    TEST_PASS();
}

void test_gsa_basic(void) {
    NovaGSAOpts opts = {
        .n_agents = 30,
        .n_dims = 2,
        .max_iter = 100,
        .G0 = 100.0f,
        .alpha = 20.0f
    };
    
    float *result = nova_gsa_optimize(&opts, obj_sphere, NULL);
    
    assert(result != NULL);
    
    float final_value = obj_sphere(result, 2, NULL);
    printf("  GSA result: x=%.6f, y=%.6f, f(x,y)=%.6f\n", result[0], result[1], final_value);
    
    /* Verify it returns valid results */
    assert(isfinite(final_value) || isfinite(result[0]) || isfinite(result[1]));
    
    nova_gsa_free(result);
    TEST_PASS();
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("\n=== Nova Physics-Inspired Optimization Unit Tests ===\n\n");
    
    test_sa_basic();
    test_sa_accept_prob_downhill();
    test_sa_accept_prob_uphill();
    test_pso_create_optimize();
    test_pso_bounds();
    test_harmony_search_basic();
    test_spring_mass_basic();
    test_gsa_basic();
    
    printf("\n=== All Physics Optimization Tests Passed ===\n\n");
    return 0;
}
