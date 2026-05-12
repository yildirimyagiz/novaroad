#include "nova_metrics.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Helper Structures and Functions
 * ============================================================================ */

typedef struct {
    float score;
    float label;
} ScoreLabelPair;

/**
 * Comparison function for qsort (scores in descending order)
 */
static int compare_score_label_pairs(const void *a, const void *b) {
    const ScoreLabelPair *pa = (const ScoreLabelPair *)a;
    const ScoreLabelPair *pb = (const ScoreLabelPair *)b;
    
    if (pa->score > pb->score) return -1;
    if (pa->score < pb->score) return 1;
    return 0;
}

/**
 * Clamp value to [1e-7, 1-1e-7] range to avoid log(0)
 */
static float clamp_prob(float x) {
    if (x < 1e-7f) return 1e-7f;
    if (x > 1.0f - 1e-7f) return 1.0f - 1e-7f;
    return x;
}

/* ============================================================================
 * Classification Metrics
 * ============================================================================ */

float nova_accuracy(const int *y_true, const int *y_pred, size_t n_samples) {
    if (!y_true || !y_pred || n_samples == 0) return 0.0f;
    
    size_t correct = 0;
    for (size_t i = 0; i < n_samples; i++) {
        if (y_true[i] == y_pred[i]) {
            correct++;
        }
    }
    
    return (float)correct / (float)n_samples;
}

float nova_precision(const int *y_true, const int *y_pred, size_t n_samples,
                     int n_classes, int class_idx) {
    if (!y_true || !y_pred || n_samples == 0) return 0.0f;
    if (class_idx < 0 || class_idx >= n_classes) return 0.0f;
    
    size_t true_positives = 0;
    size_t false_positives = 0;
    
    for (size_t i = 0; i < n_samples; i++) {
        if (y_pred[i] == class_idx) {
            if (y_true[i] == class_idx) {
                true_positives++;
            } else {
                false_positives++;
            }
        }
    }
    
    size_t pred_positive = true_positives + false_positives;
    if (pred_positive == 0) return 0.0f;
    
    return (float)true_positives / (float)pred_positive;
}

float nova_recall(const int *y_true, const int *y_pred, size_t n_samples,
                  int n_classes, int class_idx) {
    if (!y_true || !y_pred || n_samples == 0) return 0.0f;
    if (class_idx < 0 || class_idx >= n_classes) return 0.0f;
    
    size_t true_positives = 0;
    size_t false_negatives = 0;
    
    for (size_t i = 0; i < n_samples; i++) {
        if (y_true[i] == class_idx) {
            if (y_pred[i] == class_idx) {
                true_positives++;
            } else {
                false_negatives++;
            }
        }
    }
    
    size_t actual_positive = true_positives + false_negatives;
    if (actual_positive == 0) return 0.0f;
    
    return (float)true_positives / (float)actual_positive;
}

float nova_f1_score(const int *y_true, const int *y_pred, size_t n_samples,
                    int n_classes, int class_idx) {
    float precision = nova_precision(y_true, y_pred, n_samples, n_classes, class_idx);
    float recall = nova_recall(y_true, y_pred, n_samples, n_classes, class_idx);
    
    if (precision + recall == 0.0f) return 0.0f;
    
    return 2.0f * (precision * recall) / (precision + recall);
}

void nova_confusion_matrix(const int *y_true, const int *y_pred, size_t n_samples,
                           int n_classes, int *out_matrix) {
    if (!y_true || !y_pred || !out_matrix || n_samples == 0) return;
    
    /* Initialize confusion matrix to zero */
    memset(out_matrix, 0, n_classes * n_classes * sizeof(int));
    
    for (size_t i = 0; i < n_samples; i++) {
        int true_label = y_true[i];
        int pred_label = y_pred[i];
        
        if (true_label >= 0 && true_label < n_classes &&
            pred_label >= 0 && pred_label < n_classes) {
            out_matrix[true_label * n_classes + pred_label]++;
        }
    }
}

/* ============================================================================
 * Regression Metrics
 * ============================================================================ */

float nova_mse(const float *y_true, const float *y_pred, size_t n_samples) {
    if (!y_true || !y_pred || n_samples == 0) return 0.0f;
    
    float sum_squared_error = 0.0f;
    for (size_t i = 0; i < n_samples; i++) {
        float error = y_true[i] - y_pred[i];
        sum_squared_error += error * error;
    }
    
    return sum_squared_error / (float)n_samples;
}

float nova_mae(const float *y_true, const float *y_pred, size_t n_samples) {
    if (!y_true || !y_pred || n_samples == 0) return 0.0f;
    
    float sum_absolute_error = 0.0f;
    for (size_t i = 0; i < n_samples; i++) {
        float error = y_true[i] - y_pred[i];
        sum_absolute_error += fabsf(error);
    }
    
    return sum_absolute_error / (float)n_samples;
}

float nova_r2_score(const float *y_true, const float *y_pred, size_t n_samples) {
    if (!y_true || !y_pred || n_samples == 0) return 0.0f;
    
    /* Compute mean of y_true */
    float mean_y = 0.0f;
    for (size_t i = 0; i < n_samples; i++) {
        mean_y += y_true[i];
    }
    mean_y /= (float)n_samples;
    
    /* Compute SS_res and SS_tot */
    float ss_res = 0.0f;
    float ss_tot = 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        float residual = y_true[i] - y_pred[i];
        float total = y_true[i] - mean_y;
        
        ss_res += residual * residual;
        ss_tot += total * total;
    }
    
    if (ss_tot == 0.0f) return 0.0f;
    
    return 1.0f - (ss_res / ss_tot);
}

/* ============================================================================
 * Loss Functions
 * ============================================================================ */

