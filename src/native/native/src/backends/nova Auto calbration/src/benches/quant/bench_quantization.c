/**
 * Quantization Benchmark for Auto-Calibration
 * Tests various quantization schemes and bit-widths
 */

#include "nova_autocal.h"
#include "nova_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

typedef enum {
    QUANT_INT8,
    QUANT_INT4,
    QUANT_FP16,
    QUANT_BF16,
} QuantType;

typedef struct {
    size_t size;
    QuantType type;
    bool symmetric;
} QuantConfig;

typedef struct {
    double quant_time_ms;
    double dequant_time_ms;
    double throughput_gb_s;
    double mse;  // Mean squared error
} QuantResult;

// FP32 to INT8 quantization
static void quantize_int8(const float *input, int8_t *output, size_t size,
                          float *scale, float *zero_point, bool symmetric) {
    // Find min/max
    float min_val = input[0], max_val = input[0];
    for (size_t i = 1; i < size; i++) {
        if (input[i] < min_val) min_val = input[i];
        if (input[i] > max_val) max_val = input[i];
    }
    
    // Calculate scale and zero point
    if (symmetric) {
        float abs_max = fmaxf(fabsf(min_val), fabsf(max_val));
        *scale = abs_max / 127.0f;
        *zero_point = 0.0f;
    } else {
        *scale = (max_val - min_val) / 255.0f;
        *zero_point = -min_val / *scale;
    }
    
    // Quantize
    for (size_t i = 0; i < size; i++) {
        float scaled = input[i] / *scale + *zero_point;
        int32_t clamped = (int32_t)roundf(scaled);
        if (clamped < -128) clamped = -128;
        if (clamped > 127) clamped = 127;
        output[i] = (int8_t)clamped;
    }
}

// INT8 to FP32 dequantization
static void dequantize_int8(const int8_t *input, float *output, size_t size,
                            float scale, float zero_point) {
    for (size_t i = 0; i < size; i++) {
        output[i] = (input[i] - zero_point) * scale;
    }
}

// FP32 to FP16 quantization (simplified)
static void quantize_fp16(const float *input, uint16_t *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // Simplified FP16 conversion (not IEEE compliant)
        output[i] = (uint16_t)(input[i] * 1000.0f);
    }
}

static void dequantize_fp16(const uint16_t *input, float *output, size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[i] = (float)input[i] / 1000.0f;
    }
}

static QuantResult bench_quantization(const QuantConfig *cfg, size_t iterations) {
    QuantResult result = {0};
    
    // Allocate buffers
    float *input = (float*)malloc(cfg->size * sizeof(float));
    float *output = (float*)malloc(cfg->size * sizeof(float));
    void *quantized = NULL;
    
    size_t quant_bytes = 0;
    switch (cfg->type) {
        case QUANT_INT8:
        case QUANT_INT4:
            quant_bytes = cfg->size;
            break;
        case QUANT_FP16:
        case QUANT_BF16:
            quant_bytes = cfg->size * 2;
            break;
    }
    quantized = malloc(quant_bytes);
    
    if (!input || !output || !quantized) {
        free(input); free(output); free(quantized);
        return result;
    }
    
    // Initialize input
    for (size_t i = 0; i < cfg->size; i++) {
        input[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
    
    nova_timer_t *timer = nova_timer_create();
    float scale, zero_point;
    
    // Benchmark quantization
    nova_timer_start(timer);
    for (size_t i = 0; i < iterations; i++) {
        if (cfg->type == QUANT_INT8) {
            quantize_int8(input, (int8_t*)quantized, cfg->size, &scale, &zero_point, cfg->symmetric);
        } else if (cfg->type == QUANT_FP16) {
            quantize_fp16(input, (uint16_t*)quantized, cfg->size);
        }
    }
    nova_timer_stop(timer);
    result.quant_time_ms = nova_timer_elapsed_ms(timer) / iterations;
    
    // Benchmark dequantization
    nova_timer_start(timer);
    for (size_t i = 0; i < iterations; i++) {
        if (cfg->type == QUANT_INT8) {
            dequantize_int8((int8_t*)quantized, output, cfg->size, scale, zero_point);
        } else if (cfg->type == QUANT_FP16) {
            dequantize_fp16((uint16_t*)quantized, output, cfg->size);
        }
    }
    nova_timer_stop(timer);
    result.dequant_time_ms = nova_timer_elapsed_ms(timer) / iterations;
    
    // Calculate throughput
    double total_time = (result.quant_time_ms + result.dequant_time_ms) / 1000.0;
    double bytes_processed = cfg->size * sizeof(float);
    result.throughput_gb_s = (bytes_processed / 1e9) / total_time;
    
    // Calculate MSE
    double mse_sum = 0.0;
    for (size_t i = 0; i < cfg->size; i++) {
        double diff = input[i] - output[i];
        mse_sum += diff * diff;
    }
    result.mse = mse_sum / cfg->size;
    
    nova_timer_destroy(timer);
    free(input); free(output); free(quantized);
    
    return result;
}

void nova_autocal_bench_quantization(nova_autocal_context_t *ctx) {
    printf("=== Quantization Auto-Calibration ===\n\n");
    
    QuantConfig configs[] = {
        // INT8 symmetric
        {.size = 1024 * 1024, .type = QUANT_INT8, .symmetric = true},
        // INT8 asymmetric
        {.size = 1024 * 1024, .type = QUANT_INT8, .symmetric = false},
        // FP16
        {.size = 1024 * 1024, .type = QUANT_FP16, .symmetric = true},
        // Large tensor
        {.size = 4 * 1024 * 1024, .type = QUANT_INT8, .symmetric = true},
    };
    
    const char *type_names[] = {"INT8", "INT4", "FP16", "BF16"};
    size_t num_configs = sizeof(configs) / sizeof(configs[0]);
    
    for (size_t i = 0; i < num_configs; i++) {
        QuantConfig *cfg = &configs[i];
        
        printf("Config %zu: Size=%zu, Type=%s, Symmetric=%s\n",
               i + 1, cfg->size, type_names[cfg->type],
               cfg->symmetric ? "Yes" : "No");
        
        QuantResult res = bench_quantization(cfg, 50);
        
        printf("  Quant time: %.3f ms\n", res.quant_time_ms);
        printf("  Dequant time: %.3f ms\n", res.dequant_time_ms);
        printf("  Throughput: %.2f GB/s\n", res.throughput_gb_s);
        printf("  MSE: %.6f\n\n", res.mse);
        
        char key[256];
        snprintf(key, sizeof(key), "quant_%s_%zu", type_names[cfg->type], cfg->size);
        nova_autocal_record_metric(ctx, key, "quant_time_ms", res.quant_time_ms);
        nova_autocal_record_metric(ctx, key, "throughput_gb_s", res.throughput_gb_s);
        nova_autocal_record_metric(ctx, key, "mse", res.mse);
    }
    
    printf("Quantization calibration complete.\n");
}
