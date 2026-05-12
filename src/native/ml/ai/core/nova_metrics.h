#ifndef NOVA_METRICS_H
#define NOVA_METRICS_H

#include <stddef.h>

/* ============================================================================
 * Classification Metrics
 * ============================================================================ */

/**
 * Compute accuracy: proportion of correct predictions
 * @param y_true: ground truth labels [n_samples]
 * @param y_pred: predicted labels [n_samples]
 * @param n_samples: number of samples
 * @return accuracy in [0, 1]
 */
float nova_accuracy(const int *y_true, const int *y_pred, size_t n_samples);

/**
 * Compute precision for a specific class
 * @param y_true: ground truth labels [n_samples]
 * @param y_pred: predicted labels [n_samples]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @param class_idx: class index to compute precision for
 * @return precision in [0, 1]
 */
float nova_precision(const int *y_true, const int *y_pred, size_t n_samples, 
                     int n_classes, int class_idx);

/**
 * Compute recall for a specific class
 * @param y_true: ground truth labels [n_samples]
 * @param y_pred: predicted labels [n_samples]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @param class_idx: class index to compute recall for
 * @return recall in [0, 1]
 */
float nova_recall(const int *y_true, const int *y_pred, size_t n_samples,
                  int n_classes, int class_idx);

/**
 * Compute F1 score for a specific class
 * @param y_true: ground truth labels [n_samples]
 * @param y_pred: predicted labels [n_samples]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @param class_idx: class index to compute F1 for
 * @return F1 score in [0, 1]
 */
float nova_f1_score(const int *y_true, const int *y_pred, size_t n_samples,
                    int n_classes, int class_idx);

/**
 * Compute confusion matrix
 * @param y_true: ground truth labels [n_samples]
 * @param y_pred: predicted labels [n_samples]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @param out_matrix: output confusion matrix [n_classes * n_classes]
 */
void nova_confusion_matrix(const int *y_true, const int *y_pred, size_t n_samples,
                           int n_classes, int *out_matrix);

/* ============================================================================
 * Regression Metrics
 * ============================================================================ */

/**
 * Compute Mean Squared Error
 * @param y_true: ground truth values [n_samples]
 * @param y_pred: predicted values [n_samples]
 * @param n_samples: number of samples
 * @return MSE value
 */
float nova_mse(const float *y_true, const float *y_pred, size_t n_samples);

/**
 * Compute Mean Absolute Error
 * @param y_true: ground truth values [n_samples]
 * @param y_pred: predicted values [n_samples]
 * @param n_samples: number of samples
 * @return MAE value
 */
float nova_mae(const float *y_true, const float *y_pred, size_t n_samples);

/**
 * Compute R² coefficient of determination
 * @param y_true: ground truth values [n_samples]
 * @param y_pred: predicted values [n_samples]
 * @param n_samples: number of samples
 * @return R² in (-∞, 1]
 */
float nova_r2_score(const float *y_true, const float *y_pred, size_t n_samples);

/* ============================================================================
 * Loss Functions
 * ============================================================================ */

/**
 * Compute cross-entropy loss for multiclass classification
 * @param y_true_proba: one-hot encoded ground truth [n_samples * n_classes]
 * @param y_pred_proba: predicted probabilities [n_samples * n_classes]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @return cross-entropy loss
 */
float nova_cross_entropy_loss(const float *y_true_proba, const float *y_pred_proba,
                              size_t n_samples, size_t n_classes);

/**
 * Compute binary cross-entropy loss
 * @param y_true: ground truth binary labels [n_samples]
 * @param y_pred_proba: predicted probabilities [n_samples]
 * @param n_samples: number of samples
 * @return binary cross-entropy loss
 */
float nova_binary_cross_entropy(const float *y_true, const float *y_pred_proba, size_t n_samples);

/**
 * Compute log loss (same as binary cross-entropy)
 * @param y_true: ground truth labels [n_samples]
 * @param y_pred_proba: predicted probabilities [n_samples * n_classes]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @return log loss
 */
float nova_log_loss(const float *y_true, const float *y_pred_proba,
                    size_t n_samples, size_t n_classes);

/* ============================================================================
 * Impurity Metrics (for Decision Trees)
 * ============================================================================ */

/**
 * Compute Gini impurity
 * @param labels: class labels [n_samples]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @return Gini impurity in [0, 1]
 */
float nova_gini_impurity(const int *labels, size_t n_samples, int n_classes);

/**
 * Compute entropy impurity (information)
 * @param labels: class labels [n_samples]
 * @param n_samples: number of samples
 * @param n_classes: number of classes
 * @return entropy in [0, log(n_classes)]
 */
float nova_entropy_impurity(const int *labels, size_t n_samples, int n_classes);

/**
 * Compute information gain from split
 * @param parent_labels: parent node labels [n_parent]
 * @param n_parent: number of parent samples
 * @param left_labels: left child labels [n_left]
 * @param n_left: number of left samples
 * @param right_labels: right child labels [n_right]
 * @param n_right: number of right samples
 * @param n_classes: number of classes
 * @return information gain
 */
float nova_information_gain(const int *parent_labels, size_t n_parent,
                            const int *left_labels, size_t n_left,
                            const int *right_labels, size_t n_right, int n_classes);

/* ============================================================================
 * ROC and AUC Metrics
 * ============================================================================ */

/**
 * Compute ROC AUC (Area Under the Receiver Operating Characteristic Curve)
 * Uses trapezoidal rule for integration
 * @param y_true: ground truth binary labels [n_samples]
 * @param y_scores: predicted scores/probabilities [n_samples]
 * @param n_samples: number of samples
 * @return AUC in [0, 1]
 */
float nova_roc_auc(const float *y_true, const float *y_scores, size_t n_samples);

#endif
