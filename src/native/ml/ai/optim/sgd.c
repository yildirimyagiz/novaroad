/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA OPTIM - SGD Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * SGD: Stochastic Gradient Descent with Momentum and Nesterov Acceleration
 * 
 * Variants:
 * - Vanilla SGD: momentum = 0
 * - SGD with Momentum: momentum > 0, nesterov = false
 * - Nesterov Accelerated Gradient (NAG): momentum > 0, nesterov = true
 */

#include "nova_optim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NovaSGD *nova_sgd_create(float lr, float momentum, float weight_decay, bool nesterov) {
    NovaSGD *opt = (NovaSGD *)malloc(sizeof(NovaSGD));
    if (!opt) {
        fprintf(stderr, "Failed to allocate SGD optimizer\n");
        return NULL;
    }
    
    opt->lr = lr;
    opt->momentum = momentum;
    opt->dampening = 0.0f; // PyTorch default
    opt->weight_decay = weight_decay;
    opt->nesterov = nesterov;
    opt->num_params = 0;
    opt->momentum_buffer = NULL;
    
    return opt;
}

void nova_sgd_init_state(NovaSGD *opt, NovaTensor **params, int num_params) {
    if (!opt || !params) return;
    
    // Free existing state
    if (opt->momentum_buffer) {
        for (int i = 0; i < opt->num_params; i++) {
            if (opt->momentum_buffer[i]) {
                nova_tensor_destroy(opt->momentum_buffer[i]);
            }
        }
        free(opt->momentum_buffer);
    }
    
    opt->num_params = num_params;
    
    // Only allocate momentum buffers if momentum > 0
    if (opt->momentum > 0.0f) {
        opt->momentum_buffer = (NovaTensor **)calloc(num_params, sizeof(NovaTensor *));
        if (!opt->momentum_buffer) {
            fprintf(stderr, "Failed to allocate SGD momentum buffers\n");
            return;
        }
        
        for (int i = 0; i < num_params; i++) {
            NovaTensor *param = params[i];
            opt->momentum_buffer[i] = nova_tensor_create(NULL, param->shape, 
                                                         param->ndim, param->dtype);
            if (opt->momentum_buffer[i]->data) {
                memset(opt->momentum_buffer[i]->data, 0,
                       param->total_elements * sizeof(float));
            }
        }
    }
}

void nova_sgd_step(NovaSGD *opt, NovaTensor **params, int num_params) {
    if (!opt || !params) return;
    
    // Initialize state on first call
    if (opt->num_params == 0 && opt->momentum > 0.0f) {
        nova_sgd_init_state(opt, params, num_params);
    }
    
    for (int i = 0; i < num_params; i++) {
        NovaTensor *param = params[i];
        if (!param->grad) continue;
        
        float *p_data = (float *)param->data;
        float *g_data = (float *)param->grad->data;
        
        // Apply L2 weight decay: g = g + weight_decay * p
        if (opt->weight_decay > 0.0f) {
            for (size_t j = 0; j < param->total_elements; j++) {
                g_data[j] += opt->weight_decay * p_data[j];
            }
        }
        
        // Apply momentum if enabled
        if (opt->momentum > 0.0f && opt->momentum_buffer) {
            float *buf_data = (float *)opt->momentum_buffer[i]->data;
            
            for (size_t j = 0; j < param->total_elements; j++) {
                // Update momentum buffer: buf = momentum * buf + (1 - dampening) * g
                buf_data[j] = opt->momentum * buf_data[j] + 
                              (1.0f - opt->dampening) * g_data[j];
                
                // Nesterov acceleration: g = g + momentum * buf
                if (opt->nesterov) {
                    g_data[j] += opt->momentum * buf_data[j];
                } else {
                    // Standard momentum: use buf as gradient
                    g_data[j] = buf_data[j];
                }
            }
        }
        
        // Parameter update: p = p - lr * g
        for (size_t j = 0; j < param->total_elements; j++) {
            p_data[j] -= opt->lr * g_data[j];
        }
    }
}

void nova_sgd_zero_grad(NovaTensor **params, int num_params) {
    for (int i = 0; i < num_params; i++) {
        if (params[i]->grad) {
            memset(params[i]->grad->data, 0,
                   params[i]->total_elements * sizeof(float));
        }
    }
}

void nova_sgd_free(NovaSGD *opt) {
    if (!opt) return;
    
    if (opt->momentum_buffer) {
        for (int i = 0; i < opt->num_params; i++) {
            if (opt->momentum_buffer[i]) {
                nova_tensor_destroy(opt->momentum_buffer[i]);
            }
        }
        free(opt->momentum_buffer);
    }
    
    free(opt);
}
