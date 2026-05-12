#ifndef NOVA_LOSS_H
#define NOVA_LOSS_H

#include "nova_tensor.h"
#include <math.h>
#include <stdbool.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA LOSS - Machine Learning Loss Functions
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Common loss functions for training neural networks:
 * - MSE (Mean Squared Error) - Regression
 * - Cross Entropy - Classification
 * - Binary Cross Entropy - Binary classification
 */

typedef enum {
    NOVA_REDUCTION_NONE,    // No reduction, return per-element loss
    NOVA_REDUCTION_MEAN,    // Average loss over all elements
    NOVA_REDUCTION_SUM      // Sum loss over all elements
} NovaReduction;

// ═══════════════════════════════════════════════════════════════════════════
// Mean Squared Error Loss
// ═══════════════════════════════════════════════════════════════════════════

/**
 * MSE Loss: L = 1/N * Σ(predictions - targets)^2
 * 
 * Use for: Regression tasks
 * Gradient: dL/dp = 2/N * (predictions - targets)
 * 
 * @param predictions Model predictions [N, ...]
 * @param targets Ground truth values [N, ...]
 * @param reduction How to reduce the loss (mean/sum/none)
 * @return Loss tensor (scalar if reduction != NONE)
 */
NovaTensor *nova_mse_loss(NovaTensor *predictions, NovaTensor *targets,
                          NovaReduction reduction);

// ═══════════════════════════════════════════════════════════════════════════
// Cross Entropy Loss
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Cross Entropy Loss: L = -Σ targets * log(softmax(predictions))
 * 
 * Use for: Multi-class classification
 * Input:
 *   - predictions: Raw logits [batch_size, num_classes]
 *   - targets: Class indices [batch_size] (as integers)
 * 
 * Note: Applies softmax internally for numerical stability
 * 
 * @param logits Raw model outputs [N, C]
 * @param targets Class indices [N] (0 to C-1)
 * @param reduction How to reduce the loss
 * @return Loss tensor
 */
NovaTensor *nova_cross_entropy_loss(NovaTensor *logits, NovaTensor *targets,
                                    NovaReduction reduction);

/**
 * Cross Entropy with one-hot targets
 * 
 * @param logits Raw model outputs [N, C]
 * @param targets One-hot encoded labels [N, C]
 * @param reduction How to reduce the loss
 * @return Loss tensor
 */
NovaTensor *nova_cross_entropy_loss_onehot(NovaTensor *logits, NovaTensor *targets,
                                           NovaReduction reduction);

// ═══════════════════════════════════════════════════════════════════════════
// Binary Cross Entropy Loss
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Binary Cross Entropy: L = -[y*log(p) + (1-y)*log(1-p)]
 * 
 * Use for: Binary classification
 * Input predictions should be in [0, 1] (apply sigmoid first if needed)
 * 
 * @param predictions Predicted probabilities [N, ...] in range [0, 1]
 * @param targets Ground truth binary labels [N, ...] (0 or 1)
 * @param reduction How to reduce the loss
 * @return Loss tensor
 */
NovaTensor *nova_bce_loss(NovaTensor *predictions, NovaTensor *targets,
                          NovaReduction reduction);

/**
 * Binary Cross Entropy with Logits (numerically stable)
 * 
 * Combines sigmoid + BCE for better numerical stability
 * Use this instead of sigmoid + bce_loss
 * 
 * @param logits Raw model outputs (not sigmoidified)
 * @param targets Ground truth binary labels (0 or 1)
 * @param reduction How to reduce the loss
 * @return Loss tensor
 */
NovaTensor *nova_bce_with_logits_loss(NovaTensor *logits, NovaTensor *targets,
                                      NovaReduction reduction);

// ═══════════════════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Softmax activation (used internally by cross_entropy)
 * 
 * @param input Input tensor [N, C]
 * @param dim Dimension to apply softmax (typically last dim)
 * @return Softmax probabilities
 */
NovaTensor *nova_softmax(NovaTensor *input, int dim);

/**
 * Log-Softmax (numerically stable log(softmax(x)))
 * 
 * @param input Input tensor
 * @param dim Dimension to apply log-softmax
 * @return Log probabilities
 */
NovaTensor *nova_log_softmax(NovaTensor *input, int dim);

#endif // NOVA_LOSS_H
