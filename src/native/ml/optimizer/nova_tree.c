#include "nova_tree.h"
#include "../ai/core/nova_metrics.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============ Tree Node Management ============ */

/**
 * Create a new tree node
 */
static NovaTreeNode* nova_tree_node_create(int n_classes) {
    NovaTreeNode* node = (NovaTreeNode*)malloc(sizeof(NovaTreeNode));
    node->feature_idx = -1;
    node->threshold = 0.0f;
    node->left = NULL;
    node->right = NULL;
    node->value = (float*)calloc(n_classes, sizeof(float));
    node->is_leaf = 0;
    node->n_samples = 0;
    node->impurity = 0.0f;
    return node;
}

/**
 * Free a tree node recursively
 */
static void nova_tree_node_free(NovaTreeNode* node) {
    if (node == NULL) return;
    nova_tree_node_free(node->left);
    nova_tree_node_free(node->right);
    if (node->value != NULL) {
        free(node->value);
    }
    free(node);
}

/* ============ Tree Creation/Destruction ============ */

NovaDecisionTree* nova_tree_create(int max_depth, int min_samples_split, int criterion) {
    NovaDecisionTree* tree = (NovaDecisionTree*)malloc(sizeof(NovaDecisionTree));
    tree->nodes = NULL;
    tree->n_nodes = 0;
    tree->capacity = 0;
    tree->max_depth = max_depth;
    tree->min_samples_split = min_samples_split;
    tree->min_samples_leaf = 1;
    tree->n_features = 0;
    tree->n_classes = 0;
    tree->criterion = criterion;
    return tree;
}

void nova_tree_free(NovaDecisionTree* tree) {
    if (tree == NULL) return;
    
    /* Free all nodes recursively if we have root */
    if (tree->nodes != NULL && tree->n_nodes > 0) {
        nova_tree_node_free(tree->nodes[0]);
    }
    
    if (tree->nodes != NULL) {
        free(tree->nodes);
    }
    free(tree);
}

/* ============ Impurity Measures ============ */

float nova_entropy(const int* y, int n_samples, int n_classes) {
    if (n_samples == 0) return 0.0f;
    
    /* Count class frequencies */
    int* counts = (int*)calloc(n_classes, sizeof(int));
    for (int i = 0; i < n_samples; i++) {
        if (y[i] >= 0 && y[i] < n_classes) {
            counts[y[i]]++;
        }
    }
    
    /* Entropy = -sum(p_i * log2(p_i)) */
    float entropy = 0.0f;
    for (int i = 0; i < n_classes; i++) {
        float p = (float)counts[i] / n_samples;
        if (p > 0.0f) {
            entropy -= p * logf(p) / logf(2.0f);
        }
    }
    
    free(counts);
    return entropy;
}

float nova_mse_impurity(const float* y, int n_samples) {
    if (n_samples == 0) return 0.0f;
    
    /* Compute mean */
    float mean = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        mean += y[i];
    }
    mean /= n_samples;
    
    /* Compute MSE */
    float mse = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        float diff = y[i] - mean;
        mse += diff * diff;
    }
    
    return mse / n_samples;
}

/* ============ Split Finding ============ */

