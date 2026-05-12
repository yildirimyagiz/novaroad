/**
 * @file normalization.c
 * @brief Normalization layer implementations
 */

#include "normalization.h"
#include "ai/tensor.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Layer Normalization */
nova_tensor_t* nova_layer_norm(nova_tensor_t* input, size_t normalized_shape, float eps) {
    if (!input) return NULL;
    
    // Create output tensor with same shape
    nova_tensor_t* output = nova_tensor_zeros(input->shape, input->ndim, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    // Calculate total elements
    size_t total_elements = 1;
    for (size_t i = 0; i < input->ndim; i++) {
        total_elements *= input->shape[i];
    }
    
    // Determine normalization dimensions
    size_t norm_size = 1;
    for (size_t i = input->ndim - normalized_shape; i < input->ndim; i++) {
        norm_size *= input->shape[i];
    }
    size_t num_groups = total_elements / norm_size;
    
    // Normalize each group
    for (size_t g = 0; g < num_groups; g++) {
        size_t offset = g * norm_size;
        
        // Calculate mean
        float mean = 0.0f;
        for (size_t i = 0; i < norm_size; i++) {
            mean += in_data[offset + i];
        }
        mean /= norm_size;
        
        // Calculate variance
        float var = 0.0f;
        for (size_t i = 0; i < norm_size; i++) {
            float diff = in_data[offset + i] - mean;
            var += diff * diff;
        }
        var /= norm_size;
        
        // Normalize
        float std = sqrtf(var + eps);
        for (size_t i = 0; i < norm_size; i++) {
            out_data[offset + i] = (in_data[offset + i] - mean) / std;
        }
    }
    
    return output;
}

/* Batch Normalization */
nova_tensor_t* nova_batch_norm(nova_tensor_t* input, 
                               nova_tensor_t* running_mean,
                               nova_tensor_t* running_var,
                               nova_tensor_t* gamma,
                               nova_tensor_t* beta,
                               float eps,
                               float momentum,
                               bool training) {
    if (!input) return NULL;
    
    nova_tensor_t* output = nova_tensor_zeros(input->shape, input->ndim, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    size_t batch_size = input->shape[0];
    size_t num_channels = input->shape[1];
    
    // Calculate spatial dimensions
    size_t spatial_size = 1;
    for (size_t i = 2; i < input->ndim; i++) {
        spatial_size *= input->shape[i];
    }
    
    size_t channel_stride = spatial_size;
    size_t batch_stride = num_channels * spatial_size;
    
    // Process each channel
    for (size_t c = 0; c < num_channels; c++) {
        float mean, var;
        
        if (training) {
            // Calculate batch statistics
            mean = 0.0f;
            for (size_t b = 0; b < batch_size; b++) {
                for (size_t s = 0; s < spatial_size; s++) {
                    size_t idx = b * batch_stride + c * channel_stride + s;
                    mean += in_data[idx];
                }
            }
            mean /= (batch_size * spatial_size);
            
            var = 0.0f;
            for (size_t b = 0; b < batch_size; b++) {
                for (size_t s = 0; s < spatial_size; s++) {
                    size_t idx = b * batch_stride + c * channel_stride + s;
                    float diff = in_data[idx] - mean;
                    var += diff * diff;
                }
            }
            var /= (batch_size * spatial_size);
            
            // Update running statistics
            if (running_mean && running_var) {
                float* rm_data = (float*)running_mean->data;
                float* rv_data = (float*)running_var->data;
                rm_data[c] = (1.0f - momentum) * rm_data[c] + momentum * mean;
                rv_data[c] = (1.0f - momentum) * rv_data[c] + momentum * var;
            }
        } else {
            // Use running statistics for inference
            if (running_mean && running_var) {
                mean = ((float*)running_mean->data)[c];
                var = ((float*)running_var->data)[c];
            } else {
                mean = 0.0f;
                var = 1.0f;
            }
        }
        
        // Normalize and apply affine transformation
        float scale = gamma ? ((float*)gamma->data)[c] : 1.0f;
        float shift = beta ? ((float*)beta->data)[c] : 0.0f;
        float std = sqrtf(var + eps);
        
        for (size_t b = 0; b < batch_size; b++) {
            for (size_t s = 0; s < spatial_size; s++) {
                size_t idx = b * batch_stride + c * channel_stride + s;
                out_data[idx] = scale * (in_data[idx] - mean) / std + shift;
            }
        }
    }
    
    return output;
}

/* Group Normalization */
nova_tensor_t* nova_group_norm(nova_tensor_t* input, size_t num_groups, float eps) {
    if (!input || input->ndim < 2) return NULL;
    
    size_t batch_size = input->shape[0];
    size_t num_channels = input->shape[1];
    
    if (num_channels % num_groups != 0) return NULL;
    
    nova_tensor_t* output = nova_tensor_zeros(input->shape, input->ndim, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    size_t channels_per_group = num_channels / num_groups;
    size_t spatial_size = 1;
    for (size_t i = 2; i < input->ndim; i++) {
        spatial_size *= input->shape[i];
    }
    
    size_t group_size = channels_per_group * spatial_size;
    
    // Normalize each group
    for (size_t b = 0; b < batch_size; b++) {
        for (size_t g = 0; g < num_groups; g++) {
            size_t group_offset = b * num_channels * spatial_size + g * group_size;
            
            // Calculate mean
            float mean = 0.0f;
            for (size_t i = 0; i < group_size; i++) {
                mean += in_data[group_offset + i];
            }
            mean /= group_size;
            
            // Calculate variance
            float var = 0.0f;
            for (size_t i = 0; i < group_size; i++) {
                float diff = in_data[group_offset + i] - mean;
                var += diff * diff;
            }
            var /= group_size;
            
            // Normalize
            float std = sqrtf(var + eps);
            for (size_t i = 0; i < group_size; i++) {
                out_data[group_offset + i] = (in_data[group_offset + i] - mean) / std;
            }
        }
    }
    
    return output;
}

/* Instance Normalization */
nova_tensor_t* nova_instance_norm(nova_tensor_t* input, float eps) {
    if (!input || input->ndim < 2) return NULL;
    
    // Instance norm is just group norm with num_groups = num_channels
    size_t num_channels = input->shape[1];
    return nova_group_norm(input, num_channels, eps);
}

/* RMS Normalization */
nova_tensor_t* nova_rms_norm(nova_tensor_t* input, float eps) {
    if (!input) return NULL;
    
    nova_tensor_t* output = nova_tensor_zeros(input->shape, input->ndim, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    // Normalize over last dimension
    size_t last_dim = input->shape[input->ndim - 1];
    size_t num_vectors = 1;
    for (size_t i = 0; i < input->ndim - 1; i++) {
        num_vectors *= input->shape[i];
    }
    
    for (size_t v = 0; v < num_vectors; v++) {
        size_t offset = v * last_dim;
        
        // Calculate RMS
        float rms = 0.0f;
        for (size_t i = 0; i < last_dim; i++) {
            float val = in_data[offset + i];
            rms += val * val;
        }
        rms = sqrtf(rms / last_dim + eps);
        
        // Normalize
        for (size_t i = 0; i < last_dim; i++) {
            out_data[offset + i] = in_data[offset + i] / rms;
        }
    }
    
    return output;
}
