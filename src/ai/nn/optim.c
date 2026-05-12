/**
 * @file optim.c
 * @brief Optimizer implementations
 */

#include "optim.h"
#include "ai/tensor.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Optimizer structure */
struct nova_optimizer {
    nova_optim_type_t type;
    float learning_rate;
    float weight_decay;
    size_t step_count;
    
    /* Hyperparameters */
    union {
        struct {
            float momentum;
            bool nesterov;
            nova_tensor_t** velocity;  // Momentum buffers
        } sgd;
        
        struct {
            float beta1;
            float beta2;
            float eps;
            nova_tensor_t** m;  // First moment
            nova_tensor_t** v;  // Second moment
        } adam;
        
        struct {
            float alpha;
            float eps;
            float momentum;
            nova_tensor_t** square_avg;
            nova_tensor_t** momentum_buffer;
        } rmsprop;
        
        struct {
            float eps;
            nova_tensor_t** sum_of_squares;
        } adagrad;
    } state;
    
    size_t num_params;
};

/* SGD Optimizer */
nova_optimizer_t* nova_optim_sgd(float learning_rate, float momentum, float weight_decay, bool nesterov) {
    nova_optimizer_t* opt = nova_alloc(sizeof(nova_optimizer_t));
    if (!opt) return NULL;
    
    opt->type = momentum > 0 ? NOVA_OPTIM_MOMENTUM : NOVA_OPTIM_SGD;
    opt->learning_rate = learning_rate;
    opt->weight_decay = weight_decay;
    opt->step_count = 0;
    opt->state.sgd.momentum = momentum;
    opt->state.sgd.nesterov = nesterov;
    opt->state.sgd.velocity = NULL;
    opt->num_params = 0;
    
    return opt;
}

/* Adam Optimizer */
nova_optimizer_t* nova_optim_adam(float learning_rate, float beta1, float beta2, float eps, float weight_decay) {
    nova_optimizer_t* opt = nova_alloc(sizeof(nova_optimizer_t));
    if (!opt) return NULL;
    
    opt->type = NOVA_OPTIM_ADAM;
    opt->learning_rate = learning_rate;
    opt->weight_decay = weight_decay;
    opt->step_count = 0;
    opt->state.adam.beta1 = beta1;
    opt->state.adam.beta2 = beta2;
    opt->state.adam.eps = eps;
    opt->state.adam.m = NULL;
    opt->state.adam.v = NULL;
    opt->num_params = 0;
    
    return opt;
}

/* AdamW Optimizer */
nova_optimizer_t* nova_optim_adamw(float learning_rate, float beta1, float beta2, float eps, float weight_decay) {
    nova_optimizer_t* opt = nova_alloc(sizeof(nova_optimizer_t));
    if (!opt) return NULL;
    
    opt->type = NOVA_OPTIM_ADAMW;
    opt->learning_rate = learning_rate;
    opt->weight_decay = weight_decay;
    opt->step_count = 0;
    opt->state.adam.beta1 = beta1;
    opt->state.adam.beta2 = beta2;
    opt->state.adam.eps = eps;
    opt->state.adam.m = NULL;
    opt->state.adam.v = NULL;
    opt->num_params = 0;
    
    return opt;
}

/* RMSprop Optimizer */
nova_optimizer_t* nova_optim_rmsprop(float learning_rate, float alpha, float eps, float weight_decay, float momentum) {
    nova_optimizer_t* opt = nova_alloc(sizeof(nova_optimizer_t));
    if (!opt) return NULL;
    
    opt->type = NOVA_OPTIM_RMSPROP;
    opt->learning_rate = learning_rate;
    opt->weight_decay = weight_decay;
    opt->step_count = 0;
    opt->state.rmsprop.alpha = alpha;
    opt->state.rmsprop.eps = eps;
    opt->state.rmsprop.momentum = momentum;
    opt->state.rmsprop.square_avg = NULL;
    opt->state.rmsprop.momentum_buffer = NULL;
    opt->num_params = 0;
    
    return opt;
}

