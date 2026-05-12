/**
 * @file pooling.c
 * @brief Pooling layer implementations
 */

#include "pooling.h"
#include "ai/tensor.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* MaxPool2D */
nova_tensor_t* nova_pool2d_max(nova_tensor_t* input, size_t kernel_size, size_t stride, size_t padding) {
    if (!input || input->ndim != 4) return NULL;
    
    size_t batch = input->shape[0];
    size_t channels = input->shape[1];
    size_t in_h = input->shape[2];
    size_t in_w = input->shape[3];
    
    // Calculate output dimensions
    size_t out_h = (in_h + 2 * padding - kernel_size) / stride + 1;
    size_t out_w = (in_w + 2 * padding - kernel_size) / stride + 1;
    
    size_t out_shape[4] = {batch, channels, out_h, out_w};
    nova_tensor_t* output = nova_tensor_zeros(out_shape, 4, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    // Perform max pooling
    for (size_t b = 0; b < batch; b++) {
        for (size_t c = 0; c < channels; c++) {
            for (size_t oh = 0; oh < out_h; oh++) {
                for (size_t ow = 0; ow < out_w; ow++) {
                    float max_val = -FLT_MAX;
                    
                    // Find max in pooling window
                    for (size_t kh = 0; kh < kernel_size; kh++) {
                        for (size_t kw = 0; kw < kernel_size; kw++) {
                            long ih = (long)(oh * stride + kh) - (long)padding;
                            long iw = (long)(ow * stride + kw) - (long)padding;
                            
                            if (ih >= 0 && ih < (long)in_h && iw >= 0 && iw < (long)in_w) {
                                size_t in_idx = b * (channels * in_h * in_w) + 
                                              c * (in_h * in_w) + 
                                              ih * in_w + iw;
                                if (in_data[in_idx] > max_val) {
                                    max_val = in_data[in_idx];
                                }
                            }
                        }
                    }
                    
                    size_t out_idx = b * (channels * out_h * out_w) + 
                                   c * (out_h * out_w) + 
                                   oh * out_w + ow;
                    out_data[out_idx] = max_val;
                }
            }
        }
    }
    
    return output;
}

/* AvgPool2D */
nova_tensor_t* nova_pool2d_avg(nova_tensor_t* input, size_t kernel_size, size_t stride, size_t padding) {
    if (!input || input->ndim != 4) return NULL;
    
    size_t batch = input->shape[0];
    size_t channels = input->shape[1];
    size_t in_h = input->shape[2];
    size_t in_w = input->shape[3];
    
    size_t out_h = (in_h + 2 * padding - kernel_size) / stride + 1;
    size_t out_w = (in_w + 2 * padding - kernel_size) / stride + 1;
    
    size_t out_shape[4] = {batch, channels, out_h, out_w};
    nova_tensor_t* output = nova_tensor_zeros(out_shape, 4, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    // Perform average pooling
    for (size_t b = 0; b < batch; b++) {
        for (size_t c = 0; c < channels; c++) {
            for (size_t oh = 0; oh < out_h; oh++) {
                for (size_t ow = 0; ow < out_w; ow++) {
                    float sum = 0.0f;
                    size_t count = 0;
                    
                    // Sum values in pooling window
                    for (size_t kh = 0; kh < kernel_size; kh++) {
                        for (size_t kw = 0; kw < kernel_size; kw++) {
                            long ih = (long)(oh * stride + kh) - (long)padding;
                            long iw = (long)(ow * stride + kw) - (long)padding;
                            
                            if (ih >= 0 && ih < (long)in_h && iw >= 0 && iw < (long)in_w) {
                                size_t in_idx = b * (channels * in_h * in_w) + 
                                              c * (in_h * in_w) + 
                                              ih * in_w + iw;
                                sum += in_data[in_idx];
                                count++;
                            }
                        }
                    }
                    
                    size_t out_idx = b * (channels * out_h * out_w) + 
                                   c * (out_h * out_w) + 
                                   oh * out_w + ow;
                    out_data[out_idx] = count > 0 ? sum / count : 0.0f;
                }
            }
        }
    }
    
    return output;
}

/* AdaptiveMaxPool2D */
nova_tensor_t* nova_pool2d_adaptive_max(nova_tensor_t* input, size_t output_height, size_t output_width) {
    if (!input || input->ndim != 4) return NULL;
    
    size_t batch = input->shape[0];
    size_t channels = input->shape[1];
    size_t in_h = input->shape[2];
    size_t in_w = input->shape[3];
    
    size_t out_shape[4] = {batch, channels, output_height, output_width};
    nova_tensor_t* output = nova_tensor_zeros(out_shape, 4, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    // Adaptive pooling with dynamic kernel size
    for (size_t b = 0; b < batch; b++) {
        for (size_t c = 0; c < channels; c++) {
            for (size_t oh = 0; oh < output_height; oh++) {
                for (size_t ow = 0; ow < output_width; ow++) {
                    // Calculate input region for this output position
                    size_t start_h = (oh * in_h) / output_height;
                    size_t end_h = ((oh + 1) * in_h) / output_height;
                    size_t start_w = (ow * in_w) / output_width;
                    size_t end_w = ((ow + 1) * in_w) / output_width;
                    
                    float max_val = -FLT_MAX;
                    for (size_t ih = start_h; ih < end_h; ih++) {
                        for (size_t iw = start_w; iw < end_w; iw++) {
                            size_t in_idx = b * (channels * in_h * in_w) + 
                                          c * (in_h * in_w) + 
                                          ih * in_w + iw;
                            if (in_data[in_idx] > max_val) {
                                max_val = in_data[in_idx];
                            }
                        }
                    }
                    
                    size_t out_idx = b * (channels * output_height * output_width) + 
                                   c * (output_height * output_width) + 
                                   oh * output_width + ow;
                    out_data[out_idx] = max_val;
                }
            }
        }
    }
    
    return output;
}

/* AdaptiveAvgPool2D */
nova_tensor_t* nova_pool2d_adaptive_avg(nova_tensor_t* input, size_t output_height, size_t output_width) {
    if (!input || input->ndim != 4) return NULL;
    
    size_t batch = input->shape[0];
    size_t channels = input->shape[1];
    size_t in_h = input->shape[2];
    size_t in_w = input->shape[3];
    
    size_t out_shape[4] = {batch, channels, output_height, output_width};
    nova_tensor_t* output = nova_tensor_zeros(out_shape, 4, input->dtype);
    if (!output) return NULL;
    
    float* in_data = (float*)input->data;
    float* out_data = (float*)output->data;
    
    for (size_t b = 0; b < batch; b++) {
        for (size_t c = 0; c < channels; c++) {
            for (size_t oh = 0; oh < output_height; oh++) {
                for (size_t ow = 0; ow < output_width; ow++) {
                    size_t start_h = (oh * in_h) / output_height;
                    size_t end_h = ((oh + 1) * in_h) / output_height;
                    size_t start_w = (ow * in_w) / output_width;
                    size_t end_w = ((ow + 1) * in_w) / output_width;
                    
                    float sum = 0.0f;
                    size_t count = 0;
                    for (size_t ih = start_h; ih < end_h; ih++) {
                        for (size_t iw = start_w; iw < end_w; iw++) {
                            size_t in_idx = b * (channels * in_h * in_w) + 
                                          c * (in_h * in_w) + 
                                          ih * in_w + iw;
                            sum += in_data[in_idx];
                            count++;
                        }
                    }
                    
                    size_t out_idx = b * (channels * output_height * output_width) + 
                                   c * (output_height * output_width) + 
                                   oh * output_width + ow;
                    out_data[out_idx] = count > 0 ? sum / count : 0.0f;
                }
            }
        }
    }
    
    return output;
}

/* Global Average Pooling */
nova_tensor_t* nova_pool2d_global_avg(nova_tensor_t* input) {
    return nova_pool2d_adaptive_avg(input, 1, 1);
}
