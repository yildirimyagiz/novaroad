#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
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
 * Classification Metrics Tests
 * ============================================================================ */

void test_accuracy_perfect(void) {
    int y_true[] = {0, 1, 2, 0, 1, 2};
    int y_pred[] = {0, 1, 2, 0, 1, 2};
    float acc = nova_accuracy(y_true, y_pred, 6);
    ASSERT_NEAR(acc, 1.0f, 1e-5);
}

void test_accuracy_half(void) {
    int y_true[] = {0, 1, 0, 1};
    int y_pred[] = {0, 0, 1, 1};
    float acc = nova_accuracy(y_true, y_pred, 4);
    ASSERT_NEAR(acc, 0.5f, 1e-5);
}

void test_accuracy_zero(void) {
    int y_true[] = {0, 1, 2};
    int y_pred[] = {1, 2, 0};
    float acc = nova_accuracy(y_true, y_pred, 3);
    ASSERT_NEAR(acc, 0.0f, 1e-5);
}

/* ============================================================================
 * Regression Metrics Tests
 * ============================================================================ */

void test_mse_zero(void) {
    float y_true[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y_pred[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float mse = nova_mse(y_true, y_pred, 4);
    ASSERT_NEAR(mse, 0.0f, 1e-5);
}

void test_mse_basic(void) {
    float y_true[] = {0.0f, 0.0f, 0.0f};
    float y_pred[] = {1.0f, 1.0f, 1.0f};
    float mse = nova_mse(y_true, y_pred, 3);
    ASSERT_NEAR(mse, 1.0f, 1e-5);
}

void test_mae_basic(void) {
    float y_true[] = {0.0f, 0.0f, 0.0f};
    float y_pred[] = {3.0f, 3.0f, 3.0f};
    float mae = nova_mae(y_true, y_pred, 3);
    ASSERT_NEAR(mae, 3.0f, 1e-5);
}

void test_r2_perfect(void) {
    float y_true[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float y_pred[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float r2 = nova_r2_score(y_true, y_pred, 4);
    ASSERT_NEAR(r2, 1.0f, 1e-5);
}

void test_r2_baseline(void) {
    /* Mean predictor should give R² = 0 */
    float y_true[] = {1.0f, 2.0f, 3.0f};
    float mean = 2.0f;
    float y_pred[] = {mean, mean, mean};
    float r2 = nova_r2_score(y_true, y_pred, 3);
    ASSERT_NEAR(r2, 0.0f, 1e-5);
}

/* ============================================================================
 * Loss Functions Tests
 * ============================================================================ */

void test_binary_cross_entropy(void) {
    /* Perfect prediction: high confidence */
    float y_true[] = {0.0f, 1.0f};
    float y_pred_proba[] = {0.01f, 0.99f};
    float bce = nova_binary_cross_entropy(y_true, y_pred_proba, 2);
    /* Should be close to 0 for perfect predictions */
    ASSERT_NEAR(bce, 0.0f, 0.1f);
}

/* ============================================================================
 * Impurity Metrics Tests
 * ============================================================================ */

void test_gini_impurity_pure(void) {
    /* All samples same class */
    int labels[] = {0, 0, 0, 0};
    float gini = nova_gini_impurity(labels, 4, 2);
    ASSERT_NEAR(gini, 0.0f, 1e-5);
}

void test_gini_impurity_balanced(void) {
    /* 50/50 binary split */
    int labels[] = {0, 0, 1, 1};
    float gini = nova_gini_impurity(labels, 4, 2);
    ASSERT_NEAR(gini, 0.5f, 1e-5);
}

void test_entropy_pure(void) {
    /* All same class => entropy = 0 */
    int labels[] = {1, 1, 1, 1};
    float entropy = nova_entropy_impurity(labels, 4, 2);
    ASSERT_NEAR(entropy, 0.0f, 1e-5);
}

/* ============================================================================
 * Confusion Matrix and F1 Tests
 * ============================================================================ */

void test_confusion_matrix(void) {
    int y_true[] = {0, 1, 2, 0, 1, 2};
    int y_pred[] = {0, 1, 2, 0, 1, 2};
    int confusion[9];
    nova_confusion_matrix(y_true, y_pred, 6, 3, confusion);
    
    /* Diagonal should be [2, 2, 2] for perfect predictions */
    ASSERT_NEAR((float)confusion[0], 2.0f, 1e-5);  /* (0,0) */
    ASSERT_NEAR((float)confusion[4], 2.0f, 1e-5);  /* (1,1) */
    ASSERT_NEAR((float)confusion[8], 2.0f, 1e-5);  /* (2,2) */
    
    /* Off-diagonal should be 0 */
    for (int i = 0; i < 9; i++) {
        if (i != 0 && i != 4 && i != 8) {
            ASSERT_NEAR((float)confusion[i], 0.0f, 1e-5);
        }
    }
}

void test_precision_recall_f1(void) {
    /* Binary classification test case:
     * y_true = [0, 1, 1, 0, 1]
     * y_pred = [0, 1, 0, 0, 1]
     * For class 1: TP=2, FP=0, FN=1, TN=2
     * Precision = TP/(TP+FP) = 2/2 = 1.0
     * Recall = TP/(TP+FN) = 2/3 = 0.667
     * F1 = 2*P*R/(P+R) = 2*1.0*0.667/1.667 = 0.8
     */
    int y_true[] = {0, 1, 1, 0, 1};
    int y_pred[] = {0, 1, 0, 0, 1};
    
    float precision = nova_precision(y_true, y_pred, 5, 2, 1);
    float recall = nova_recall(y_true, y_pred, 5, 2, 1);
    float f1 = nova_f1_score(y_true, y_pred, 5, 2, 1);
    
    ASSERT_NEAR(precision, 1.0f, 1e-5);
    ASSERT_NEAR(recall, 2.0f/3.0f, 1e-5);
    ASSERT_NEAR(f1, 0.8f, 1e-2);
}

/* ============================================================================
 * main() - Run all tests
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("   Nova Metrics Unit Tests\n");
    printf("========================================\n\n");
    
    /* Classification tests */
    RUN_TEST(test_accuracy_perfect);
    RUN_TEST(test_accuracy_half);
    RUN_TEST(test_accuracy_zero);
    
    /* Regression tests */
    RUN_TEST(test_mse_zero);
    RUN_TEST(test_mse_basic);
    RUN_TEST(test_mae_basic);
    RUN_TEST(test_r2_perfect);
    RUN_TEST(test_r2_baseline);
    
    /* Loss function tests */
    RUN_TEST(test_binary_cross_entropy);
    
    /* Impurity tests */
    RUN_TEST(test_gini_impurity_pure);
    RUN_TEST(test_gini_impurity_balanced);
    RUN_TEST(test_entropy_pure);
    
    /* Confusion matrix and F1 tests */
    RUN_TEST(test_confusion_matrix);
    RUN_TEST(test_precision_recall_f1);
    
    printf("\n========================================\n");
    printf("=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    printf("========================================\n");
    
    return (tests_passed == tests_total) ? 0 : 1;
}
