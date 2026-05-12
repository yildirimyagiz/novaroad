#include "nova_ensemble.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============ Helper Functions ============ */

/**
 * Sigmoid activation function for binary classification
 */
static float sigmoid(float x) {
    if (x > 100.0f) return 1.0f;
    if (x < -100.0f) return 0.0f;
    return 1.0f / (1.0f + expf(-x));
}

/**
 * Softmax for multi-class (in-place)
 */
static void __attribute__((unused)) softmax(float* logits, int n_classes) {
    float max_val = logits[0];
    for (int i = 1; i < n_classes; i++) {
        if (logits[i] > max_val) max_val = logits[i];
    }
    
    float sum = 0.0f;
    for (int i = 0; i < n_classes; i++) {
        logits[i] = expf(logits[i] - max_val);
        sum += logits[i];
    }
    
    for (int i = 0; i < n_classes; i++) {
        logits[i] /= sum;
    }
}

/**
 * Compute mean squared error
 */
static float compute_mse(const float* y, int n_samples) {
    float mean = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        mean += y[i];
    }
    mean /= n_samples;
    
    float mse = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        float diff = y[i] - mean;
        mse += diff * diff;
    }
    return mse / n_samples;
}

/**
 * Allocate tree node
 */
static NovaTreeNode* nova_tree_node_create(void) {
    NovaTreeNode* node = (NovaTreeNode*)malloc(sizeof(NovaTreeNode));
    node->split_feature = -1;
    node->split_value = 0.0f;
    node->left = NULL;
    node->right = NULL;
    node->leaf_value = 0.0f;
    node->is_leaf = 0;
    node->n_samples = 0;
    return node;
}

/**
 * Free tree recursively
 */
static void __attribute__((unused)) nova_tree_node_free(NovaTreeNode* node) {
    if (node == NULL) return;
    nova_tree_node_free(node->left);
    nova_tree_node_free(node->right);
    free(node);
}

/**
 * Create decision tree
 */
static NovaDecisionTree* nova_tree_create(int max_depth) {
    NovaDecisionTree* tree = (NovaDecisionTree*)malloc(sizeof(NovaDecisionTree));
    tree->nodes = NULL;
    tree->n_nodes = 0;
    tree->max_depth = max_depth;
    return tree;
}

/**
 * Free decision tree
 */
static void nova_tree_free(NovaDecisionTree* tree) {
    if (tree == NULL) return;
    if (tree->nodes != NULL) {
        free(tree->nodes);
    }
    free(tree);
}

/* ============ Gradient and Hessian Computation ============ */

float* nova_compute_gradients(const float* predictions, const int* y, int n_samples) {
    float* gradients = (float*)malloc(n_samples * sizeof(float));
    
    for (int i = 0; i < n_samples; i++) {
        /* Log-loss gradient: (sigmoid(pred) - y) */
        float pred = sigmoid(predictions[i]);
        gradients[i] = pred - (float)y[i];
    }
    
    return gradients;
}

float* nova_compute_hessians(const float* predictions, const int* y, int n_samples) { (void)y;
    float* hessians = (float*)malloc(n_samples * sizeof(float));
    
    for (int i = 0; i < n_samples; i++) {
        /* Log-loss hessian: sigmoid(pred) * (1 - sigmoid(pred)) */
        float pred = sigmoid(predictions[i]);
        hessians[i] = pred * (1.0f - pred);
        /* Clip hessian to avoid numerical issues */
        if (hessians[i] < 1e-6f) hessians[i] = 1e-6f;
    }
    
    return hessians;
}

/* ============ Split Finding ============ */

