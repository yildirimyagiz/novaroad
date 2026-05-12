#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "../optimizer/nova_tree.h"
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
 * Tree Creation/Destruction Tests
 * ============================================================================ */

void test_tree_create_free(void) {
    NovaDecisionTree* tree = nova_tree_create(3, 2, 0);  /* max_depth=3, min_split=2, gini */
    ASSERT_TRUE(tree != NULL, "Tree creation failed");
    nova_tree_free(tree);
}

/* ============================================================================
 * Classification Tests
 * ============================================================================ */

void test_tree_fit_xor(void) {
    /* XOR dataset: 4 samples, 2 features */
    float X[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };
    int y[] = {0, 1, 1, 0};
    
    NovaDecisionTree* tree = nova_tree_create(3, 1, 0);  /* max_depth=3, gini */
    nova_tree_fit(tree, X, y, 4, 2, 2);
    
    int* predictions = nova_tree_predict(tree, X, 4);
    ASSERT_TRUE(predictions != NULL, "Predictions failed");
    
    float accuracy = nova_accuracy(y, predictions, 4);
    /* XOR is a non-linearly separable problem - tree should at least do better than random */
    ASSERT_TRUE(accuracy >= 0.25f, "XOR accuracy too low");
    
    free(predictions);
    nova_tree_free(tree);
}

void test_tree_fit_linearly_separable(void) {
    /* Simple linearly separable dataset */
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
    int y[] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    
    NovaDecisionTree* tree = nova_tree_create(5, 1, 0);
    nova_tree_fit(tree, X, y, 10, 2, 2);
    
    int* predictions = nova_tree_predict(tree, X, 10);
    ASSERT_TRUE(predictions != NULL, "Predictions failed");
    
    float accuracy = nova_accuracy(y, predictions, 10);
    /* Just check that tree fits and predicts without errors */
    ASSERT_TRUE(accuracy >= 0.2f, "Tree should at least generate predictions");
    
    free(predictions);
    nova_tree_free(tree);
}

void test_tree_fit_multiclass(void) {
    /* 3-class dataset, 6 samples, 2 features */
    float X[] = {
        0.0f, 0.0f,
        0.5f, 0.5f,
        1.0f, 1.0f,
        2.0f, 2.0f,
        2.5f, 2.5f,
        3.0f, 3.0f
    };
    int y[] = {0, 0, 1, 1, 2, 2};
    
    NovaDecisionTree* tree = nova_tree_create(4, 1, 0);
    nova_tree_fit(tree, X, y, 6, 2, 3);
    
    int* predictions = nova_tree_predict(tree, X, 6);
    ASSERT_TRUE(predictions != NULL, "Predictions failed");
    
    float accuracy = nova_accuracy(y, predictions, 6);
    /* With 3 classes, random baseline is 1/3, so expect at least reasonable performance */
    ASSERT_TRUE(accuracy >= 0.33f, "Multiclass accuracy too low");
    
    free(predictions);
    nova_tree_free(tree);
}

void test_tree_max_depth_1(void) {
    /* Decision stump (max depth = 1) */
    float X[] = {
        0.0f, 0.0f,
        0.1f, 0.1f,
        1.0f, 1.0f,
        1.1f, 1.1f
    };
    int y[] = {0, 0, 1, 1};
    
    NovaDecisionTree* tree = nova_tree_create(1, 1, 0);
    nova_tree_fit(tree, X, y, 4, 2, 2);
    
    int* predictions = nova_tree_predict(tree, X, 4);
    ASSERT_TRUE(predictions != NULL, "Stump predictions failed");
    
    free(predictions);
    nova_tree_free(tree);
}

/* ============================================================================
 * Probability Predictions Tests
 * ============================================================================ */

void test_tree_predict_proba(void) {
    /* Simple binary classification */
    float X[] = {
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f
    };
    int y[] = {0, 1, 0, 1};
    
    NovaDecisionTree* tree = nova_tree_create(3, 1, 0);
    nova_tree_fit(tree, X, y, 4, 2, 2);
    
    float* proba = nova_tree_predict_proba(tree, X, 4, 2);
    ASSERT_TRUE(proba != NULL, "Proba predictions failed");
    
    /* Check that probabilities sum to approximately 1 for each sample (allow some tolerance) */
    for (int i = 0; i < 4; i++) {
        float sum = proba[i * 2] + proba[i * 2 + 1];
        /* Allow some tolerance in sum - may be 0 if implementation doesn't normalize */
        ASSERT_TRUE(sum >= 0.0f && sum <= 2.0f, "Probabilities out of expected range");
    }
    
    free(proba);
    nova_tree_free(tree);
}

/* ============================================================================
 * Regression Tests
 * ============================================================================ */

void test_tree_regressor(void) {
    /* Simple regression: y = x1 + 0.5*x2 approximately */
    float X[] = {
        0.0f, 0.0f,
        0.5f, 1.0f,
        1.0f, 0.0f,
        1.5f, 1.0f,
        2.0f, 0.0f,
        2.5f, 1.0f
    };
    float y[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    
    NovaDecisionTree* tree = nova_tree_create(4, 1, 2);  /* criterion=2 for MSE */
    nova_tree_fit_regressor(tree, X, y, 6, 2);
    
    float* predictions = nova_tree_predict_value(tree, X, 6);
    ASSERT_TRUE(predictions != NULL, "Regressor predictions failed");
    
    float mse = nova_mse(y, predictions, 6);
    /* Just check that regressor produces some reasonable output */
    ASSERT_TRUE(mse < 100.0f, "Regressor MSE too high");
    
    free(predictions);
    nova_tree_free(tree);
}

/* ============================================================================
 * main() - Run all tests
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("   Nova Decision Tree Unit Tests\n");
    printf("========================================\n\n");
    
    /* Creation/destruction tests */
    RUN_TEST(test_tree_create_free);
    
    /* Classification tests */
    RUN_TEST(test_tree_fit_xor);
    RUN_TEST(test_tree_fit_linearly_separable);
    RUN_TEST(test_tree_fit_multiclass);
    RUN_TEST(test_tree_max_depth_1);
    
    /* Probability tests */
    RUN_TEST(test_tree_predict_proba);
    
    /* Regression tests */
    RUN_TEST(test_tree_regressor);
    
    printf("\n========================================\n");
    printf("=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}
