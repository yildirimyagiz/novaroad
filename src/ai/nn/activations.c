/**
 * @file activations.c
 * @brief Activation functions
 */

#include "ai/nn.h"
#include "ai/tensor.h"
#include <math.h>

/* ReLU: max(0, x) */
nova_tensor_t *nova_relu(nova_tensor_t *x)
{
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    for (size_t i = 0; i < size; i++) {
        if (data[i] < 0.0f) {
            data[i] = 0.0f;
        }
    }
    
    return output;
}

/* Leaky ReLU: max(0.01*x, x) */
nova_tensor_t *nova_leaky_relu(nova_tensor_t *x, float alpha)
{
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    for (size_t i = 0; i < size; i++) {
        if (data[i] < 0.0f) {
            data[i] *= alpha;
        }
    }
    
    return output;
}

/* GELU: x * Φ(x) where Φ is the cumulative distribution function */
nova_tensor_t *nova_gelu(nova_tensor_t *x)
{
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    for (size_t i = 0; i < size; i++) {
        float val = data[i];
        /* Approximation: 0.5 * x * (1 + tanh(sqrt(2/π) * (x + 0.044715 * x^3))) */
        float x3 = val * val * val;
        float inner = sqrtf(2.0f / M_PI) * (val + 0.044715f * x3);
        data[i] = 0.5f * val * (1.0f + tanhf(inner));
    }
    
    return output;
}

/* Sigmoid: 1 / (1 + exp(-x)) */
nova_tensor_t *nova_sigmoid(nova_tensor_t *x)
{
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    for (size_t i = 0; i < size; i++) {
        data[i] = 1.0f / (1.0f + expf(-data[i]));
    }
    
    return output;
}

/* Tanh: (exp(x) - exp(-x)) / (exp(x) + exp(-x)) */
nova_tensor_t *nova_tanh(nova_tensor_t *x)
{
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    for (size_t i = 0; i < size; i++) {
        data[i] = tanhf(data[i]);
    }
    
    return output;
}

/* Softmax: exp(x_i) / sum(exp(x_j)) */
nova_tensor_t *nova_softmax(nova_tensor_t *x, int dim)
{
    (void)dim;
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    /* Find max for numerical stability */
    float max_val = data[0];
    for (size_t i = 1; i < size; i++) {
        if (data[i] > max_val) max_val = data[i];
    }
    
    /* Compute exp(x - max) and sum */
    float sum = 0.0f;
    for (size_t i = 0; i < size; i++) {
        data[i] = expf(data[i] - max_val);
        sum += data[i];
    }
    
    /* Normalize */
    for (size_t i = 0; i < size; i++) {
        data[i] /= sum;
    }
    
    return output;
}

/* Swish/SiLU: x * sigmoid(x) */
nova_tensor_t *nova_swish(nova_tensor_t *x)
{
    if (!x) return NULL;
    
    nova_tensor_t *output = nova_tensor_clone(x);
    float *data = nova_tensor_data(output);
    size_t size = nova_tensor_numel(output);
    
    for (size_t i = 0; i < size; i++) {
        float val = data[i];
        data[i] = val / (1.0f + expf(-val));
    }
    
    return output;
}