float nova_find_best_split(const float* X, const float* y, int n_samples,
                           int n_features, int* best_feature, float* best_threshold) {
    float best_gain = 0.0f;
    *best_feature = 0;
    *best_threshold = 0.0f;
    
    if (n_samples < 2) return 0.0f;
    
    /* Compute total MSE for parent node */
    float parent_mse = compute_mse(y, n_samples);
    
    /* Try each feature */
    for (int feat = 0; feat < n_features; feat++) {
        /* Collect unique thresholds for this feature */
        float* thresholds = (float*)malloc(n_samples * sizeof(float));
        for (int i = 0; i < n_samples; i++) {
            thresholds[i] = X[i * n_features + feat];
        }
        
        /* Sort thresholds */
        for (int i = 0; i < n_samples - 1; i++) {
            for (int j = i + 1; j < n_samples; j++) {
                if (thresholds[i] > thresholds[j]) {
                    float tmp = thresholds[i];
                    thresholds[i] = thresholds[j];
                    thresholds[j] = tmp;
                }
            }
        }
        
        /* Try each threshold */
        for (int t = 0; t < n_samples - 1; t++) {
            float threshold = (thresholds[t] + thresholds[t + 1]) / 2.0f;
            
            /* Split samples */
            float* left_y = (float*)malloc(n_samples * sizeof(float));
            float* right_y = (float*)malloc(n_samples * sizeof(float));
            int n_left = 0, n_right = 0;
            
            for (int i = 0; i < n_samples; i++) {
                if (X[i * n_features + feat] <= threshold) {
                    left_y[n_left++] = y[i];
                } else {
                    right_y[n_right++] = y[i];
                }
            }
            
            if (n_left > 0 && n_right > 0) {
                /* Information gain = parent_mse - (n_left/n*mse_left + n_right/n*mse_right) */
                float left_mse = compute_mse(left_y, n_left);
                float right_mse = compute_mse(right_y, n_right);
                float weighted_mse = (float)n_left / n_samples * left_mse + 
                                    (float)n_right / n_samples * right_mse;
                float gain = parent_mse - weighted_mse;
                
                if (gain > best_gain) {
                    best_gain = gain;
                    *best_feature = feat;
                    *best_threshold = threshold;
                }
            }
            
            free(left_y);
            free(right_y);
        }
        
        free(thresholds);
    }
    
    return best_gain;
}

/* ============ Tree Building ============ */

void nova_build_tree_depth(NovaDecisionTree* tree, const float* X, const float* y,
                           int n_samples, int n_features, int current_depth, int max_depth) {
    if (n_samples == 0) return;
    
    /* Create leaf if max depth reached or pure node */
    if (current_depth >= max_depth || n_samples < 2) {
        float sum = 0.0f;
        for (int i = 0; i < n_samples; i++) {
            sum += y[i];
        }
        float leaf_value = sum / n_samples;
        
        NovaTreeNode* leaf = nova_tree_node_create();
        leaf->is_leaf = 1;
        leaf->leaf_value = leaf_value;
        leaf->n_samples = n_samples;
        
        tree->nodes = (NovaTreeNode*)realloc(tree->nodes, 
                                            (tree->n_nodes + 1) * sizeof(NovaTreeNode));
        tree->nodes[tree->n_nodes++] = *leaf;
        free(leaf);
        return;
    }
    
    /* Find best split */
    int best_feature = 0;
    float best_threshold = 0.0f;
    float gain = nova_find_best_split(X, y, n_samples, n_features, 
                                      &best_feature, &best_threshold);
    
    if (gain <= 0.0f) {
        /* No good split found, create leaf */
        float sum = 0.0f;
        for (int i = 0; i < n_samples; i++) {
            sum += y[i];
        }
        float leaf_value = sum / n_samples;
        
        NovaTreeNode* leaf = nova_tree_node_create();
        leaf->is_leaf = 1;
        leaf->leaf_value = leaf_value;
        leaf->n_samples = n_samples;
        
        tree->nodes = (NovaTreeNode*)realloc(tree->nodes, 
                                            (tree->n_nodes + 1) * sizeof(NovaTreeNode));
        tree->nodes[tree->n_nodes++] = *leaf;
        free(leaf);
        return;
    }
    
    /* Split data */
    float* X_left = (float*)malloc(n_samples * n_features * sizeof(float));
    float* y_left = (float*)malloc(n_samples * sizeof(float));
    float* X_right = (float*)malloc(n_samples * n_features * sizeof(float));
    float* y_right = (float*)malloc(n_samples * sizeof(float));
    
    int n_left = 0, n_right = 0;
    for (int i = 0; i < n_samples; i++) {
        if (X[i * n_features + best_feature] <= best_threshold) {
            memcpy(&X_left[n_left * n_features], &X[i * n_features], 
                   n_features * sizeof(float));
            y_left[n_left] = y[i];
            n_left++;
        } else {
            memcpy(&X_right[n_right * n_features], &X[i * n_features], 
                   n_features * sizeof(float));
            y_right[n_right] = y[i];
            n_right++;
        }
    }
    
    /* Create split node */
    NovaTreeNode* split_node = nova_tree_node_create();
    split_node->split_feature = best_feature;
    split_node->split_value = best_threshold;
    split_node->is_leaf = 0;
    split_node->n_samples = n_samples;
    
    tree->nodes = (NovaTreeNode*)realloc(tree->nodes, 
                                        (tree->n_nodes + 1) * sizeof(NovaTreeNode));
    tree->nodes[tree->n_nodes++] = *split_node;
    free(split_node);
    
    /* Recursively build left and right subtrees */
    nova_build_tree_depth(tree, X_left, y_left, n_left, n_features, 
                         current_depth + 1, max_depth);
    nova_build_tree_depth(tree, X_right, y_right, n_right, n_features, 
                         current_depth + 1, max_depth);
    
    free(X_left);
    free(y_left);
    free(X_right);
    free(y_right);
}

