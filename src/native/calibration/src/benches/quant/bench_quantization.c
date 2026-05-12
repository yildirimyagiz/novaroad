/**
 * Quantization Benchmark
 * Measures performance of different quantization schemes
 */

#include "nova_autocal.h"
#include <stdio.h>
#include <stdlib.h>

#define TENSOR_SIZE (1024 * 1024)

typedef enum {
    QUANT_INT8,
    QUANT_INT4,
    QUANT_FP16,
    QUANT_BFLOAT16
} QuantType;

typedef struct {
    float *input;
    void *output;
    int size;
    QuantType type;
} QuantContext;

extern void quantize_int8(const float *input, int8_t *output, int size, 
                           float *scale, float *zero_point);
extern void quantize_int4(const float *input, uint8_t *output, int size,
                           float *scale, float *zero_point);
extern void quantize_fp16(const float *input, uint16_t *output, int size);
extern void quantize_bfloat16(const float *input, uint16_t *output, int size);

extern void dequantize_int8(const int8_t *input, float *output, int size,
                             float scale, float zero_point);
extern void dequantize_int4(const uint8_t *input, float *output, int size,
                             float scale, float zero_point);
extern void dequantize_fp16(const uint16_t *input, float *output, int size);
extern void dequantize_bfloat16(const uint16_t *input, float *output, int size);

static void setup_quant(void **ctx, QuantType type) {
    QuantContext *qc = (QuantContext *)malloc(sizeof(QuantContext));
    qc->size = TENSOR_SIZE;
    qc->type = type;
    qc->input = (float *)malloc(TENSOR_SIZE * sizeof(float));
    
    for (int i = 0; i < TENSOR_SIZE; i++) {
        qc->input[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    }
    
    switch (type) {
        case QUANT_INT8:
            qc->output = malloc(TENSOR_SIZE * sizeof(int8_t));
            break;
        case QUANT_INT4:
            qc->output = malloc((TENSOR_SIZE + 1) / 2);
            break;
        case QUANT_FP16:
        case QUANT_BFLOAT16:
            qc->output = malloc(TENSOR_SIZE * sizeof(uint16_t));
            break;
    }
    
    *ctx = qc;
}

static void run_quantization(void *ctx) {
    QuantContext *qc = (QuantContext *)ctx;
    float scale, zero_point;
    
    switch (qc->type) {
        case QUANT_INT8:
            quantize_int8(qc->input, qc->output, qc->size, &scale, &zero_point);
            break;
        case QUANT_INT4:
            quantize_int4(qc->input, qc->output, qc->size, &scale, &zero_point);
            break;
        case QUANT_FP16:
            quantize_fp16(qc->input, qc->output, qc->size);
            break;
        case QUANT_BFLOAT16:
            quantize_bfloat16(qc->input, qc->output, qc->size);
            break;
    }
}

static void teardown_quant(void *ctx) {
    QuantContext *qc = (QuantContext *)ctx;
    free(qc->input);
    free(qc->output);
    free(qc);
}

int main(void) {
    printf("=== Quantization Benchmark ===\n");
    printf("Tensor size: %d elements\n\n", TENSOR_SIZE);
    
    AutocalContext *ctx = autocal_create();
    
    const char *quant_names[] = {"INT8", "INT4", "FP16", "BFLOAT16"};
    QuantType quant_types[] = {QUANT_INT8, QUANT_INT4, QUANT_FP16, QUANT_BFLOAT16};
    int bits[] = {8, 4, 16, 16};
    
    for (int i = 0; i < 4; i++) {
        void *bench_ctx;
        setup_quant(&bench_ctx, quant_types[i]);
        
        double time_ms = autocal_measure_time(
            (AutocalWorkload){
                .name = quant_names[i],
                .execute = run_quantization,
                .context = bench_ctx,
                .iterations = 100
            }
        );
        
        double gb_per_sec = (TENSOR_SIZE * sizeof(float) / (1024.0 * 1024.0 * 1024.0)) / (time_ms / 1000.0);
        double compression_ratio = 32.0 / bits[i];
        
        printf("%10s: %.3f ms, %.2f GB/s, %.1fx compression\n",
               quant_names[i], time_ms, gb_per_sec, compression_ratio);
        
        teardown_quant(bench_ctx);
    }
    
    autocal_destroy(ctx);
    return 0;
}
