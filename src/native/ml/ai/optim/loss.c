/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA LOSS - Loss Function Implementations
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_loss.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// MSE Loss Implementation
// ═══════════════════════════════════════════════════════════════════════════

NovaTensor *nova_mse_loss(NovaTensor *predictions, NovaTensor *targets,
                          NovaReduction reduction) {
    if (!predictions || !targets) return NULL;
    
    // Validate shapes match
    if (predictions->total_elements != targets->total_elements) {
        fprintf(stderr, "MSE Loss: predictions and targets must have same shape\n");
        return NULL;
    }
    
    float *pred_data = (float *)predictions->data;
    float *target_data = (float *)targets->data;
    size_t n = predictions->total_elements;
    
    if (reduction == NOVA_REDUCTION_NONE) {
        // Return per-element loss
        NovaTensor *loss = nova_tensor_create(NULL, predictions->shape,
                                              predictions->ndim, predictions->dtype);
        float *loss_data = (float *)loss->data;
        
        for (size_t i = 0; i < n; i++) {
            float diff = pred_data[i] - target_data[i];
            loss_data[i] = diff * diff;
        }
        return loss;
    }
    
    // Compute sum of squared errors
    float total_loss = 0.0f;
    for (size_t i = 0; i < n; i++) {
        float diff = pred_data[i] - target_data[i];
        total_loss += diff * diff;
    }
    
    // Apply reduction
    if (reduction == NOVA_REDUCTION_MEAN) {
        total_loss /= (float)n;
    }
    
    // Return scalar tensor
    int64_t shape[1] = {1};
    NovaTensor *loss = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    ((float *)loss->data)[0] = total_loss;
    loss->requires_grad = true; // Enable gradient computation
    
    return loss;
}

// ═══════════════════════════════════════════════════════════════════════════
// Softmax Helper
// ═══════════════════════════════════════════════════════════════════════════

NovaTensor *nova_softmax(NovaTensor *input, int dim) {
    if (!input) return NULL;
    
    // For now, assume 2D input [batch, classes] and dim=-1
    if (input->ndim != 2) {
        fprintf(stderr, "Softmax currently only supports 2D tensors\n");
        return NULL;
    }
    
    int64_t batch_size = input->shape[0];
    int64_t num_classes = input->shape[1];
    
    NovaTensor *output = nova_tensor_create(NULL, input->shape, input->ndim,
                                            input->dtype);
    float *in_data = (float *)input->data;
    float *out_data = (float *)output->data;
    
    // Apply softmax per batch element
    for (int64_t b = 0; b < batch_size; b++) {
        float *in_row = in_data + b * num_classes;
        float *out_row = out_data + b * num_classes;
        
        // Find max for numerical stability
        float max_val = -FLT_MAX;
        for (int64_t c = 0; c < num_classes; c++) {
            if (in_row[c] > max_val) max_val = in_row[c];
        }
        
        // Compute exp and sum
        float sum_exp = 0.0f;
        for (int64_t c = 0; c < num_classes; c++) {
            out_row[c] = expf(in_row[c] - max_val);
            sum_exp += out_row[c];
        }
        
        // Normalize
        for (int64_t c = 0; c < num_classes; c++) {
            out_row[c] /= sum_exp;
        }
    }
    
    return output;
}

NovaTensor *nova_log_softmax(NovaTensor *input, int dim) {
    if (!input) return NULL;
    
    if (input->ndim != 2) {
        fprintf(stderr, "Log-softmax currently only supports 2D tensors\n");
        return NULL;
    }
    
    int64_t batch_size = input->shape[0];
    int64_t num_classes = input->shape[1];
    
    NovaTensor *output = nova_tensor_create(NULL, input->shape, input->ndim,
                                            input->dtype);
    float *in_data = (float *)input->data;
    float *out_data = (float *)output->data;
    
    for (int64_t b = 0; b < batch_size; b++) {
        float *in_row = in_data + b * num_classes;
        float *out_row = out_data + b * num_classes;
        
        // Find max for stability
        float max_val = -FLT_MAX;
        for (int64_t c = 0; c < num_classes; c++) {
            if (in_row[c] > max_val) max_val = in_row[c];
        }
        
        // Compute log-sum-exp
        float sum_exp = 0.0f;
        for (int64_t c = 0; c < num_classes; c++) {
            sum_exp += expf(in_row[c] - max_val);
        }
        float log_sum_exp = max_val + logf(sum_exp);
        
        // log_softmax = x - log_sum_exp
        for (int64_t c = 0; c < num_classes; c++) {
            out_row[c] = in_row[c] - log_sum_exp;
        }
    }
    
    return output;
}

// ═══════════════════════════════════════════════════════════════════════════
// Cross Entropy Loss
// ═══════════════════════════════════════════════════════════════════════════

