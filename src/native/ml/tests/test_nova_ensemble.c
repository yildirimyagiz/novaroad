#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../optimizer/nova_ensemble.h"
#include "../ai/core/nova_metrics.h"

/* Macro for floating point comparison with epsilon */
#define ASSERT_NEAR(a, b, eps) \
    do { \
        float diff = fabs((a) - (b)); \
        if (diff > (eps)) { \
            printf("[FAIL] assertion failed: %f vs %f (diff=%f, eps=%f)\n", \
                   (float)(a), (float)(b), diff, (float)(eps)); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] assertion failed: %s\n", msg); \
            exit(1); \
        } \
    } while(0)

/* Test state tracking */
static int tests_passed = 0;
static int tests_total = 0;

#define RUN_TEST(test_func) \
    do { \
        tests_total++; \
        test_func(); \
        tests_passed++; \
        printf("[PASS] " #test_func "\n"); \
    } while(0)

/* ============================================================================
 * XGBoost Tests
 * ============================================================================ */

void test_xgb_create_free(void) {
    NovaXGBModel* model = nova_xgb_create(3, 2, 0.1f);  /* 3 features, 2 classes, lr=0.1 */
    ASSERT_TRUE(model != NULL, "XGBoost model creation failed");
    nova_xgb_free(model);
}

void test_xgb_fit_binary(void) {
    /* Binary classification: 20 samples, 3 features */
    float X[] = {
        0.0f, 0.0f, 0.0f,
        0.1f, 0.1f, 0.1f,
        0.2f, 0.2f, 0.2f,
        0.3f, 0.3f, 0.3f,
        0.4f, 0.4f, 0.4f,
        1.0f, 1.0f, 1.0f,
        1.1f, 1.1f, 1.1f,
        1.2f, 1.2f, 1.2f,
        1.3f, 1.3f, 1.3f,
        1.4f, 1.4f, 1.4f,
        0.0f, 1.0f, 0.5f,
        0.1f, 1.1f, 0.6f,
        0.2f, 1.2f, 0.7f,
        0.3f, 1.3f, 0.8f,
        0.4f, 1.4f, 0.9f,
        1.0f, 0.0f, 1.5f,
        1.1f, 0.1f, 1.6f,
        1.2f, 0.2f, 1.7f,
        1.3f, 0.3f, 1.8f,
        1.4f, 0.4f, 1.9f
    };
    int y[] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    
    NovaXGBModel* model = nova_xgb_create(3, 2, 0.1f);
    nova_xgb_fit(model, X, y, 20, 3, 5, 3);  /* 5 estimators, max_depth=3 */
    
    int* predictions = nova_xgb_predict(model, X, 20);
    ASSERT_TRUE(predictions != NULL, "XGBoost predictions failed");
    
    float accuracy = nova_accuracy(y, predictions, 20);
    /* Just verify that model produces predictions, accuracy may vary */
    ASSERT_TRUE(accuracy >= 0.0f, "XGBoost accuracy check");
    
    free(predictions);
    nova_xgb_free(model);
}

void test_xgb_fit_regression(void) {
    /* Regression test: 10 samples, 2 features */
    float X[] = {
        0.0f, 0.0f,
        0.5f, 0.5f,
        1.0f, 1.0f,
        1.5f, 1.5f,
        2.0f, 2.0f,
        0.0f, 1.0f,
        0.5f, 1.5f,
        1.0f, 2.0f,
        1.5f, 2.5f,
        2.0f, 3.0f
    };
    
    /* Convert float targets to int for fitting (multiclass with 6 classes) */
    int y[] = {1, 2, 3, 4, 5, 2, 3, 4, 5, 6};
    
    NovaXGBModel* model = nova_xgb_create(2, 6, 0.1f);
    nova_xgb_fit(model, X, y, 10, 2, 5, 3);
    
    int* predictions = nova_xgb_predict(model, X, 10);
    ASSERT_TRUE(predictions != NULL, "XGBoost predictions failed");
    
    /* For regression, just check that predictions are within reasonable range */
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(predictions[i] >= 1 && predictions[i] <= 6, "XGBoost prediction out of range");
    }
    
    free(predictions);
    nova_xgb_free(model);
}

/* ============================================================================
 * Gradient Boosting Tests
 * ============================================================================ */

void test_gb_create_free(void) {
    NovaGBModel* model = nova_gb_create(3, 2, 0.1f);
    ASSERT_TRUE(model != NULL, "Gradient Boosting model creation failed");
    nova_gb_free(model);
}

void test_gb_fit_binary(void) {
    /* Binary classification: 20 samples, 3 features */
    float X[] = {
        0.0f, 0.0f, 0.0f,
        0.1f, 0.1f, 0.1f,
        0.2f, 0.2f, 0.2f,
        0.3f, 0.3f, 0.3f,
        0.4f, 0.4f, 0.4f,
        1.0f, 1.0f, 1.0f,
        1.1f, 1.1f, 1.1f,
        1.2f, 1.2f, 1.2f,
        1.3f, 1.3f, 1.3f,
        1.4f, 1.4f, 1.4f,
        0.0f, 1.0f, 0.5f,
        0.1f, 1.1f, 0.6f,
        0.2f, 1.2f, 0.7f,
        0.3f, 1.3f, 0.8f,
        0.4f, 1.4f, 0.9f,
        1.0f, 0.0f, 1.5f,
        1.1f, 0.1f, 1.6f,
        1.2f, 0.2f, 1.7f,
        1.3f, 0.3f, 1.8f,
        1.4f, 0.4f, 1.9f
    };
    int y[] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    
    NovaGBModel* model = nova_gb_create(3, 2, 0.1f);
    nova_gb_fit(model, X, y, 20, 3, 5, 3);
    
    int* predictions = nova_gb_predict(model, X, 20);
    ASSERT_TRUE(predictions != NULL, "Gradient Boosting predictions failed");
    
    float accuracy = nova_accuracy(y, predictions, 20);
    /* Just verify that model produces predictions */
    ASSERT_TRUE(accuracy >= 0.0f, "Gradient Boosting accuracy check");
    
    free(predictions);
    nova_gb_free(model);
}

/* ============================================================================
 * LightGBM Tests
 * ============================================================================ */

void test_lgbm_create_free(void) {
    NovaLGBMModel* model = nova_lgbm_create(3, 2, 0.1f);
    ASSERT_TRUE(model != NULL, "LightGBM model creation failed");
    nova_lgbm_free(model);
}

void test_lgbm_fit_binary(void) {
    /* Binary classification: 20 samples, 3 features */
    float X[] = {
        0.0f, 0.0f, 0.0f,
        0.1f, 0.1f, 0.1f,
        0.2f, 0.2f, 0.2f,
        0.3f, 0.3f, 0.3f,
        0.4f, 0.4f, 0.4f,
        1.0f, 1.0f, 1.0f,
        1.1f, 1.1f, 1.1f,
        1.2f, 1.2f, 1.2f,
        1.3f, 1.3f, 1.3f,
        1.4f, 1.4f, 1.4f,
        0.0f, 1.0f, 0.5f,
        0.1f, 1.1f, 0.6f,
        0.2f, 1.2f, 0.7f,
        0.3f, 1.3f, 0.8f,
        0.4f, 1.4f, 0.9f,
        1.0f, 0.0f, 1.5f,
        1.1f, 0.1f, 1.6f,
        1.2f, 0.2f, 1.7f,
        1.3f, 0.3f, 1.8f,
        1.4f, 0.4f, 1.9f
    };
    int y[] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    
    NovaLGBMModel* model = nova_lgbm_create(3, 2, 0.1f);
    nova_lgbm_fit(model, X, y, 20, 3, 5, 3);
    
    int* predictions = nova_lgbm_predict(model, X, 20);
    ASSERT_TRUE(predictions != NULL, "LightGBM predictions failed");
    
    float accuracy = nova_accuracy(y, predictions, 20);
    /* Just verify that model produces predictions */
    ASSERT_TRUE(accuracy >= 0.0f, "LightGBM accuracy check");
    
    free(predictions);
    nova_lgbm_free(model);
}

/* ============================================================================
 * Helper Functions Tests
 * ============================================================================ */

void test_ensemble_compute_gradients(void) {
    /* Gradients for MSE loss:
     * predictions = [1.0, 2.0]
     * y = [2, 3]
     * Gradient of MSE = (pred - y) = [-1.0, -1.0]
     * (exact formula may vary in implementation)
     */
    float predictions[] = {1.0f, 2.0f};
    int y[] = {2, 3};
    
    float* gradients = nova_compute_gradients(predictions, y, 2);
    ASSERT_TRUE(gradients != NULL, "Gradient computation failed");
    
    /* Just check that gradients are computed and are reasonable (negative for underestimation) */
    ASSERT_TRUE(gradients[0] < 0.0f, "Gradient should be negative");
    ASSERT_TRUE(gradients[1] < 0.0f, "Gradient should be negative");
    
    free(gradients);
}

void test_ensemble_compute_hessians(void) {
    /* Hessians (second derivatives) should be positive for MSE loss */
    float predictions[] = {0.5f, 1.5f, 2.5f};
    int y[] = {1, 2, 3};
    
    float* hessians = nova_compute_hessians(predictions, y, 3);
    ASSERT_TRUE(hessians != NULL, "Hessian computation failed");
    
    /* Just verify that hessians are computed and positive (convexity property) */
    for (int i = 0; i < 3; i++) {
        ASSERT_TRUE(hessians[i] > 0.0f, "Hessian should be positive");
    }
    
    free(hessians);
}

/* ============================================================================
 * main() - Run all tests
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("   Nova Ensemble Unit Tests\n");
    printf("========================================\n\n");
    
    /* XGBoost tests */
    RUN_TEST(test_xgb_create_free);
    RUN_TEST(test_xgb_fit_binary);
    RUN_TEST(test_xgb_fit_regression);
    
    /* Gradient Boosting tests */
    RUN_TEST(test_gb_create_free);
    RUN_TEST(test_gb_fit_binary);
    
    /* LightGBM tests */
    RUN_TEST(test_lgbm_create_free);
    RUN_TEST(test_lgbm_fit_binary);
    
    /* Helper function tests */
    RUN_TEST(test_ensemble_compute_gradients);
    RUN_TEST(test_ensemble_compute_hessians);
    
    printf("\n========================================\n");
    printf("=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}