/* ============ Tree Prediction ============ */

float nova_tree_predict_sample(NovaDecisionTree* tree, const float* sample, int n_features) { (void)n_features;
    if (tree->nodes == NULL || tree->n_nodes == 0) {
        return 0.0f;
    }
    
    NovaTreeNode* node = &tree->nodes[0];
    
    while (!node->is_leaf) {
        int feat = node->split_feature;
        float threshold = node->split_value;
        
        if (sample[feat] <= threshold) {
            if (node->left != NULL) {
                node = node->left;
            } else {
                break;
            }
        } else {
            if (node->right != NULL) {
                node = node->right;
            } else {
                break;
            }
        }
    }
    
    return node->leaf_value;
}

/* ============ XGBoost Implementation ============ */

NovaXGBModel* nova_xgb_create(int n_features, int n_classes, float learning_rate) {
    NovaXGBModel* model = (NovaXGBModel*)malloc(sizeof(NovaXGBModel));
    model->trees = NULL;
    model->n_trees = 0;
    model->n_features = n_features;
    model->n_classes = n_classes;
    model->learning_rate = learning_rate;
    model->base_score = 0.5f;
    model->feature_importance = (float*)calloc(n_features, sizeof(float));
    return model;
}

void nova_xgb_free(NovaXGBModel* model) {
    if (model == NULL) return;
    
    for (int i = 0; i < model->n_trees; i++) {
        nova_tree_free(model->trees[i]);
    }
    free(model->trees);
    free(model->feature_importance);
    free(model);
}

void nova_xgb_fit(NovaXGBModel* model, const float* X, const int* y,
                  int n_samples, int n_features, int n_estimators, int max_depth) {
    /* Initialize predictions with base score */
    float* predictions = (float*)malloc(n_samples * sizeof(float));
    for (int i = 0; i < n_samples; i++) {
        predictions[i] = model->base_score;
    }
    
    /* Allocate trees array */
    model->trees = (NovaDecisionTree**)malloc(n_estimators * sizeof(NovaDecisionTree*));
    
    /* Boosting iterations */
    for (int iter = 0; iter < n_estimators; iter++) {
        /* Compute gradients */
        float* gradients = nova_compute_gradients(predictions, y, n_samples);
        
        /* Convert gradients to regression targets (residuals) */
        float* residuals = (float*)malloc(n_samples * sizeof(float));
        for (int i = 0; i < n_samples; i++) {
            residuals[i] = -gradients[i];
        }
        
        /* Build tree on residuals */
        NovaDecisionTree* tree = nova_tree_create(max_depth);
        nova_build_tree_depth(tree, X, residuals, n_samples, n_features, 0, max_depth);
        
        model->trees[model->n_trees++] = tree;
        
        /* Update predictions with tree output */
        for (int i = 0; i < n_samples; i++) {
            float tree_pred = nova_tree_predict_sample(tree, &X[i * n_features], n_features);
            predictions[i] += model->learning_rate * tree_pred;
        }
        
        free(gradients);
        free(residuals);
    }
    
    free(predictions);
}