NovaTensor *nova_cross_entropy_loss(NovaTensor *logits, NovaTensor *targets,
                                    NovaReduction reduction) {
    if (!logits || !targets) return NULL;
    
    // Assume logits: [batch_size, num_classes]
    // targets: [batch_size] (integer class indices)
    if (logits->ndim != 2) {
        fprintf(stderr, "Cross entropy expects 2D logits [batch, classes]\n");
        return NULL;
    }
    
    int64_t batch_size = logits->shape[0];
    int64_t num_classes = logits->shape[1];
    
    // Compute log-softmax for numerical stability
    NovaTensor *log_probs = nova_log_softmax(logits, -1);
    
    float *log_prob_data = (float *)log_probs->data;
    float *target_data = (float *)targets->data;
    
    float total_loss = 0.0f;
    
    for (int64_t b = 0; b < batch_size; b++) {
        int target_class = (int)target_data[b];
        if (target_class < 0 || target_class >= num_classes) {
            fprintf(stderr, "Invalid target class %d (must be 0-%ld)\n",
                    target_class, num_classes - 1);
            nova_tensor_destroy(log_probs);
            return NULL;
        }
        
        // Negative log likelihood: -log(p[target_class])
        float log_prob = log_prob_data[b * num_classes + target_class];
        total_loss += -log_prob;
    }
    
    nova_tensor_destroy(log_probs);
    
    // Apply reduction
    if (reduction == NOVA_REDUCTION_MEAN) {
        total_loss /= (float)batch_size;
    }
    
    // Return scalar loss
    int64_t shape[1] = {1};
    NovaTensor *loss = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    ((float *)loss->data)[0] = total_loss;
    loss->requires_grad = true;
    
    return loss;
}

NovaTensor *nova_cross_entropy_loss_onehot(NovaTensor *logits, NovaTensor *targets,
                                           NovaReduction reduction) {
    if (!logits || !targets) return NULL;
    
    // Both should be [batch_size, num_classes]
    if (logits->ndim != 2 || targets->ndim != 2) {
        fprintf(stderr, "One-hot cross entropy expects 2D tensors\n");
        return NULL;
    }
    
    int64_t batch_size = logits->shape[0];
    int64_t num_classes = logits->shape[1];
    
    NovaTensor *log_probs = nova_log_softmax(logits, -1);
    
    float *log_prob_data = (float *)log_probs->data;
    float *target_data = (float *)targets->data;
    
    float total_loss = 0.0f;
    
    // -Σ targets * log_probs
    for (size_t i = 0; i < logits->total_elements; i++) {
        total_loss += -target_data[i] * log_prob_data[i];
    }
    
    nova_tensor_destroy(log_probs);
    
    if (reduction == NOVA_REDUCTION_MEAN) {
        total_loss /= (float)batch_size;
    }
    
    int64_t shape[1] = {1};
    NovaTensor *loss = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    ((float *)loss->data)[0] = total_loss;
    loss->requires_grad = true;
    
    return loss;
}

// ═══════════════════════════════════════════════════════════════════════════
// Binary Cross Entropy
// ═══════════════════════════════════════════════════════════════════════════

NovaTensor *nova_bce_loss(NovaTensor *predictions, NovaTensor *targets,
                          NovaReduction reduction) {
    if (!predictions || !targets) return NULL;
    
    if (predictions->total_elements != targets->total_elements) {
        fprintf(stderr, "BCE: predictions and targets must have same shape\n");
        return NULL;
    }
    
    float *pred_data = (float *)predictions->data;
    float *target_data = (float *)targets->data;
    size_t n = predictions->total_elements;
    
    float total_loss = 0.0f;
    const float eps = 1e-7f; // For numerical stability
    
    for (size_t i = 0; i < n; i++) {
        float p = pred_data[i];
        float t = target_data[i];
        
        // Clamp predictions to [eps, 1-eps] to avoid log(0)
        p = fmaxf(eps, fminf(1.0f - eps, p));
        
        // BCE: -[t*log(p) + (1-t)*log(1-p)]
        total_loss += -(t * logf(p) + (1.0f - t) * logf(1.0f - p));
    }
    
    if (reduction == NOVA_REDUCTION_MEAN) {
        total_loss /= (float)n;
    }
    
    int64_t shape[1] = {1};
    NovaTensor *loss = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    ((float *)loss->data)[0] = total_loss;
    loss->requires_grad = true;
    
    return loss;
}

NovaTensor *nova_bce_with_logits_loss(NovaTensor *logits, NovaTensor *targets,
                                      NovaReduction reduction) {
    if (!logits || !targets) return NULL;
    
    float *logit_data = (float *)logits->data;
    float *target_data = (float *)targets->data;
    size_t n = logits->total_elements;
    
    float total_loss = 0.0f;
    
    for (size_t i = 0; i < n; i++) {
        float x = logit_data[i];
        float t = target_data[i];
        
        // Numerically stable: max(x, 0) - x*t + log(1 + exp(-|x|))
        float max_val = fmaxf(x, 0.0f);
        total_loss += max_val - x * t + logf(1.0f + expf(-fabsf(x)));
    }
    
    if (reduction == NOVA_REDUCTION_MEAN) {
        total_loss /= (float)n;
    }
    
    int64_t shape[1] = {1};
    NovaTensor *loss = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    ((float *)loss->data)[0] = total_loss;
    loss->requires_grad = true;
    
    return loss;
}
