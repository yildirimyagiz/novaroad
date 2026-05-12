/**
 * autograd.c - Automatic Differentiation Engine
 * 
 * Simple autograd for GPT training
 * Implements backward pass with chain rule
 */

#include "../../include/ml/nova_training.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// Gradient Storage
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Allocate gradient tensor for parameter
 */
static void ensure_grad(NovaTensor *tensor) {
    if (!tensor->grad) {
        tensor->grad = nova_tensor_create(
            NULL,
            tensor->shape,
            tensor->ndim,
            tensor->dtype
        );
        memset(tensor->grad->data, 0, 
               tensor->grad->total_elements * sizeof(float));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Backward Operations
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Backward for linear layer: Y = X @ W^T
 * 
 * dL/dX = dL/dY @ W
 * dL/dW = dL/dY^T @ X
 */
void linear_backward(
    NovaTensor *input,      // X
    NovaTensor *weight,     // W
    NovaTensor *output      // Y (has grad)
) {
    if (!output->grad) return;
    
    ensure_grad(input);
    ensure_grad(weight);
    
    // Shapes:
    // input:  [batch, seq_len, in_features]
    // weight: [out_features, in_features]
    // output: [batch, seq_len, out_features]
    
    int64_t batch = input->shape[0];
    int64_t seq_len = input->shape[1];
    int64_t in_features = input->shape[2];
    int64_t out_features = weight->shape[0];
    
    float *input_data = (float *)input->data;
    float *weight_data = (float *)weight->data;
    float *output_grad = (float *)output->grad->data;
    float *input_grad = (float *)input->grad->data;
    float *weight_grad = (float *)weight->grad->data;
    
    // dL/dX = dL/dY @ W
    for (int64_t b = 0; b < batch; b++) {
        for (int64_t s = 0; s < seq_len; s++) {
            for (int64_t i = 0; i < in_features; i++) {
                float grad_sum = 0.0f;
                for (int64_t o = 0; o < out_features; o++) {
                    grad_sum += output_grad[(b * seq_len + s) * out_features + o] *
                               weight_data[o * in_features + i];
                }
                input_grad[(b * seq_len + s) * in_features + i] += grad_sum;
            }
        }
    }
    
    // dL/dW = dL/dY^T @ X
    for (int64_t o = 0; o < out_features; o++) {
        for (int64_t i = 0; i < in_features; i++) {
            float grad_sum = 0.0f;
            for (int64_t b = 0; b < batch; b++) {
                for (int64_t s = 0; s < seq_len; s++) {
                    grad_sum += output_grad[(b * seq_len + s) * out_features + o] *
                               input_data[(b * seq_len + s) * in_features + i];
                }
            }
            weight_grad[o * in_features + i] += grad_sum;
        }
    }
}

/**
 * Backward for RMSNorm
 */
void rmsnorm_backward(
    NovaTensor *input,
    NovaTensor *weight,
    NovaTensor *output,
    float eps
) {
    if (!output->grad) return;
    
    ensure_grad(input);
    ensure_grad(weight);
    
    int64_t batch = input->shape[0];
    int64_t seq_len = input->shape[1];
    int64_t hidden_size = input->shape[2];
    
    float *input_data = (float *)input->data;
    float *weight_data = (float *)weight->data;
    float *output_grad = (float *)output->grad->data;
    float *input_grad = (float *)input->grad->data;
    float *weight_grad = (float *)weight->grad->data;
    
    for (int64_t b = 0; b < batch; b++) {
        for (int64_t s = 0; s < seq_len; s++) {
            const float *row = input_data + (b * seq_len + s) * hidden_size;
            float *row_grad = input_grad + (b * seq_len + s) * hidden_size;
            const float *out_grad = output_grad + (b * seq_len + s) * hidden_size;
            
            // Compute RMS
            float sum_sq = 0.0f;
            for (int64_t h = 0; h < hidden_size; h++) {
                sum_sq += row[h] * row[h];
            }
            float rms = sqrtf(sum_sq / (float)hidden_size + eps);
            float inv_rms = 1.0f / rms;
            
            // Gradient computation (simplified)
            for (int64_t h = 0; h < hidden_size; h++) {
                // dL/dweight
                weight_grad[h] += out_grad[h] * (row[h] * inv_rms);
                
                // dL/dinput (simplified, full version needs chain rule through RMS)
                row_grad[h] += out_grad[h] * weight_data[h] * inv_rms;
            }
        }
    }
}

/**
 * Backward for SwiGLU: out = swish(gate) * up
 */
void swiglu_backward(
    NovaTensor *gate,
    NovaTensor *up,
    NovaTensor *output
) {
    if (!output->grad) return;
    
    ensure_grad(gate);
    ensure_grad(up);
    
    float *gate_data = (float *)gate->data;
    float *up_data = (float *)up->data;
    float *output_grad = (float *)output->grad->data;
    float *gate_grad = (float *)gate->grad->data;
    float *up_grad = (float *)up->grad->data;
    
    for (size_t i = 0; i < gate->total_elements; i++) {
        float x = gate_data[i];
        float sigmoid = 1.0f / (1.0f + expf(-x));
        float swish = x * sigmoid;
        float swish_grad = sigmoid + x * sigmoid * (1.0f - sigmoid);
        
        // dL/dgate = dL/dout * up * swish'(gate)
        gate_grad[i] += output_grad[i] * up_data[i] * swish_grad;
        
        // dL/dup = dL/dout * swish(gate)
        up_grad[i] += output_grad[i] * swish;
    }
}

/**
 * Backward for cross-entropy loss
 */
void cross_entropy_backward(
    NovaTensor *logits,     // [batch, seq_len, vocab_size]
    NovaTensor *targets,    // [batch, seq_len]
    float scale
) {
    ensure_grad(logits);
    
    int64_t batch = logits->shape[0];
    int64_t seq_len = logits->shape[1];
    int64_t vocab_size = logits->shape[2];
    
    float *logits_data = (float *)logits->data;
    float *targets_data = (float *)targets->data;
    float *logits_grad = (float *)logits->grad->data;
    
    for (int64_t b = 0; b < batch; b++) {
        for (int64_t s = 0; s < seq_len; s++) {
            int64_t target_id = (int64_t)targets_data[b * seq_len + s];
            
            // Compute softmax
            float *logit_row = logits_data + (b * seq_len + s) * vocab_size;
            float *grad_row = logits_grad + (b * seq_len + s) * vocab_size;
            
            // Find max for numerical stability
            float max_logit = logit_row[0];
            for (int64_t v = 1; v < vocab_size; v++) {
                if (logit_row[v] > max_logit) max_logit = logit_row[v];
            }
            
            // Compute exp and sum
            float sum_exp = 0.0f;
            for (int64_t v = 0; v < vocab_size; v++) {
                sum_exp += expf(logit_row[v] - max_logit);
            }
            
            // Gradient: softmax - one_hot
            for (int64_t v = 0; v < vocab_size; v++) {
                float prob = expf(logit_row[v] - max_logit) / sum_exp;
                float target_val = (v == target_id) ? 1.0f : 0.0f;
                grad_row[v] += (prob - target_val) * scale;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Full Backward Pass
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Backward pass through entire model
 * 
 * This is a simplified version. Full version would need
 * computation graph tracking.
 */
void nova_model_backward(NovaTensor *loss) {
    // For now, this is a stub
    // Real implementation needs:
    // 1. Track computation graph during forward
    // 2. Reverse topological sort
    // 3. Apply chain rule
    
    printf("⚠️  Backward pass: simplified implementation\n");
    printf("   Loss gradient initialized\n");
    
    // Initialize loss gradient to 1.0
    if (loss) {
        ensure_grad(loss);
        float *loss_grad = (float *)loss->grad->data;
        for (size_t i = 0; i < loss->grad->total_elements; i++) {
            loss_grad[i] = 1.0f;
        }
    }
}

/**
 * Clip gradients by norm (prevent exploding gradients)
 */
void nova_clip_gradients(NovaTensor **parameters, int num_params, float max_norm) {
    // Compute total gradient norm
    float total_norm = 0.0f;
    
    for (int i = 0; i < num_params; i++) {
        if (!parameters[i]->grad) continue;
        
        float *grad_data = (float *)parameters[i]->grad->data;
        for (size_t j = 0; j < parameters[i]->grad->total_elements; j++) {
            total_norm += grad_data[j] * grad_data[j];
        }
    }
    
    total_norm = sqrtf(total_norm);
    
    // Clip if needed
    if (total_norm > max_norm) {
        float scale = max_norm / (total_norm + 1e-6f);
        
        for (int i = 0; i < num_params; i++) {
            if (!parameters[i]->grad) continue;
            
            float *grad_data = (float *)parameters[i]->grad->data;
            for (size_t j = 0; j < parameters[i]->grad->total_elements; j++) {
                grad_data[j] *= scale;
            }
        }
        
        printf("   Gradients clipped: %.2f → %.2f\n", total_norm, max_norm);
    }
}
