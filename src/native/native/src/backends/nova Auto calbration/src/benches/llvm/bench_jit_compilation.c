/**
 * JIT Compilation Benchmark for Auto-Calibration
 * Tests compilation speed and optimization levels
 */

#include "nova_autocal.h"
#include "nova_autocal_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    OPT_NONE,
    OPT_O1,
    OPT_O2,
    OPT_O3,
} OptLevel;

typedef struct {
    size_t function_count;
    size_t avg_function_size;  // Instructions
    OptLevel opt_level;
} JITConfig;

typedef struct {
    double compile_time_ms;
    double functions_per_sec;
    size_t compiled_size_bytes;
} JITResult;

// Mock IR generation
static char* generate_mock_ir(size_t instruction_count) {
    size_t buffer_size = instruction_count * 50;  // ~50 chars per instruction
    char *ir = (char*)malloc(buffer_size);
    if (!ir) return NULL;
    
    size_t offset = 0;
    for (size_t i = 0; i < instruction_count; i++) {
        offset += snprintf(ir + offset, buffer_size - offset,
                          "  %%v%zu = add i32 %%v%zu, %zu\n", 
                          i, (i > 0) ? i - 1 : 0, i);
    }
    
    return ir;
}

// Mock JIT compilation
static void* jit_compile_function(const char *ir, OptLevel opt, size_t *out_size) {
    size_t ir_len = strlen(ir);
    
    // Simulate compilation time based on opt level
    volatile size_t dummy = 0;
    size_t iterations = ir_len * (1 << opt);  // More iterations for higher opt
    for (size_t i = 0; i < iterations; i++) {
        dummy += i;
    }
    
    // Mock compiled code
    *out_size = ir_len / 2;  // Assume 50% compression
    return malloc(*out_size);
}

static JITResult bench_jit_compilation(const JITConfig *cfg, size_t iterations) {
    JITResult result = {0};
    
    // Generate IR for all functions
    char **irs = (char**)malloc(cfg->function_count * sizeof(char*));
    if (!irs) return result;
    
    for (size_t i = 0; i < cfg->function_count; i++) {
        irs[i] = generate_mock_ir(cfg->avg_function_size);
        if (!irs[i]) {
            for (size_t j = 0; j < i; j++) free(irs[j]);
            free(irs);
            return result;
        }
    }
    
    // Benchmark compilation
    nova_timer_t *timer = nova_timer_create();
    nova_timer_start(timer);
    
    size_t total_compiled_size = 0;
    
    for (size_t iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < cfg->function_count; i++) {
            size_t compiled_size;
            void *code = jit_compile_function(irs[i], cfg->opt_level, &compiled_size);
            total_compiled_size += compiled_size;
            free(code);
        }
    }
    
    nova_timer_stop(timer);
    double total_time = nova_timer_elapsed_ms(timer);
    
    result.compile_time_ms = total_time / (cfg->function_count * iterations);
    result.functions_per_sec = (cfg->function_count * iterations * 1000.0) / total_time;
    result.compiled_size_bytes = total_compiled_size / iterations;
    
    nova_timer_destroy(timer);
    
    // Cleanup
    for (size_t i = 0; i < cfg->function_count; i++) {
        free(irs[i]);
    }
    free(irs);
    
    return result;
}

void nova_autocal_bench_jit_compilation(nova_autocal_context_t *ctx) {
    printf("=== JIT Compilation Auto-Calibration ===\n\n");
    
    JITConfig configs[] = {
        // Small functions, no optimization
        {.function_count = 100, .avg_function_size = 10, .opt_level = OPT_NONE},
        // Small functions, O2
        {.function_count = 100, .avg_function_size = 10, .opt_level = OPT_O2},
        // Medium functions, O2
        {.function_count = 50, .avg_function_size = 100, .opt_level = OPT_O2},
        // Large functions, O3
        {.function_count = 10, .avg_function_size = 1000, .opt_level = OPT_O3},
    };
    
    const char *opt_names[] = {"None", "O1", "O2", "O3"};
    size_t num_configs = sizeof(configs) / sizeof(configs[0]);
    
    for (size_t i = 0; i < num_configs; i++) {
        JITConfig *cfg = &configs[i];
        
        printf("Config %zu: Functions=%zu, Size=%zu, Opt=%s\n",
               i + 1, cfg->function_count, cfg->avg_function_size,
               opt_names[cfg->opt_level]);
        
        JITResult res = bench_jit_compilation(cfg, 5);
        
        printf("  Compile time: %.3f ms/function\n", res.compile_time_ms);
        printf("  Throughput: %.2f functions/sec\n", res.functions_per_sec);
        printf("  Compiled size: %zu bytes\n\n", res.compiled_size_bytes);
        
        char key[256];
        snprintf(key, sizeof(key), "jit_f%zu_s%zu_o%d",
                 cfg->function_count, cfg->avg_function_size, cfg->opt_level);
        nova_autocal_record_metric(ctx, key, "compile_time_ms", res.compile_time_ms);
        nova_autocal_record_metric(ctx, key, "functions_per_sec", res.functions_per_sec);
    }
    
    printf("JIT compilation calibration complete.\n");
}