float nova_best_split(const float* X, const void* y, int n_samples,
                      int n_features, int n_classes, int criterion,
                      int* best_feature, float* best_threshold) {
    float best_gain = 0.0f;
    *best_feature = 0;
    *best_threshold = 0.0f;
    
    if (n_samples < 2) return 0.0f;
    
    int is_regressor = (criterion == 2);
    
    /* Compute parent impurity */
    float parent_impurity;
    if (is_regressor) {
        parent_impurity = nova_mse_impurity((const float*)y, n_samples);
    } else {
        if (criterion == 1) {
            parent_impurity = nova_entropy((const int*)y, n_samples, n_classes);
        } else {
            parent_impurity = nova_gini_impurity((const int*)y, n_samples, n_classes);
        }
    }
    
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
        
        /* Try split between consecutive unique thresholds */
        for (int t = 0; t < n_samples - 1; t++) {
            /* Skip if same threshold */
            if (thresholds[t] == thresholds[t + 1]) {
                continue;
            }
            
            float threshold = (thresholds[t] + thresholds[t + 1]) / 2.0f;
            
            /* Split samples */
            int n_left = 0, n_right = 0;
            
            for (int i = 0; i < n_samples; i++) {
                if (X[i * n_features + feat] <= threshold) {
                    n_left++;
                } else {
                    n_right++;
                }
            }
            
            if (n_left < 1 || n_right < 1) {
                continue;
            }
            
            /* Compute weighted impurity after split */
            float left_impurity, right_impurity;
            
            if (is_regressor) {
                /* Regression: split float y */
                float* left_y = (float*)malloc(n_left * sizeof(float));
                float* right_y = (float*)malloc(n_right * sizeof(float));
                int li = 0, ri = 0;
                
                for (int i = 0; i < n_samples; i++) {
                    if (X[i * n_features + feat] <= threshold) {
                        left_y[li++] = ((const float*)y)[i];
                    } else {
                        right_y[ri++] = ((const float*)y)[i];
                    }
                }
                
                left_impurity = nova_mse_impurity(left_y, n_left);
                right_impurity = nova_mse_impurity(right_y, n_right);
                
                free(left_y);
                free(right_y);
            } else {
                /* Classification: split int y */
                int* left_y = (int*)malloc(n_left * sizeof(int));
                int* right_y = (int*)malloc(n_right * sizeof(int));
                int li = 0, ri = 0;
                
                for (int i = 0; i < n_samples; i++) {
                    if (X[i * n_features + feat] <= threshold) {
                        left_y[li++] = ((const int*)y)[i];
                    } else {
                        right_y[ri++] = ((const int*)y)[i];
                    }
                }
                
                if (criterion == 1) {
                    left_impurity = nova_entropy(left_y, n_left, n_classes);
                    right_impurity = nova_entropy(right_y, n_right, n_classes);
                } else {
                    left_impurity = nova_gini_impurity(left_y, n_left, n_classes);
                    right_impurity = nova_gini_impurity(right_y, n_right, n_classes);
                }
                
                free(left_y);
                free(right_y);
            }
            
            /* Information gain = parent - weighted_child */
            float weighted_child = ((float)n_left / n_samples) * left_impurity + 
                                   ((float)n_right / n_samples) * right_impurity;
            float gain = parent_impurity - weighted_child;
            
            if (gain > best_gain) {
                best_gain = gain;
                *best_feature = feat;
                *best_threshold = threshold;
            }
        }
        
        free(thresholds);
    }
    
    return best_gain;
}

/* ============ Tree Building - Classifier ============ */