float nova_cross_entropy_loss(const float *y_true_proba, const float *y_pred_proba,
                              size_t n_samples, size_t n_classes) {
    if (!y_true_proba || !y_pred_proba || n_samples == 0 || n_classes == 0) return 0.0f;
    
    float total_loss = 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        for (size_t j = 0; j < n_classes; j++) {
            float y_true = y_true_proba[i * n_classes + j];
            float y_pred = clamp_prob(y_pred_proba[i * n_classes + j]);
            
            total_loss -= y_true * logf(y_pred);
        }
    }
    
    return total_loss / (float)n_samples;
}

float nova_binary_cross_entropy(const float *y_true, const float *y_pred_proba, size_t n_samples) {
    if (!y_true || !y_pred_proba || n_samples == 0) return 0.0f;
    
    float total_loss = 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        float y = y_true[i];
        float p = clamp_prob(y_pred_proba[i]);
        
        total_loss -= (y * logf(p) + (1.0f - y) * logf(1.0f - p));
    }
    
    return total_loss / (float)n_samples;
}

float nova_log_loss(const float *y_true, const float *y_pred_proba,
                    size_t n_samples, size_t n_classes) {
    if (!y_true || !y_pred_proba || n_samples == 0 || n_classes == 0) return 0.0f;
    
    float total_loss = 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        int true_label = (int)y_true[i];
        
        if (true_label >= 0 && true_label < (int)n_classes) {
            float y_pred = clamp_prob(y_pred_proba[i * n_classes + true_label]);
            total_loss -= logf(y_pred);
        }
    }
    
    return total_loss / (float)n_samples;
}

/* ============================================================================
 * Impurity Metrics (for Decision Trees)
 * ============================================================================ */

float nova_gini_impurity(const int *labels, size_t n_samples, int n_classes) {
    if (!labels || n_samples == 0 || n_classes <= 0) return 0.0f;
    
    /* Count class frequencies */
    int *class_counts = (int *)calloc(n_classes, sizeof(int));
    if (!class_counts) return 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        if (labels[i] >= 0 && labels[i] < n_classes) {
            class_counts[labels[i]]++;
        }
    }
    
    /* Compute Gini impurity: 1 - sum(p_i^2) */
    float gini = 1.0f;
    for (int i = 0; i < n_classes; i++) {
        float p = (float)class_counts[i] / (float)n_samples;
        gini -= p * p;
    }
    
    free(class_counts);
    return gini;
}

float nova_entropy_impurity(const int *labels, size_t n_samples, int n_classes) {
    if (!labels || n_samples == 0 || n_classes <= 0) return 0.0f;
    
    /* Count class frequencies */
    int *class_counts = (int *)calloc(n_classes, sizeof(int));
    if (!class_counts) return 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        if (labels[i] >= 0 && labels[i] < n_classes) {
            class_counts[labels[i]]++;
        }
    }
    
    /* Compute entropy: -sum(p_i * log(p_i)) */
    float entropy = 0.0f;
    for (int i = 0; i < n_classes; i++) {
        if (class_counts[i] > 0) {
            float p = (float)class_counts[i] / (float)n_samples;
            entropy -= p * logf(p);
        }
    }
    
    free(class_counts);
    return entropy;
}

float nova_information_gain(const int *parent_labels, size_t n_parent,
                            const int *left_labels, size_t n_left,
                            const int *right_labels, size_t n_right, int n_classes) {
    if (!parent_labels || n_parent == 0) return 0.0f;
    
    /* Compute parent entropy */
    float parent_entropy = nova_entropy_impurity(parent_labels, n_parent, n_classes);
    
    /* Compute weighted child entropy */
    float left_entropy = 0.0f;
    float right_entropy = 0.0f;
    
    if (n_left > 0) {
        left_entropy = nova_entropy_impurity(left_labels, n_left, n_classes);
    }
    if (n_right > 0) {
        right_entropy = nova_entropy_impurity(right_labels, n_right, n_classes);
    }
    
    float weighted_child_entropy = ((float)n_left / (float)n_parent) * left_entropy +
                                   ((float)n_right / (float)n_parent) * right_entropy;
    
    return parent_entropy - weighted_child_entropy;
}

/* ============================================================================
 * ROC and AUC Metrics
 * ============================================================================ */

float nova_roc_auc(const float *y_true, const float *y_scores, size_t n_samples) {
    if (!y_true || !y_scores || n_samples < 2) return 0.5f;
    
    /* Create array of (score, label) pairs */
    ScoreLabelPair *pairs = (ScoreLabelPair *)malloc(n_samples * sizeof(ScoreLabelPair));
    if (!pairs) return 0.5f;
    
    for (size_t i = 0; i < n_samples; i++) {
        pairs[i].score = y_scores[i];
        pairs[i].label = y_true[i];
    }
    
    /* Sort by score in descending order */
    qsort(pairs, n_samples, sizeof(ScoreLabelPair), compare_score_label_pairs);
    
    /* Count positives and negatives */
    size_t n_pos = 0, n_neg = 0;
    for (size_t i = 0; i < n_samples; i++) {
        if (pairs[i].label > 0.5f) n_pos++;
        else n_neg++;
    }
    
    if (n_pos == 0 || n_neg == 0) {
        free(pairs);
        return 0.5f;
    }
    
    /* Compute ROC AUC using trapezoidal rule */
    float auc = 0.0f;
    float tp = 0.0f;
    
    for (size_t i = 0; i < n_samples; i++) {
        if (pairs[i].label > 0.5f) {
            tp++;
        } else {
            auc += tp;
        }
    }
    
    auc /= ((float)n_pos * (float)n_neg);
    
    free(pairs);
    return auc;
}