int* nova_xgb_predict(NovaXGBModel* model, const float* X, int n_samples) {
    int* predictions = (int*)malloc(n_samples * sizeof(int));
    
    for (int i = 0; i < n_samples; i++) {
        float score = model->base_score;
        
        for (int t = 0; t < model->n_trees; t++) {
            float tree_pred = nova_tree_predict_sample(model->trees[t], 
                                                       &X[i * model->n_features],
                                                       model->n_features);
            score += model->learning_rate * tree_pred;
        }
        
        /* Apply sigmoid and threshold at 0.5 */
        predictions[i] = (sigmoid(score) > 0.5f) ? 1 : 0;
    }
    
    return predictions;
}

float* nova_xgb_predict_proba(NovaXGBModel* model, const float* X, 
                              int n_samples, int n_classes) {
    float* probabilities = (float*)malloc(n_samples * n_classes * sizeof(float));
    
    for (int i = 0; i < n_samples; i++) {
        float score = model->base_score;
        
        for (int t = 0; t < model->n_trees; t++) {
            float tree_pred = nova_tree_predict_sample(model->trees[t],
                                                       &X[i * model->n_features],
                                                       model->n_features);
            score += model->learning_rate * tree_pred;
        }
        
        float prob = sigmoid(score);
        probabilities[i * n_classes + 0] = 1.0f - prob;
        probabilities[i * n_classes + 1] = prob;
    }
    
    return probabilities;
}

/* ============ Gradient Boosting Implementation ============ */

NovaGBModel* nova_gb_create(int n_features, int n_classes, float learning_rate) {
    NovaGBModel* model = (NovaGBModel*)malloc(sizeof(NovaGBModel));
    model->trees = NULL;
    model->n_trees = 0;
    model->n_features = n_features;
    model->n_classes = n_classes;
    model->learning_rate = learning_rate;
    model->base_score = 0.5f;
    model->feature_importance = (float*)calloc(n_features, sizeof(float));
    return model;
}

void nova_gb_free(NovaGBModel* model) {
    if (model == NULL) return;
    
    for (int i = 0; i < model->n_trees; i++) {
        nova_tree_free(model->trees[i]);
    }
    free(model->trees);
    free(model->feature_importance);
    free(model);
}

void nova_gb_fit(NovaGBModel* model, const float* X, const int* y,
                 int n_samples, int n_features, int n_estimators, int max_depth) {
    /* Initialize predictions with base score */
    float* predictions = (float*)malloc(n_samples * sizeof(float));
    for (int i = 0; i < n_samples; i++) {
        predictions[i] = model->base_score;
    }
    
    /* Allocate trees array */
    model->trees = (NovaDecisionTree**)malloc(n_estimators * sizeof(NovaDecisionTree*));
    
    /* Boosting iterations */
    for (int iter = 0; iter < n_estimators; iter++) {
        /* Compute gradients (simple MSE loss) */
        float* residuals = (float*)malloc(n_samples * sizeof(float));
        for (int i = 0; i < n_samples; i++) {
            residuals[i] = (float)y[i] - predictions[i];
        }
        
        /* Build tree on residuals */
        NovaDecisionTree* tree = nova_tree_create(max_depth);
        nova_build_tree_depth(tree, X, residuals, n_samples, n_features, 0, max_depth);
        
        model->trees[model->n_trees++] = tree;
        
        /* Update predictions with tree output */
        for (int i = 0; i < n_samples; i++) {
            float tree_pred = nova_tree_predict_sample(tree, &X[i * n_features], n_features);
            predictions[i] += model->learning_rate * tree_pred;
        }
        
        free(residuals);
    }
    
    free(predictions);
}

int* nova_gb_predict(NovaGBModel* model, const float* X, int n_samples) {
    int* predictions = (int*)malloc(n_samples * sizeof(int));
    
    for (int i = 0; i < n_samples; i++) {
        float score = model->base_score;
        
        for (int t = 0; t < model->n_trees; t++) {
            float tree_pred = nova_tree_predict_sample(model->trees[t], 
                                                       &X[i * model->n_features],
                                                       model->n_features);
            score += model->learning_rate * tree_pred;
        }
        
        predictions[i] = (int)(score + 0.5f);
    }
    
    return predictions;
}