void nova_tree_build_node(NovaDecisionTree* tree, const float* X, const int* y,
                          int n_samples, int n_features, int depth, int n_classes) {
    if (n_samples == 0) return;
    
    /* Check stopping criteria */
    int max_depth_reached = (tree->max_depth > 0 && depth >= tree->max_depth);
    int min_samples_reached = (n_samples < tree->min_samples_split);
    
    if (max_depth_reached || min_samples_reached) {
        /* Create leaf node */
        NovaTreeNode* leaf = nova_tree_node_create(n_classes);
        leaf->is_leaf = 1;
        leaf->n_samples = n_samples;
        leaf->impurity = nova_gini_impurity(y, n_samples, n_classes);
        
        /* Compute class counts and probabilities */
        int* counts = (int*)calloc(n_classes, sizeof(int));
        for (int i = 0; i < n_samples; i++) {
            if (y[i] >= 0 && y[i] < n_classes) {
                counts[y[i]]++;
            }
        }
        
        for (int c = 0; c < n_classes; c++) {
            leaf->value[c] = (float)counts[c] / n_samples;
        }
        
        free(counts);
        
        /* Add to tree */
        if (tree->n_nodes >= tree->capacity) {
            tree->capacity = (tree->capacity == 0) ? 10 : tree->capacity * 2;
            tree->nodes = (NovaTreeNode**)realloc(tree->nodes, 
                                                tree->capacity * sizeof(NovaTreeNode*));
        }
        tree->nodes[tree->n_nodes++] = leaf;
        return;
    }
    
    /* Find best split */
    int best_feature = 0;
    float best_threshold = 0.0f;
    float gain = nova_best_split(X, y, n_samples, n_features, n_classes,
                                 tree->criterion, &best_feature, &best_threshold);
    
    if (gain <= 0.0f) {
        /* No good split, create leaf */
        NovaTreeNode* leaf = nova_tree_node_create(n_classes);
        leaf->is_leaf = 1;
        leaf->n_samples = n_samples;
        leaf->impurity = nova_gini_impurity(y, n_samples, n_classes);
        
        int* counts = (int*)calloc(n_classes, sizeof(int));
        for (int i = 0; i < n_samples; i++) {
            if (y[i] >= 0 && y[i] < n_classes) {
                counts[y[i]]++;
            }
        }
        
        for (int c = 0; c < n_classes; c++) {
            leaf->value[c] = (float)counts[c] / n_samples;
        }
        
        free(counts);
        
        if (tree->n_nodes >= tree->capacity) {
            tree->capacity = (tree->capacity == 0) ? 10 : tree->capacity * 2;
            tree->nodes = (NovaTreeNode**)realloc(tree->nodes, 
                                                tree->capacity * sizeof(NovaTreeNode*));
        }
        tree->nodes[tree->n_nodes++] = leaf;
        return;
    }
    
    /* Split data */
    int* left_idx = (int*)malloc(n_samples * sizeof(int));
    int* right_idx = (int*)malloc(n_samples * sizeof(int));
    int n_left = 0, n_right = 0;
    
    for (int i = 0; i < n_samples; i++) {
        if (X[i * n_features + best_feature] <= best_threshold) {
            left_idx[n_left++] = i;
        } else {
            right_idx[n_right++] = i;
        }
    }
    
    /* Create split node */
    NovaTreeNode* split = nova_tree_node_create(n_classes);
    split->feature_idx = best_feature;
    split->threshold = best_threshold;
    split->is_leaf = 0;
    split->n_samples = n_samples;
    split->impurity = nova_gini_impurity(y, n_samples, n_classes);
    
    if (tree->n_nodes >= tree->capacity) {
        tree->capacity = (tree->capacity == 0) ? 10 : tree->capacity * 2;
        tree->nodes = (NovaTreeNode**)realloc(tree->nodes, 
                                            tree->capacity * sizeof(NovaTreeNode*));
    }
    tree->nodes[tree->n_nodes++] = split;
    
    /* Prepare data for children */
    float* X_left = (float*)malloc(n_left * n_features * sizeof(float));
    int* y_left = (int*)malloc(n_left * sizeof(int));
    float* X_right = (float*)malloc(n_right * n_features * sizeof(float));
    int* y_right = (int*)malloc(n_right * sizeof(int));
    
    for (int i = 0; i < n_left; i++) {
        memcpy(&X_left[i * n_features], &X[left_idx[i] * n_features], 
               n_features * sizeof(float));
        y_left[i] = y[left_idx[i]];
    }
    
    for (int i = 0; i < n_right; i++) {
        memcpy(&X_right[i * n_features], &X[right_idx[i] * n_features], 
               n_features * sizeof(float));
        y_right[i] = y[right_idx[i]];
    }
    
    /* Recursively build children */
    split->left = nova_tree_node_create(n_classes);
    split->right = nova_tree_node_create(n_classes);
    
    nova_tree_build_node(tree, X_left, y_left, n_left, n_features, depth + 1, n_classes);
    nova_tree_build_node(tree, X_right, y_right, n_right, n_features, depth + 1, n_classes);
    
    free(left_idx);
    free(right_idx);
    free(X_left);
    free(y_left);
    free(X_right);
    free(y_right);
}

/* ============ Tree Building - Regressor ============ */