/* Adagrad Optimizer */
nova_optimizer_t* nova_optim_adagrad(float learning_rate, float eps, float weight_decay) {
    nova_optimizer_t* opt = nova_alloc(sizeof(nova_optimizer_t));
    if (!opt) return NULL;
    
    opt->type = NOVA_OPTIM_ADAGRAD;
    opt->learning_rate = learning_rate;
    opt->weight_decay = weight_decay;
    opt->step_count = 0;
    opt->state.adagrad.eps = eps;
    opt->state.adagrad.sum_of_squares = NULL;
    opt->num_params = 0;
    
    return opt;
}

/* Initialize optimizer state buffers */
static void init_optimizer_buffers(nova_optimizer_t* opt, nova_tensor_t** params, size_t num_params) {
    if (opt->num_params > 0) return; // Already initialized
    
    opt->num_params = num_params;
    
    switch (opt->type) {
        case NOVA_OPTIM_MOMENTUM:
            opt->state.sgd.velocity = nova_alloc(num_params * sizeof(nova_tensor_t*));
            for (size_t i = 0; i < num_params; i++) {
                opt->state.sgd.velocity[i] = nova_tensor_zeros(params[i]->shape, params[i]->ndim, params[i]->dtype);
            }
            break;
            
        case NOVA_OPTIM_ADAM:
        case NOVA_OPTIM_ADAMW:
            opt->state.adam.m = nova_alloc(num_params * sizeof(nova_tensor_t*));
            opt->state.adam.v = nova_alloc(num_params * sizeof(nova_tensor_t*));
            for (size_t i = 0; i < num_params; i++) {
                opt->state.adam.m[i] = nova_tensor_zeros(params[i]->shape, params[i]->ndim, params[i]->dtype);
                opt->state.adam.v[i] = nova_tensor_zeros(params[i]->shape, params[i]->ndim, params[i]->dtype);
            }
            break;
            
        case NOVA_OPTIM_RMSPROP:
            opt->state.rmsprop.square_avg = nova_alloc(num_params * sizeof(nova_tensor_t*));
            if (opt->state.rmsprop.momentum > 0) {
                opt->state.rmsprop.momentum_buffer = nova_alloc(num_params * sizeof(nova_tensor_t*));
            }
            for (size_t i = 0; i < num_params; i++) {
                opt->state.rmsprop.square_avg[i] = nova_tensor_zeros(params[i]->shape, params[i]->ndim, params[i]->dtype);
                if (opt->state.rmsprop.momentum > 0) {
                    opt->state.rmsprop.momentum_buffer[i] = nova_tensor_zeros(params[i]->shape, params[i]->ndim, params[i]->dtype);
                }
            }
            break;
            
        case NOVA_OPTIM_ADAGRAD:
            opt->state.adagrad.sum_of_squares = nova_alloc(num_params * sizeof(nova_tensor_t*));
            for (size_t i = 0; i < num_params; i++) {
                opt->state.adagrad.sum_of_squares[i] = nova_tensor_zeros(params[i]->shape, params[i]->ndim, params[i]->dtype);
            }
            break;
            
        default:
            break;
    }
}

