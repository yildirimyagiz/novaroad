/**
 * @file quantization.c
 * @brief Tensor quantization for efficient inference
 */

#include "ai/tensor.h"
#include "std/alloc.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/* Quantize FP32 tensor to INT8 */
void nova_tensor_quantize_int8(nova_tensor_t* input, int8_t* output, float* scale, float* zero_point) {
    if (!input || !output || !scale || !zero_point) return;
    
    float* data = (float*)input->data;
    size_t total_elements = 1;
    for (size_t i = 0; i < input->ndim; i++) {
        total_elements *= input->shape[i];
    }
    
    // Find min and max
    float min_val = data[0];
    float max_val = data[0];
    for (size_t i = 1; i < total_elements; i++) {
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }
    
    // Calculate scale and zero point
    *scale = (max_val - min_val) / 255.0f;
    *zero_point = -min_val / (*scale);
    
    // Quantize
    for (size_t i = 0; i < total_elements; i++) {
        float val = data[i] / (*scale) + (*zero_point);
        val = fmaxf(-128.0f, fminf(127.0f, roundf(val)));
        output[i] = (int8_t)val;
    }
}

/* Dequantize INT8 tensor to FP32 */
void nova_tensor_dequantize_int8(int8_t* input, nova_tensor_t* output, float scale, float zero_point) {
    if (!input || !output) return;
    
    float* data = (float*)output->data;
    size_t total_elements = 1;
    for (size_t i = 0; i < output->ndim; i++) {
        total_elements *= output->shape[i];
    }
    
    for (size_t i = 0; i < total_elements; i++) {
        data[i] = (input[i] - zero_point) * scale;
    }
}