void nova_tree_build_node_regressor(NovaDecisionTree* tree, const float* X, const float* y,
                                    int n_samples, int n_features, int depth) {
    if (n_samples == 0) return;
    
    /* Check stopping criteria */
    int max_depth_reached = (tree->max_depth > 0 && depth >= tree->max_depth);
    int min_samples_reached = (n_samples < tree->min_samples_split);
    
    if (max_depth_reached || min_samples_reached) {
        /* Create leaf node */
        NovaTreeNode* leaf = nova_tree_node_create(1);
        leaf->is_leaf = 1;
        leaf->n_samples = n_samples;
        leaf->impurity = nova_mse_impurity(y, n_samples);
        
        /* Mean value */
        float sum = 0.0f;
        for (int i = 0; i < n_samples; i++) {
            sum += y[i];
        }
        leaf->value[0] = sum / n_samples;
        
        if (tree->n_nodes >= tree->capacity) {
            tree->capacity = (tree->capacity == 0) ? 10 : tree->capacity * 2;
            tree->nodes = (NovaTreeNode**)realloc(tree->nodes, 
                                                tree->capacity * sizeof(NovaTreeNode*));
        }
        tree->nodes[tree->n_nodes++] = leaf;
        return;
    }
    
    /* Find best split */
    int best_feature = 0;
    float best_threshold = 0.0f;
    float gain = nova_best_split(X, (const void*)y, n_samples, n_features, 1,
                                 2, &best_feature, &best_threshold);
    
    if (gain <= 0.0f) {
        /* No good split, create leaf */
        NovaTreeNode* leaf = nova_tree_node_create(1);
        leaf->is_leaf = 1;
        leaf->n_samples = n_samples;
        leaf->impurity = nova_mse_impurity(y, n_samples);
        
        float sum = 0.0f;
        for (int i = 0; i < n_samples; i++) {
            sum += y[i];
        }
        leaf->value[0] = sum / n_samples;
        
        if (tree->n_nodes >= tree->capacity) {
            tree->capacity = (tree->capacity == 0) ? 10 : tree->capacity * 2;
            tree->nodes = (NovaTreeNode**)realloc(tree->nodes, 
                                                tree->capacity * sizeof(NovaTreeNode*));
        }
        tree->nodes[tree->n_nodes++] = leaf;
        return;
    }
    
    /* Split data */
    int* left_idx = (int*)malloc(n_samples * sizeof(int));
    int* right_idx = (int*)malloc(n_samples * sizeof(int));
    int n_left = 0, n_right = 0;
    
    for (int i = 0; i < n_samples; i++) {
        if (X[i * n_features + best_feature] <= best_threshold) {
            left_idx[n_left++] = i;
        } else {
            right_idx[n_right++] = i;
        }
    }
    
    /* Create split node */
    NovaTreeNode* split = nova_tree_node_create(1);
    split->feature_idx = best_feature;
    split->threshold = best_threshold;
    split->is_leaf = 0;
    split->n_samples = n_samples;
    split->impurity = nova_mse_impurity(y, n_samples);
    
    if (tree->n_nodes >= tree->capacity) {
        tree->capacity = (tree->capacity == 0) ? 10 : tree->capacity * 2;
        tree->nodes = (NovaTreeNode**)realloc(tree->nodes, 
                                            tree->capacity * sizeof(NovaTreeNode*));
    }
    tree->nodes[tree->n_nodes++] = split;
    
    /* Prepare data for children */
    float* X_left = (float*)malloc(n_left * n_features * sizeof(float));
    float* y_left = (float*)malloc(n_left * sizeof(float));
    float* X_right = (float*)malloc(n_right * n_features * sizeof(float));
    float* y_right = (float*)malloc(n_right * sizeof(float));
    
    for (int i = 0; i < n_left; i++) {
        memcpy(&X_left[i * n_features], &X[left_idx[i] * n_features], 
               n_features * sizeof(float));
        y_left[i] = y[left_idx[i]];
    }
    
    for (int i = 0; i < n_right; i++) {
        memcpy(&X_right[i * n_features], &X[right_idx[i] * n_features], 
               n_features * sizeof(float));
        y_right[i] = y[right_idx[i]];
    }
    
    /* Recursively build children */
    split->left = nova_tree_node_create(1);
    split->right = nova_tree_node_create(1);
    
    nova_tree_build_node_regressor(tree, X_left, y_left, n_left, n_features, depth + 1);
    nova_tree_build_node_regressor(tree, X_right, y_right, n_right, n_features, depth + 1);
    
    free(left_idx);
    free(right_idx);
    free(X_left);
    free(y_left);
    free(X_right);
    free(y_right);
}