/* Optimization step */
void nova_optim_step(nova_optimizer_t* optimizer, nova_tensor_t** params, nova_tensor_t** grads, size_t num_params) {
    if (!optimizer || !params || !grads) return;
    
    // Initialize buffers on first step
    if (optimizer->num_params == 0) {
        init_optimizer_buffers(optimizer, params, num_params);
    }
    
    optimizer->step_count++;
    float lr = optimizer->learning_rate;
    float wd = optimizer->weight_decay;
    
    for (size_t i = 0; i < num_params; i++) {
        if (!params[i] || !grads[i]) continue;
        
        float* param_data = (float*)params[i]->data;
        float* grad_data = (float*)grads[i]->data;
        
        size_t total_elements = 1;
        for (size_t j = 0; j < params[i]->ndim; j++) {
            total_elements *= params[i]->shape[j];
        }
        
        switch (optimizer->type) {
            case NOVA_OPTIM_SGD: {
                // Vanilla SGD
                for (size_t j = 0; j < total_elements; j++) {
                    float g = grad_data[j];
                    if (wd > 0) g += wd * param_data[j];  // L2 regularization
                    param_data[j] -= lr * g;
                }
                break;
            }
            
            case NOVA_OPTIM_MOMENTUM: {
                // SGD with momentum
                float* vel_data = (float*)optimizer->state.sgd.velocity[i]->data;
                float momentum = optimizer->state.sgd.momentum;
                
                for (size_t j = 0; j < total_elements; j++) {
                    float g = grad_data[j];
                    if (wd > 0) g += wd * param_data[j];
                    
                    vel_data[j] = momentum * vel_data[j] + g;
                    
                    if (optimizer->state.sgd.nesterov) {
                        param_data[j] -= lr * (g + momentum * vel_data[j]);
                    } else {
                        param_data[j] -= lr * vel_data[j];
                    }
                }
                break;
            }
            
            case NOVA_OPTIM_ADAM: {
                // Adam optimizer
                float* m_data = (float*)optimizer->state.adam.m[i]->data;
                float* v_data = (float*)optimizer->state.adam.v[i]->data;
                float beta1 = optimizer->state.adam.beta1;
                float beta2 = optimizer->state.adam.beta2;
                float eps = optimizer->state.adam.eps;
                
                float bias_correction1 = 1.0f - powf(beta1, optimizer->step_count);
                float bias_correction2 = 1.0f - powf(beta2, optimizer->step_count);
                
                for (size_t j = 0; j < total_elements; j++) {
                    float g = grad_data[j];
                    if (wd > 0) g += wd * param_data[j];  // L2 regularization
                    
                    // Update biased first moment
                    m_data[j] = beta1 * m_data[j] + (1.0f - beta1) * g;
                    
                    // Update biased second moment
                    v_data[j] = beta2 * v_data[j] + (1.0f - beta2) * g * g;
                    
                    // Compute bias-corrected moments
                    float m_hat = m_data[j] / bias_correction1;
                    float v_hat = v_data[j] / bias_correction2;
                    
                    // Update parameters
                    param_data[j] -= lr * m_hat / (sqrtf(v_hat) + eps);
                }
                break;
            }
            
            case NOVA_OPTIM_ADAMW: {
                // AdamW optimizer (decoupled weight decay)
                float* m_data = (float*)optimizer->state.adam.m[i]->data;
                float* v_data = (float*)optimizer->state.adam.v[i]->data;
                float beta1 = optimizer->state.adam.beta1;
                float beta2 = optimizer->state.adam.beta2;
                float eps = optimizer->state.adam.eps;
                
                float bias_correction1 = 1.0f - powf(beta1, optimizer->step_count);
                float bias_correction2 = 1.0f - powf(beta2, optimizer->step_count);
                
                for (size_t j = 0; j < total_elements; j++) {
                    float g = grad_data[j];
                    
                    // Update biased first moment
                    m_data[j] = beta1 * m_data[j] + (1.0f - beta1) * g;
                    
                    // Update biased second moment
                    v_data[j] = beta2 * v_data[j] + (1.0f - beta2) * g * g;
                    
                    // Compute bias-corrected moments
                    float m_hat = m_data[j] / bias_correction1;
                    float v_hat = v_data[j] / bias_correction2;
                    
                    // Update parameters with decoupled weight decay
                    param_data[j] -= lr * (m_hat / (sqrtf(v_hat) + eps) + wd * param_data[j]);
                }
                break;
            }
            
            case NOVA_OPTIM_RMSPROP: {
                // RMSprop optimizer
                float* sq_avg_data = (float*)optimizer->state.rmsprop.square_avg[i]->data;
                float alpha = optimizer->state.rmsprop.alpha;
                float eps = optimizer->state.rmsprop.eps;
                float momentum = optimizer->state.rmsprop.momentum;
                
                for (size_t j = 0; j < total_elements; j++) {
                    float g = grad_data[j];
                    if (wd > 0) g += wd * param_data[j];
                    
                    // Update square average
                    sq_avg_data[j] = alpha * sq_avg_data[j] + (1.0f - alpha) * g * g;
                    
                    float avg = sqrtf(sq_avg_data[j] + eps);
                    
                    if (momentum > 0 && optimizer->state.rmsprop.momentum_buffer) {
                        float* mom_buf_data = (float*)optimizer->state.rmsprop.momentum_buffer[i]->data;
                        mom_buf_data[j] = momentum * mom_buf_data[j] + g / avg;
                        param_data[j] -= lr * mom_buf_data[j];
                    } else {
                        param_data[j] -= lr * g / avg;
                    }
                }
                break;
            }
            
            case NOVA_OPTIM_ADAGRAD: {
                // Adagrad optimizer
                float* sum_sq_data = (float*)optimizer->state.adagrad.sum_of_squares[i]->data;
                float eps = optimizer->state.adagrad.eps;
                
                for (size_t j = 0; j < total_elements; j++) {
                    float g = grad_data[j];
                    if (wd > 0) g += wd * param_data[j];
                    
                    // Accumulate squared gradients
                    sum_sq_data[j] += g * g;
                    
                    // Update parameters
                    param_data[j] -= lr * g / (sqrtf(sum_sq_data[j]) + eps);
                }
                break;
            }
        }
    }
}

