/**
 * @file loss.c
 * @brief Loss function implementations
 */

#include "loss.h"
#include "ai/tensor.h"
#include <stdlib.h>
#include <math.h>

/* Mean Squared Error */
float nova_loss_mse(nova_tensor_t* predictions, nova_tensor_t* targets) {
    if (!predictions || !targets) return 0.0f;
    
    float* pred_data = (float*)predictions->data;
    float* target_data = (float*)targets->data;
    
    size_t total_elements = 1;
    for (size_t i = 0; i < predictions->ndim; i++) {
        total_elements *= predictions->shape[i];
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < total_elements; i++) {
        float diff = pred_data[i] - target_data[i];
        sum += diff * diff;
    }
    
    return sum / total_elements;
}

/* Mean Absolute Error */
float nova_loss_mae(nova_tensor_t* predictions, nova_tensor_t* targets) {
    if (!predictions || !targets) return 0.0f;
    
    float* pred_data = (float*)predictions->data;
    float* target_data = (float*)targets->data;
    
    size_t total_elements = 1;
    for (size_t i = 0; i < predictions->ndim; i++) {
        total_elements *= predictions->shape[i];
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < total_elements; i++) {
        sum += fabsf(pred_data[i] - target_data[i]);
    }
    
    return sum / total_elements;
}

/* Cross Entropy Loss with Softmax */
float nova_loss_cross_entropy(nova_tensor_t* logits, nova_tensor_t* targets) {
    if (!logits || !targets) return 0.0f;
    
    float* logit_data = (float*)logits->data;
    float* target_data = (float*)targets->data;
    
    // Assume shape is (batch_size, num_classes)
    size_t batch_size = logits->shape[0];
    size_t num_classes = logits->shape[1];
    
    float total_loss = 0.0f;
    
    for (size_t b = 0; b < batch_size; b++) {
        size_t offset = b * num_classes;
        
        // Find max for numerical stability
        float max_logit = logit_data[offset];
        for (size_t c = 1; c < num_classes; c++) {
            if (logit_data[offset + c] > max_logit) {
                max_logit = logit_data[offset + c];
            }
        }
        
        // Compute log-sum-exp
        float sum_exp = 0.0f;
        for (size_t c = 0; c < num_classes; c++) {
            sum_exp += expf(logit_data[offset + c] - max_logit);
        }
        float log_sum_exp = max_logit + logf(sum_exp);
        
        // Compute loss for this sample
        // If targets are class indices (assume integer stored as float)
        int target_class = (int)target_data[b];
        if (target_class >= 0 && target_class < (int)num_classes) {
            float log_prob = logit_data[offset + target_class] - log_sum_exp;
            total_loss -= log_prob;
        }
    }
    
    return total_loss / batch_size;
}

/* Binary Cross Entropy */
float nova_loss_bce(nova_tensor_t* predictions, nova_tensor_t* targets) {
    if (!predictions || !targets) return 0.0f;
    
    float* pred_data = (float*)predictions->data;
    float* target_data = (float*)targets->data;
    
    size_t total_elements = 1;
    for (size_t i = 0; i < predictions->ndim; i++) {
        total_elements *= predictions->shape[i];
    }
    
    float sum = 0.0f;
    const float eps = 1e-7f; // For numerical stability
    
    for (size_t i = 0; i < total_elements; i++) {
        float p = fmaxf(fminf(pred_data[i], 1.0f - eps), eps); // Clamp to [eps, 1-eps]
        float t = target_data[i];
        sum += -(t * logf(p) + (1.0f - t) * logf(1.0f - p));
    }
    
    return sum / total_elements;
}

/* Binary Cross Entropy with Logits */
float nova_loss_bce_with_logits(nova_tensor_t* logits, nova_tensor_t* targets) {
    if (!logits || !targets) return 0.0f;
    
    float* logit_data = (float*)logits->data;
    float* target_data = (float*)targets->data;
    
    size_t total_elements = 1;
    for (size_t i = 0; i < logits->ndim; i++) {
        total_elements *= logits->shape[i];
    }
    
    float sum = 0.0f;
    
    for (size_t i = 0; i < total_elements; i++) {
        float x = logit_data[i];
        float t = target_data[i];
        
        // Numerically stable version: max(x, 0) - x * t + log(1 + exp(-abs(x)))
        float max_val = fmaxf(x, 0.0f);
        sum += max_val - x * t + logf(1.0f + expf(-fabsf(x)));
    }
    
    return sum / total_elements;
}

/* Negative Log Likelihood */
float nova_loss_nll(nova_tensor_t* log_probs, nova_tensor_t* targets) {
    if (!log_probs || !targets) return 0.0f;
    
    float* log_prob_data = (float*)log_probs->data;
    float* target_data = (float*)targets->data;
    
    size_t batch_size = log_probs->shape[0];
    size_t num_classes = log_probs->shape[1];
    
    float sum = 0.0f;
    
    for (size_t b = 0; b < batch_size; b++) {
        int target_class = (int)target_data[b];
        if (target_class >= 0 && target_class < (int)num_classes) {
            sum -= log_prob_data[b * num_classes + target_class];
        }
    }
    
    return sum / batch_size;
}

/* Huber Loss */
float nova_loss_huber(nova_tensor_t* predictions, nova_tensor_t* targets, float delta) {
    if (!predictions || !targets) return 0.0f;
    
    float* pred_data = (float*)predictions->data;
    float* target_data = (float*)targets->data;
    
    size_t total_elements = 1;
    for (size_t i = 0; i < predictions->ndim; i++) {
        total_elements *= predictions->shape[i];
    }
    
    float sum = 0.0f;
    
    for (size_t i = 0; i < total_elements; i++) {
        float diff = fabsf(pred_data[i] - target_data[i]);
        if (diff <= delta) {
            sum += 0.5f * diff * diff;
        } else {
            sum += delta * (diff - 0.5f * delta);
        }
    }
    
    return sum / total_elements;
}

/* Smooth L1 Loss */
float nova_loss_smooth_l1(nova_tensor_t* predictions, nova_tensor_t* targets) {
    return nova_loss_huber(predictions, targets, 1.0f);
}

/* KL Divergence */
float nova_loss_kl_div(nova_tensor_t* predictions, nova_tensor_t* targets) {
    if (!predictions || !targets) return 0.0f;
    
    float* pred_data = (float*)predictions->data;
    float* target_data = (float*)targets->data;
    
    size_t total_elements = 1;
    for (size_t i = 0; i < predictions->ndim; i++) {
        total_elements *= predictions->shape[i];
    }
    
    float sum = 0.0f;
    const float eps = 1e-7f;
    
    for (size_t i = 0; i < total_elements; i++) {
        float p = fmaxf(pred_data[i], eps);
        float q = fmaxf(target_data[i], eps);
        sum += p * logf(p / q);
    }
    
    return sum / total_elements;
}