/* ============ Training ============ */

void nova_tree_fit(NovaDecisionTree* tree, const float* X, const int* y,
                   int n_samples, int n_features, int n_classes) {
    tree->n_features = n_features;
    tree->n_classes = n_classes;
    
    nova_tree_build_node(tree, X, y, n_samples, n_features, 0, n_classes);
}

void nova_tree_fit_regressor(NovaDecisionTree* tree, const float* X, const float* y,
                             int n_samples, int n_features) {
    tree->n_features = n_features;
    tree->n_classes = 1;
    tree->criterion = 2; /* MSE */
    
    nova_tree_build_node_regressor(tree, X, y, n_samples, n_features, 0);
}

/* ============ Prediction ============ */

float* nova_node_predict_sample(NovaTreeNode* node, const float* sample, int n_features,
                                int n_classes) {
    (void)n_features; /* Unused parameter */
    
    if (node == NULL) {
        return (float*)calloc(n_classes, sizeof(float));
    }
    
    while (!node->is_leaf) {
        int feat = node->feature_idx;
        float threshold = node->threshold;
        
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
    
    float* result = (float*)malloc(n_classes * sizeof(float));
    memcpy(result, node->value, n_classes * sizeof(float));
    return result;
}

int* nova_tree_predict(NovaDecisionTree* tree, const float* X, int n_samples) {
    int* predictions = (int*)malloc(n_samples * sizeof(int));
    
    if (tree->nodes == NULL || tree->n_nodes == 0) {
        for (int i = 0; i < n_samples; i++) {
            predictions[i] = 0;
        }
        return predictions;
    }
    
    for (int i = 0; i < n_samples; i++) {
        float* proba = nova_node_predict_sample(tree->nodes[0], 
                                               &X[i * tree->n_features],
                                               tree->n_features,
                                               tree->n_classes);
        
        /* Find class with max probability */
        int best_class = 0;
        float max_prob = proba[0];
        for (int c = 1; c < tree->n_classes; c++) {
            if (proba[c] > max_prob) {
                max_prob = proba[c];
                best_class = c;
            }
        }
        
        predictions[i] = best_class;
        free(proba);
    }
    
    return predictions;
}

float* nova_tree_predict_proba(NovaDecisionTree* tree, const float* X,
                               int n_samples, int n_classes) {
    float* probabilities = (float*)malloc(n_samples * n_classes * sizeof(float));
    
    if (tree->nodes == NULL || tree->n_nodes == 0) {
        for (int i = 0; i < n_samples * n_classes; i++) {
            probabilities[i] = 1.0f / n_classes;
        }
        return probabilities;
    }
    
    for (int i = 0; i < n_samples; i++) {
        float* proba = nova_node_predict_sample(tree->nodes[0],
                                               &X[i * tree->n_features],
                                               tree->n_features,
                                               n_classes);
        
        memcpy(&probabilities[i * n_classes], proba, n_classes * sizeof(float));
        free(proba);
    }
    
    return probabilities;
}

float* nova_tree_predict_value(NovaDecisionTree* tree, const float* X, int n_samples) {
    float* predictions = (float*)malloc(n_samples * sizeof(float));
    
    if (tree->nodes == NULL || tree->n_nodes == 0) {
        for (int i = 0; i < n_samples; i++) {
            predictions[i] = 0.0f;
        }
        return predictions;
    }
    
    for (int i = 0; i < n_samples; i++) {
        float* value = nova_node_predict_sample(tree->nodes[0],
                                               &X[i * tree->n_features],
                                               tree->n_features,
                                               1);
        
        predictions[i] = value[0];
        free(value);
    }
    
    return predictions;
}