/* Zero gradients */
void nova_optim_zero_grad(nova_optimizer_t* optimizer) {
    // This is typically handled externally by zeroing gradient tensors
    (void)optimizer;
}

/* Get learning rate */
float nova_optim_get_lr(nova_optimizer_t* optimizer) {
    return optimizer ? optimizer->learning_rate : 0.0f;
}

/* Set learning rate */
void nova_optim_set_lr(nova_optimizer_t* optimizer, float lr) {
    if (optimizer) {
        optimizer->learning_rate = lr;
    }
}

/* Free optimizer */
void nova_optim_free(nova_optimizer_t* optimizer) {
    if (!optimizer) return;
    
    // Free state buffers
    switch (optimizer->type) {
        case NOVA_OPTIM_MOMENTUM:
            if (optimizer->state.sgd.velocity) {
                for (size_t i = 0; i < optimizer->num_params; i++) {
                    nova_tensor_destroy(optimizer->state.sgd.velocity[i]);
                }
                nova_free(optimizer->state.sgd.velocity);
            }
            break;
            
        case NOVA_OPTIM_ADAM:
        case NOVA_OPTIM_ADAMW:
            if (optimizer->state.adam.m) {
                for (size_t i = 0; i < optimizer->num_params; i++) {
                    nova_tensor_destroy(optimizer->state.adam.m[i]);
                    nova_tensor_destroy(optimizer->state.adam.v[i]);
                }
                nova_free(optimizer->state.adam.m);
                nova_free(optimizer->state.adam.v);
            }
            break;
            
        case NOVA_OPTIM_RMSPROP:
            if (optimizer->state.rmsprop.square_avg) {
                for (size_t i = 0; i < optimizer->num_params; i++) {
                    nova_tensor_destroy(optimizer->state.rmsprop.square_avg[i]);
                    if (optimizer->state.rmsprop.momentum_buffer) {
                        nova_tensor_destroy(optimizer->state.rmsprop.momentum_buffer[i]);
                    }
                }
                nova_free(optimizer->state.rmsprop.square_avg);
                if (optimizer->state.rmsprop.momentum_buffer) {
                    nova_free(optimizer->state.rmsprop.momentum_buffer);
                }
            }
            break;
            
        case NOVA_OPTIM_ADAGRAD:
            if (optimizer->state.adagrad.sum_of_squares) {
                for (size_t i = 0; i < optimizer->num_params; i++) {
                    nova_tensor_destroy(optimizer->state.adagrad.sum_of_squares[i]);
                }
                nova_free(optimizer->state.adagrad.sum_of_squares);
            }
            break;
            
        default:
            break;
    }
    
    nova_free(optimizer);
}
