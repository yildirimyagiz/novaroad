/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA OPTIM - AdamW Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * AdamW: Adam with Decoupled Weight Decay Regularization
 * Paper: "Decoupled Weight Decay Regularization" (Loshchilov & Hutter, 2019)
 * 
 * Key difference from Adam: Weight decay is applied directly to parameters
 * before gradient update, rather than being added to gradients.
 */

#include "nova_optim.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaAdamWOptimizer *nova_adamw_create(float lr, float beta1, float beta2, float eps,
                                      float weight_decay) {
    NovaAdamWOptimizer *opt = (NovaAdamWOptimizer *)malloc(sizeof(NovaAdamWOptimizer));
    if (!opt) {
        fprintf(stderr, "Failed to allocate AdamW optimizer\n");
        return NULL;
    }
    
    opt->lr = lr;
    opt->beta1 = beta1;
    opt->beta2 = beta2;
    opt->eps = eps;
    opt->weight_decay = weight_decay;
    opt->step = 0;
    opt->num_params = 0;
    opt->m_states = NULL;
    opt->v_states = NULL;
    
    return opt;
}

void nova_adamw_init_state(NovaAdamWOptimizer *opt, NovaTensor **params, int num_params) {
    if (!opt || !params) return;
    
    // Free existing state if present
    if (opt->m_states) {
        for (int i = 0; i < opt->num_params; i++) {
            if (opt->m_states[i]) nova_tensor_destroy(opt->m_states[i]);
            if (opt->v_states[i]) nova_tensor_destroy(opt->v_states[i]);
        }
        free(opt->m_states);
        free(opt->v_states);
    }
    
    opt->num_params = num_params;
    opt->m_states = (NovaTensor **)calloc(num_params, sizeof(NovaTensor *));
    opt->v_states = (NovaTensor **)calloc(num_params, sizeof(NovaTensor *));
    
    if (!opt->m_states || !opt->v_states) {
        fprintf(stderr, "Failed to allocate optimizer state\n");
        return;
    }
    
    // Initialize momentum and variance tensors (zeros)
    for (int i = 0; i < num_params; i++) {
        NovaTensor *param = params[i];
        
        // Create state tensors with same shape as parameter
        opt->m_states[i] = nova_tensor_create(NULL, param->shape, param->ndim, 
                                               param->dtype);
        opt->v_states[i] = nova_tensor_create(NULL, param->shape, param->ndim,
                                               param->dtype);
        
        // Zero initialize (already done by nova_tensor_create usually)
        if (opt->m_states[i]->data) {
            memset(opt->m_states[i]->data, 0, 
                   param->total_elements * sizeof(float));
        }
        if (opt->v_states[i]->data) {
            memset(opt->v_states[i]->data, 0,
                   param->total_elements * sizeof(float));
        }
    }
}

void nova_adamw_step(NovaAdamWOptimizer *opt, NovaTensor **params, int num_params) {
    if (!opt || !params) return;
    
    // Initialize state on first call
    if (opt->num_params == 0 || !opt->m_states) {
        nova_adamw_init_state(opt, params, num_params);
    }
    
    opt->step++;
    
    // Bias correction terms
    float bias_correction1 = 1.0f - powf(opt->beta1, (float)opt->step);
    float bias_correction2 = 1.0f - powf(opt->beta2, (float)opt->step);
    
    for (int i = 0; i < num_params; i++) {
        NovaTensor *param = params[i];
        if (!param->grad) continue; // Skip params without gradients
        
        NovaTensor *m = opt->m_states[i];
        NovaTensor *v = opt->v_states[i];
        
        float *p_data = (float *)param->data;
        float *g_data = (float *)param->grad->data;
        float *m_data = (float *)m->data;
        float *v_data = (float *)v->data;
        
        // Apply decoupled weight decay (AdamW key feature!)
        // p = p * (1 - lr * weight_decay)
        if (opt->weight_decay > 0.0f) {
            float decay_factor = 1.0f - opt->lr * opt->weight_decay;
            for (size_t j = 0; j < param->total_elements; j++) {
                p_data[j] *= decay_factor;
            }
        }
        
        // Update biased first moment estimate: m = beta1 * m + (1 - beta1) * g
        // Update biased second moment estimate: v = beta2 * v + (1 - beta2) * g^2
        for (size_t j = 0; j < param->total_elements; j++) {
            float grad = g_data[j];
            
            // First moment (momentum)
            m_data[j] = opt->beta1 * m_data[j] + (1.0f - opt->beta1) * grad;
            
            // Second moment (variance)
            v_data[j] = opt->beta2 * v_data[j] + (1.0f - opt->beta2) * grad * grad;
            
            // Bias-corrected moments
            float m_hat = m_data[j] / bias_correction1;
            float v_hat = v_data[j] / bias_correction2;
            
            // Parameter update: p = p - lr * m_hat / (sqrt(v_hat) + eps)
            p_data[j] -= opt->lr * m_hat / (sqrtf(v_hat) + opt->eps);
        }
    }
}

void nova_adamw_zero_grad(NovaTensor **params, int num_params) {
    for (int i = 0; i < num_params; i++) {
        if (params[i]->grad) {
            // Zero out gradient tensor
            memset(params[i]->grad->data, 0,
                   params[i]->total_elements * sizeof(float));
        }
    }
}

void nova_adamw_free(NovaAdamWOptimizer *opt) {
    if (!opt) return;
    
    if (opt->m_states) {
        for (int i = 0; i < opt->num_params; i++) {
            if (opt->m_states[i]) nova_tensor_destroy(opt->m_states[i]);
            if (opt->v_states[i]) nova_tensor_destroy(opt->v_states[i]);
        }
        free(opt->m_states);
        free(opt->v_states);
    }
    
    free(opt);
}

// ═══════════════════════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════════════════════

float nova_get_grad_norm(NovaTensor **params, int num_params) {
    float total_norm = 0.0f;
    
    for (int i = 0; i < num_params; i++) {
        if (!params[i]->grad) continue;
        
        float *g_data = (float *)params[i]->grad->data;
        for (size_t j = 0; j < params[i]->total_elements; j++) {
            total_norm += g_data[j] * g_data[j];
        }
    }
    
    return sqrtf(total_norm);
}

void nova_clip_grad_norm(NovaTensor **params, int num_params, float max_norm) {
    float total_norm = nova_get_grad_norm(params, num_params);
    
    if (total_norm > max_norm) {
        float clip_coef = max_norm / (total_norm + 1e-6f);
        
        for (int i = 0; i < num_params; i++) {
            if (!params[i]->grad) continue;
            
            float *g_data = (float *)params[i]->grad->data;
            for (size_t j = 0; j < params[i]->total_elements; j++) {
                g_data[j] *= clip_coef;
            }
        }
    }
}
