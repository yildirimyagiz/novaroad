/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA SCIENTIFIC PERFORMANCE & EFFICIENCY VALIDATION SUITE
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * NON-SIMULATED HARDWARE-REALISTIC PERFORMANCE VALIDATION
 * 
 * Requirements:
 * • NO synthetic timing shortcuts
 * • NO mocked latency values
 * • ALL workloads execute real computation
 * • ALL benchmarks include checksum validation
 * • ALL measurements prevent dead-code elimination
 * • High-resolution timers (mach_absolute_time / clock_gettime)
 * • Warmup phases mandatory
 * • Multiple iteration averaging mandatory
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef NOVA_SCIENTIFIC_VALIDATION_H
#define NOVA_SCIENTIFIC_VALIDATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════
// TIMING INFRASTRUCTURE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t elapsed_ns;
} NovaTimer;

// High-resolution timer (nanoseconds)
uint64_t nova_now_ns(void);

// Start/stop timer
void nova_timer_start(NovaTimer* timer);
void nova_timer_stop(NovaTimer* timer);
double nova_timer_elapsed_us(const NovaTimer* timer);
double nova_timer_elapsed_ms(const NovaTimer* timer);

// ═══════════════════════════════════════════════════════════════════════════
// BENCHMARK RESULT STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    double latency_us;          // Latency in microseconds
    double latency_ms;          // Latency in milliseconds
    double throughput_gflops;   // Effective GFLOPS
    double throughput_tops;     // Effective TOPS (for INT8)
    double bandwidth_gbs;       // Effective bandwidth GB/s
    uint64_t checksum;          // Result checksum
    uint32_t iterations;        // Number of iterations
    bool validated;             // Checksum validated
    bool anomaly_detected;      // Anomaly flag
    char anomaly_msg[256];      // Anomaly description
} BenchmarkMetrics;

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 1: KERNEL-LEVEL MICROBENCHMARKS
// ═══════════════════════════════════════════════════════════════════════════

// Matrix multiplication variants
typedef enum {
    MATMUL_NAIVE_SCALAR,
    MATMUL_SIMD_OPTIMIZED,
    MATMUL_REGISTER_TILED,
    MATMUL_FUSED
} MatMulVariant;

typedef enum {
    DTYPE_FP32,
    DTYPE_FP16,
    DTYPE_INT8
} DataType;

// MatMul benchmark
BenchmarkMetrics bench_matmul(int M, int N, int K, DataType dtype, MatMulVariant variant);

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 2: MEMORY SYSTEM STRESS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    MEM_SEQUENTIAL,
    MEM_RANDOM_CHASE,
    MEM_STRIDED,
    MEM_LARGE_COPY
} MemoryPattern;

typedef struct {
    double latency_ns_per_op;
    double bandwidth_gbs;
    double cache_hit_rate;
    char cache_level[32];  // L1/L2/L3/DRAM
    uint64_t checksum;
    bool validated;
} MemoryMetrics;

MemoryMetrics bench_memory(size_t size, MemoryPattern pattern);

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 3: FUSION EFFICIENCY TESTS
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    BenchmarkMetrics unfused;
    BenchmarkMetrics fused;
    double latency_reduction;
    double memory_traffic_reduction;
    double speedup;
} FusionMetrics;

FusionMetrics bench_fusion_matmul_add_activation(int M, int N, int K);

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 4: NUMERICS STABILITY VALIDATION
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    ACCUM_STANDARD,
    ACCUM_KAHAN,
    ACCUM_PAIRWISE,
    ACCUM_MIXED_PRECISION
} AccumulationMethod;

typedef struct {
    double result;
    double ulp_drift;
    double relative_error;
    double latency_us;
    bool deterministic;
    uint64_t checksum;
} NumericsMetrics;

NumericsMetrics bench_accumulation(const float* data, size_t n, AccumulationMethod method);
NumericsMetrics bench_dot_product(const float* a, const float* b, size_t n, AccumulationMethod method);
NumericsMetrics bench_softmax_stability(const float* logits, size_t n);

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 5: ATTENTION / AI WORKLOADS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    ATTN_NAIVE,
    ATTN_IO_AWARE_TILED,
    ATTN_FUSED
} AttentionVariant;

typedef struct {
    double latency_ms;
    double effective_flops;
    double memory_traffic_gb;
    double numerical_drift;
    uint64_t checksum;
    bool validated;
} AttentionMetrics;

AttentionMetrics bench_attention(int seq_len, int d_model, int n_heads, AttentionVariant variant);

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 6: SYNTHETIC VS MATERIALIZED DATA MOVEMENT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    double latency_us;
    double bandwidth_gbs;
    double compute_memory_ratio;
    uint64_t checksum;
} DataMovementMetrics;

DataMovementMetrics bench_materialized_positional_encoding(int seq_len, int d_model);
DataMovementMetrics bench_synthetic_positional_encoding(int seq_len, int d_model);
DataMovementMetrics bench_recurrent_generation(int seq_len, int d_model);

// ═══════════════════════════════════════════════════════════════════════════
// SECTION 7: END-TO-END GRAPH EXECUTION
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    double dispatch_overhead_us;
    double fusion_impact;
    double throughput_tokens_per_sec;
    bool deterministic;
    uint64_t checksum;
} GraphMetrics;

GraphMetrics bench_transformer_block(int seq_len, int d_model, int d_ff, int n_heads);
GraphMetrics bench_mlp_block(int batch_size, int d_model, int d_ff);

// ═══════════════════════════════════════════════════════════════════════════
// UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

// Checksum computation (prevents dead code elimination)
uint64_t nova_checksum_fp32(const float* data, size_t n);
uint64_t nova_checksum_fp16(const uint16_t* data, size_t n);
uint64_t nova_checksum_int8(const int8_t* data, size_t n);

// Anomaly detection
bool nova_detect_anomaly_bandwidth(double measured_gbs, double theoretical_max_gbs);
bool nova_detect_anomaly_latency(double measured_us);
bool nova_detect_dead_code_elimination(uint64_t checksum);

// Warmup and averaging
void nova_warmup(void (*kernel)(void*), void* args, int iterations);
double nova_average_latency(void (*kernel)(void*), void* args, int iterations, uint64_t* checksum_out);

// Print utilities
void nova_print_metrics(const char* name, const BenchmarkMetrics* metrics);
void nova_print_memory_metrics(const char* name, const MemoryMetrics* metrics);
void nova_print_fusion_metrics(const char* name, const FusionMetrics* metrics);
void nova_print_numerics_metrics(const char* name, const NumericsMetrics* metrics);
void nova_print_attention_metrics(const char* name, const AttentionMetrics* metrics);

// Run complete validation suite
void nova_run_scientific_validation_suite(void);

#endif // NOVA_SCIENTIFIC_VALIDATION_H