float* nova_gb_predict_proba(NovaGBModel* model, const float* X,
                             int n_samples, int n_classes) {
    float* probabilities = (float*)malloc(n_samples * n_classes * sizeof(float));
    
    for (int i = 0; i < n_samples; i++) {
        float score = model->base_score;
        
        for (int t = 0; t < model->n_trees; t++) {
            float tree_pred = nova_tree_predict_sample(model->trees[t],
                                                       &X[i * model->n_features],
                                                       model->n_features);
            score += model->learning_rate * tree_pred;
        }
        
        float prob = sigmoid(score);
        probabilities[i * n_classes + 0] = 1.0f - prob;
        probabilities[i * n_classes + 1] = prob;
    }
    
    return probabilities;
}

/* ============ LightGBM Implementation (Leaf-Wise) ============ */

NovaLGBMModel* nova_lgbm_create(int n_features, int n_classes, float learning_rate) {
    NovaLGBMModel* model = (NovaLGBMModel*)malloc(sizeof(NovaLGBMModel));
    model->trees = NULL;
    model->n_trees = 0;
    model->n_features = n_features;
    model->n_classes = n_classes;
    model->learning_rate = learning_rate;
    model->base_score = 0.5f;
    model->feature_importance = (float*)calloc(n_features, sizeof(float));
    return model;
}

void nova_lgbm_free(NovaLGBMModel* model) {
    if (model == NULL) return;
    
    for (int i = 0; i < model->n_trees; i++) {
        nova_tree_free(model->trees[i]);
    }
    free(model->trees);
    free(model->feature_importance);
    free(model);
}

void nova_lgbm_fit(NovaLGBMModel* model, const float* X, const int* y,
                   int n_samples, int n_features, int n_estimators, int max_depth) {
    /* Initialize predictions with base score */
    float* predictions = (float*)malloc(n_samples * sizeof(float));
    for (int i = 0; i < n_samples; i++) {
        predictions[i] = model->base_score;
    }
    
    /* Allocate trees array */
    model->trees = (NovaDecisionTree**)malloc(n_estimators * sizeof(NovaDecisionTree*));
    
    /* Boosting iterations */
    for (int iter = 0; iter < n_estimators; iter++) {
        /* Compute gradients */
        float* gradients = nova_compute_gradients(predictions, y, n_samples);
        
        /* Convert to regression targets */
        float* residuals = (float*)malloc(n_samples * sizeof(float));
        for (int i = 0; i < n_samples; i++) {
            residuals[i] = -gradients[i];
        }
        
        /* Build tree on residuals (leaf-wise similar to depth-wise for now) */
        NovaDecisionTree* tree = nova_tree_create(max_depth);
        nova_build_tree_depth(tree, X, residuals, n_samples, n_features, 0, max_depth);
        
        model->trees[model->n_trees++] = tree;
        
        /* Update predictions with tree output */
        for (int i = 0; i < n_samples; i++) {
            float tree_pred = nova_tree_predict_sample(tree, &X[i * n_features], n_features);
            predictions[i] += model->learning_rate * tree_pred;
        }
        
        free(gradients);
        free(residuals);
    }
    
    free(predictions);
}

int* nova_lgbm_predict(NovaLGBMModel* model, const float* X, int n_samples) {
    int* predictions = (int*)malloc(n_samples * sizeof(int));
    
    for (int i = 0; i < n_samples; i++) {
        float score = model->base_score;
        
        for (int t = 0; t < model->n_trees; t++) {
            float tree_pred = nova_tree_predict_sample(model->trees[t], 
                                                       &X[i * model->n_features],
                                                       model->n_features);
            score += model->learning_rate * tree_pred;
        }
        
        predictions[i] = (sigmoid(score) > 0.5f) ? 1 : 0;
    }
    
    return predictions;
}

float* nova_lgbm_predict_proba(NovaLGBMModel* model, const float* X,
                               int n_samples, int n_classes) {
    float* probabilities = (float*)malloc(n_samples * n_classes * sizeof(float));
    
    for (int i = 0; i < n_samples; i++) {
        float score = model->base_score;
        
        for (int t = 0; t < model->n_trees; t++) {
            float tree_pred = nova_tree_predict_sample(model->trees[t],
                                                       &X[i * model->n_features],
                                                       model->n_features);
            score += model->learning_rate * tree_pred;
        }
        
        float prob = sigmoid(score);
        probabilities[i * n_classes + 0] = 1.0f - prob;
        probabilities[i * n_classes + 1] = prob;
    }
    
    return probabilities;
}
