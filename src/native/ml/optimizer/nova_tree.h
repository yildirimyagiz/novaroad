#pragma once

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
typedef struct NovaTreeNode NovaTreeNode;
typedef struct NovaDecisionTree NovaDecisionTree;

/* Tree node structure for decision trees */
struct NovaTreeNode {
    int feature_idx;         /* Feature index for split (-1 if leaf) */
    float threshold;         /* Split threshold value */
    NovaTreeNode* left;      /* Left child node (feature_idx <= threshold) */
    NovaTreeNode* right;     /* Right child node (feature_idx > threshold) */
    float* value;            /* Prediction value(s) - array for multi-class */
    int is_leaf;             /* 1 if leaf node, 0 otherwise */
    int n_samples;           /* Number of samples in this node */
    float impurity;          /* Gini/entropy/MSE value at this node */
};

/* Decision tree structure */
struct NovaDecisionTree {
    NovaTreeNode** nodes;    /* Array of pointers to nodes */
    int n_nodes;             /* Current number of nodes */
    int capacity;            /* Capacity of nodes array */
    int max_depth;           /* Maximum tree depth */
    int min_samples_split;   /* Minimum samples to split */
    int min_samples_leaf;    /* Minimum samples in leaf */
    int n_features;          /* Number of input features */
    int n_classes;           /* Number of classes (for classifier) */
    int criterion;           /* 0=gini, 1=entropy, 2=mse */
};

/* ============ Tree Creation/Destruction ============ */

/**
 * Create a new decision tree
 * max_depth: maximum depth (0 = unlimited)
 * min_samples_split: minimum samples to consider split (default 2)
 * criterion: 0=gini, 1=entropy, 2=mse
 * Returns: pointer to initialized NovaDecisionTree
 */
NovaDecisionTree* nova_tree_create(int max_depth, int min_samples_split, int criterion);

/**
 * Free decision tree and all allocated memory
 */
void nova_tree_free(NovaDecisionTree* tree);

/* ============ Training ============ */

/**
 * Fit decision tree on training data (classifier)
 * X: feature matrix (n_samples x n_features), row-major, float
 * y: class labels (n_samples,), int (0 to n_classes-1)
 * n_samples: number of training samples
 * n_features: number of features
 * n_classes: number of classes
 */
void nova_tree_fit(NovaDecisionTree* tree, const float* X, const int* y,
                   int n_samples, int n_features, int n_classes);

/**
 * Fit decision tree regressor on training data
 * X: feature matrix (n_samples x n_features), row-major, float
 * y: regression targets (n_samples,), float
 * n_samples: number of training samples
 * n_features: number of features
 */
void nova_tree_fit_regressor(NovaDecisionTree* tree, const float* X, const float* y,
                             int n_samples, int n_features);

/* ============ Prediction - Classifier ============ */

/**
 * Predict class labels for samples
 * X: feature matrix (n_samples x n_features), row-major
 * n_samples: number of samples to predict
 * Returns: pointer to predicted labels (n_samples,), allocated by function
 */
int* nova_tree_predict(NovaDecisionTree* tree, const float* X, int n_samples);

/**
 * Predict class probabilities for samples
 * X: feature matrix (n_samples x n_features), row-major
 * n_samples: number of samples
 * n_classes: number of classes
 * Returns: pointer to probabilities (n_samples x n_classes), row-major, allocated by function
 */
float* nova_tree_predict_proba(NovaDecisionTree* tree, const float* X,
                               int n_samples, int n_classes);

/* ============ Prediction - Regressor ============ */

/**
 * Predict regression values for samples
 * X: feature matrix (n_samples x n_features), row-major
 * n_samples: number of samples to predict
 * Returns: pointer to predicted values (n_samples,), allocated by function
 */
float* nova_tree_predict_value(NovaDecisionTree* tree, const float* X, int n_samples);

/* ============ Impurity Measures ============ */

/**
 * Compute Gini impurity for a set of class labels
 * y: class labels (n_samples,)
 * n_samples: number of samples
 * n_classes: number of classes
 * Returns: Gini impurity value [0, 1]
 */
float nova_gini_impurity(const int* labels, size_t n_samples, int n_classes);

/**
 * Compute entropy for a set of class labels
 * y: class labels (n_samples,)
 * n_samples: number of samples
 * n_classes: number of classes
 * Returns: entropy value
 */
float nova_entropy(const int* y, int n_samples, int n_classes);

/**
 * Compute MSE impurity for regression
 * y: regression targets (n_samples,)
 * n_samples: number of samples
 * Returns: mean squared error from mean
 */
float nova_mse_impurity(const float* y, int n_samples);

/* ============ Internal Split Functions ============ */

/**
 * Find best split for a node (greedy search)
 * X: feature matrix subset (n_samples x n_features)
 * y: labels (n_samples,) - int for classifier, float for regressor
 * n_samples: number of samples in this node
 * n_features: number of features
 * n_classes: number of classes (for classifier, ignored for regressor)
 * criterion: 0=gini, 1=entropy, 2=mse
 * best_feature: output parameter for best feature index
 * best_threshold: output parameter for best split threshold
 * Returns: information gain of best split
 */
float nova_best_split(const float* X, const void* y, int n_samples,
                      int n_features, int n_classes, int criterion,
                      int* best_feature, float* best_threshold);

/**
 * Recursively build tree nodes (depth-first traversal)
 * tree: tree structure to build into
 * X: feature matrix (n_samples x n_features)
 * y: labels (n_samples,) - int for classifier
 * n_samples: number of samples
 * n_features: number of features
 * depth: current depth in recursion
 * n_classes: number of classes
 */
void nova_tree_build_node(NovaDecisionTree* tree, const float* X, const int* y,
                          int n_samples, int n_features, int depth, int n_classes);

/**
 * Recursively build tree nodes for regressor
 * tree: tree structure to build into
 * X: feature matrix (n_samples x n_features)
 * y: regression targets (n_samples,)
 * n_samples: number of samples
 * n_features: number of features
 * depth: current depth in recursion
 */
void nova_tree_build_node_regressor(NovaDecisionTree* tree, const float* X, const float* y,
                                    int n_samples, int n_features, int depth);

/* ============ Utility Functions ============ */

/**
 * Predict for a single sample (internal use)
 * node: current node in tree traversal
 * sample: feature vector for one sample (n_features,)
 * n_features: number of features
 * n_classes: number of classes
 * Returns: pointer to prediction value(s)
 */
float* nova_node_predict_sample(NovaTreeNode* node, const float* sample, int n_features,
                                int n_classes);

