#pragma once

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
typedef struct NovaTreeNode NovaTreeNode;
typedef struct NovaDecisionTree NovaDecisionTree;
typedef struct NovaXGBModel NovaXGBModel;
typedef struct NovaGBModel NovaGBModel;
typedef struct NovaLGBMModel NovaLGBMModel;

/* Tree node structure */
struct NovaTreeNode {
    int split_feature;       /* Feature index for split (-1 if leaf) */
    float split_value;       /* Threshold value for split */
    NovaTreeNode* left;      /* Left child node */
    NovaTreeNode* right;     /* Right child node */
    float leaf_value;        /* Prediction value if leaf */
    int is_leaf;             /* 1 if leaf node, 0 otherwise */
    int n_samples;           /* Number of samples in this node */
};

/* Decision tree structure */
struct NovaDecisionTree {
    NovaTreeNode* nodes;     /* Array of tree nodes */
    int n_nodes;             /* Current number of nodes */
    int max_depth;           /* Maximum tree depth */
};

/* XGBoost model structure */
struct NovaXGBModel {
    NovaDecisionTree** trees; /* Array of decision trees */
    int n_trees;             /* Number of trees (estimators) */
    int n_classes;           /* Number of classes */
    int n_features;          /* Number of input features */
    float learning_rate;     /* Learning rate / shrinkage */
    float base_score;        /* Initial prediction score */
    float* feature_importance; /* Feature importance scores */
};

/* Gradient Boosting model structure */
struct NovaGBModel {
    NovaDecisionTree** trees; /* Array of decision trees */
    int n_trees;             /* Number of trees (estimators) */
    int n_classes;           /* Number of classes */
    int n_features;          /* Number of input features */
    float learning_rate;     /* Learning rate */
    float base_score;        /* Initial prediction score */
    float* feature_importance; /* Feature importance scores */
};

/* LightGBM model structure (leaf-wise growth) */
struct NovaLGBMModel {
    NovaDecisionTree** trees; /* Array of decision trees */
    int n_trees;             /* Number of trees (estimators) */
    int n_classes;           /* Number of classes */
    int n_features;          /* Number of input features */
    float learning_rate;     /* Learning rate */
    float base_score;        /* Initial prediction score */
    float* feature_importance; /* Feature importance scores */
};

/* ============ XGBoost Functions ============ */

/**
 * Create a new XGBoost model
 * Returns: pointer to initialized NovaXGBModel
 */
NovaXGBModel* nova_xgb_create(int n_features, int n_classes, float learning_rate);

/**
 * Free XGBoost model and all allocated memory
 */
void nova_xgb_free(NovaXGBModel* model);

/**
 * Fit XGBoost model on training data
 * X: feature matrix (n_samples x n_features), row-major
 * y: labels (n_samples,)
 * n_samples: number of training samples
 * n_features: number of features
 * n_estimators: number of boosting rounds
 * max_depth: maximum tree depth
 * learning_rate: learning rate for Newton step
 */
void nova_xgb_fit(NovaXGBModel* model, const float* X, const int* y,
                  int n_samples, int n_features, int n_estimators, int max_depth);

/**
 * Predict class labels for samples using XGBoost
 * X: feature matrix (n_samples x n_features), row-major
 * n_samples: number of samples to predict
 * Returns: pointer to predicted labels (n_samples,)
 */
int* nova_xgb_predict(NovaXGBModel* model, const float* X, int n_samples);

/**
 * Predict class probabilities for samples using XGBoost
 * X: feature matrix (n_samples x n_features), row-major
 * n_samples: number of samples
 * n_classes: number of classes
 * Returns: pointer to probabilities (n_samples x n_classes), row-major
 */
float* nova_xgb_predict_proba(NovaXGBModel* model, const float* X, 
                              int n_samples, int n_classes);

/* ============ Gradient Boosting Functions ============ */

/**
 * Create a new Gradient Boosting model
 */
NovaGBModel* nova_gb_create(int n_features, int n_classes, float learning_rate);

/**
 * Free Gradient Boosting model
 */
void nova_gb_free(NovaGBModel* model);

/**
 * Fit Gradient Boosting model on training data
 */
void nova_gb_fit(NovaGBModel* model, const float* X, const int* y,
                 int n_samples, int n_features, int n_estimators, int max_depth);

/**
 * Predict using Gradient Boosting model
 */
int* nova_gb_predict(NovaGBModel* model, const float* X, int n_samples);

/**
 * Predict probabilities using Gradient Boosting model
 */
float* nova_gb_predict_proba(NovaGBModel* model, const float* X,
                             int n_samples, int n_classes);

/* ============ LightGBM Functions ============ */

/**
 * Create a new LightGBM model (leaf-wise growth)
 */
NovaLGBMModel* nova_lgbm_create(int n_features, int n_classes, float learning_rate);

/**
 * Free LightGBM model
 */
void nova_lgbm_free(NovaLGBMModel* model);

/**
 * Fit LightGBM model with leaf-wise tree growth
 */
void nova_lgbm_fit(NovaLGBMModel* model, const float* X, const int* y,
                   int n_samples, int n_features, int n_estimators, int max_depth);

/**
 * Predict using LightGBM model
 */
int* nova_lgbm_predict(NovaLGBMModel* model, const float* X, int n_samples);

/**
 * Predict probabilities using LightGBM model
 */
float* nova_lgbm_predict_proba(NovaLGBMModel* model, const float* X,
                               int n_samples, int n_classes);

/* ============ Helper Functions ============ */

/**
 * Compute gradients for XGBoost (log-loss for classification)
 * predictions: model predictions (n_samples,)
 * y: true labels (n_samples,)
 * n_samples: number of samples
 * Returns: pointer to gradient array (n_samples,)
 */
float* nova_compute_gradients(const float* predictions, const int* y, int n_samples);

/**
 * Compute hessians (second-order derivatives) for Newton step
 * predictions: model predictions (n_samples,)
 * y: true labels (n_samples,)
 * n_samples: number of samples
 * Returns: pointer to hessian array (n_samples,)
 */
float* nova_compute_hessians(const float* predictions, const int* y, int n_samples);

/**
 * Find best split for a node using greedy search
 * X: feature matrix subset (n_samples x n_features)
 * y: labels/residuals (n_samples,)
 * n_samples: number of samples in node
 * n_features: number of features
 * best_feature: output parameter for best feature index
 * best_threshold: output parameter for best split threshold
 * Returns: information gain of best split
 */
float nova_find_best_split(const float* X, const float* y, int n_samples,
                           int n_features, int* best_feature, float* best_threshold);

/**
 * Build tree recursively using depth-first approach
 * tree: tree structure to build
 * X: feature matrix (n_samples x n_features)
 * y: labels/residuals (n_samples,)
 * n_samples: number of samples
 * n_features: number of features
 * current_depth: current depth in recursion
 * max_depth: maximum allowed depth
 */
void nova_build_tree_depth(NovaDecisionTree* tree, const float* X, const float* y,
                           int n_samples, int n_features, int current_depth, int max_depth);

/**
 * Predict for a single sample using tree traversal
 * tree: decision tree
 * sample: feature vector for one sample (n_features,)
 * n_features: number of features
 * Returns: prediction value (leaf node value)
 */
float nova_tree_predict_sample(NovaDecisionTree* tree, const float* sample, int n_features);

