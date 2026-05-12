/**
 * optimizer_adamw.c - AdamW Optimizer Implementation
 * 
 * AdamW: Adam with decoupled weight decay
 * The state-of-the-art optimizer for transformers
 */

#include "../../include/ml/nova_training.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/**
 * Create AdamW optimizer
 */
NovaAdamWOptimizer *nova_optimizer_adamw_create(
    NovaTensor **parameters,
    int num_params,
    float learning_rate,
    float weight_decay
) {
    NovaAdamWOptimizer *opt = calloc(1, sizeof(NovaAdamWOptimizer));
    if (!opt) return NULL;
    
    opt->learning_rate = learning_rate;
    opt->beta1 = 0.9f;
    opt->beta2 = 0.999f;
    opt->epsilon = 1e-8f;
    opt->weight_decay = weight_decay;
    opt->step = 0;
    opt->num_params = num_params;
    
    // Allocate optimizer state
    opt->momentum = malloc(num_params * sizeof(NovaTensor*));
    opt->variance = malloc(num_params * sizeof(NovaTensor*));
    
    // Initialize momentum and variance tensors (same shape as parameters)
    for (int i = 0; i < num_params; i++) {
        opt->momentum[i] = nova_tensor_create(
            NULL,
            parameters[i]->shape,
            parameters[i]->ndim,
            NOVA_DTYPE_FP32
        );
        opt->variance[i] = nova_tensor_create(
            NULL,
            parameters[i]->shape,
            parameters[i]->ndim,
            NOVA_DTYPE_FP32
        );
        
        // Initialize to zero
        memset(opt->momentum[i]->data, 0, 
               opt->momentum[i]->total_elements * sizeof(float));
        memset(opt->variance[i]->data, 0,
               opt->variance[i]->total_elements * sizeof(float));
    }
    
    printf("✅ AdamW optimizer created\n");
    printf("   LR: %.6f, Weight decay: %.6f\n", learning_rate, weight_decay);
    printf("   Parameters: %d\n", num_params);
    
    return opt;
}

/**
 * Optimizer step - update all parameters
 */
void nova_optimizer_step(NovaAdamWOptimizer *opt, NovaTensor **parameters) {
    if (!opt || !parameters) return;
    
    opt->step++;
    
    // Bias correction
    float bias_correction1 = 1.0f - powf(opt->beta1, (float)opt->step);
    float bias_correction2 = 1.0f - powf(opt->beta2, (float)opt->step);
    
    for (int i = 0; i < opt->num_params; i++) {
        NovaTensor *param = parameters[i];
        NovaTensor *grad = param->grad;
        
        if (!grad) continue;  // Skip parameters without gradients
        
        float *p_data = (float *)param->data;
        float *g_data = (float *)grad->data;
        float *m_data = (float *)opt->momentum[i]->data;
        float *v_data = (float *)opt->variance[i]->data;
        
        for (size_t j = 0; j < param->total_elements; j++) {
            float grad_val = g_data[j];
            
            // Update biased first moment estimate
            m_data[j] = opt->beta1 * m_data[j] + (1.0f - opt->beta1) * grad_val;
            
            // Update biased second moment estimate
            v_data[j] = opt->beta2 * v_data[j] + 
                       (1.0f - opt->beta2) * grad_val * grad_val;
            
            // Compute bias-corrected estimates
            float m_hat = m_data[j] / bias_correction1;
            float v_hat = v_data[j] / bias_correction2;
            
            // AdamW update (decoupled weight decay)
            float update = opt->learning_rate * (m_hat / (sqrtf(v_hat) + opt->epsilon) +
                                                  opt->weight_decay * p_data[j]);
            
            p_data[j] -= update;
        }
    }
}

/**
 * Zero all gradients
 */
void nova_model_zero_grad(NovaGPTModel *model) {
    if (!model) return;
    
    int num_params;
    NovaTensor **params = nova_model_parameters(model, &num_params);
    
    for (int i = 0; i < num_params; i++) {
        if (params[i]->grad) {
            memset(params[i]->grad->data, 0,
                   params[i]->grad->total_elements * sizeof(float));
        }
    }
    
    free(params);
}

/**
 * Free optimizer
 */
void nova_optimizer_destroy(NovaAdamWOptimizer *opt) {
    if (!opt) return;
    
    for (int i = 0; i < opt->num_params; i++) {
        nova_tensor_destroy(opt->momentum[i]);
        nova_tensor_destroy(opt->variance[i]);
    }
    
    free(opt->momentum);
    free(opt->variance);
    free(opt);
}
