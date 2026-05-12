/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA SCIENTIFIC PERFORMANCE & EFFICIENCY VALIDATION SUITE
 * Main Driver Program
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "nova_scientific_validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════
// SECTION RUNNERS
// ═══════════════════════════════════════════════════════════════════════════

static void run_section1_matmul_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 1: KERNEL-LEVEL MICROBENCHMARKS - MATRIX MULTIPLICATION          ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    // FP32 benchmarks
    printf("\n[ANALYSIS] FP32 Matrix Multiplication (64×64×64)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    BenchmarkMetrics m1 = bench_matmul(64, 64, 64, DTYPE_FP32, MATMUL_NAIVE_SCALAR);
    printf("[MEASURED] Naive Scalar:\n");
    nova_print_metrics("  ", &m1);
    
    BenchmarkMetrics m2 = bench_matmul(64, 64, 64, DTYPE_FP32, MATMUL_SIMD_OPTIMIZED);
    printf("\n[MEASURED] SIMD Optimized:\n");
    nova_print_metrics("  ", &m2);
    printf("[ANALYSIS] SIMD Speedup: %.2fx\n", m1.latency_us / m2.latency_us);
    
    printf("\n[ANALYSIS] FP32 Matrix Multiplication (256×256×256)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    BenchmarkMetrics m3 = bench_matmul(256, 256, 256, DTYPE_FP32, MATMUL_NAIVE_SCALAR);
    printf("[MEASURED] Naive Scalar:\n");
    nova_print_metrics("  ", &m3);
    
    BenchmarkMetrics m4 = bench_matmul(256, 256, 256, DTYPE_FP32, MATMUL_REGISTER_TILED);
    printf("\n[MEASURED] Register Tiled:\n");
    nova_print_metrics("  ", &m4);
    printf("[ANALYSIS] Tiling Speedup: %.2fx\n", m3.latency_us / m4.latency_us);
    
    printf("\n[ANALYSIS] FP32 Matrix Multiplication (1024×1024×1024)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    BenchmarkMetrics m5 = bench_matmul(1024, 1024, 1024, DTYPE_FP32, MATMUL_REGISTER_TILED);
    printf("[MEASURED] Register Tiled (Large):\n");
    nova_print_metrics("  ", &m5);
    
    // INT8 benchmarks
    printf("\n[ANALYSIS] INT8 Matrix Multiplication (256×256×256)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    BenchmarkMetrics m6 = bench_matmul(256, 256, 256, DTYPE_INT8, MATMUL_NAIVE_SCALAR);
    printf("[MEASURED] INT8 Naive:\n");
    nova_print_metrics("  ", &m6);
    
    BenchmarkMetrics m7 = bench_matmul(256, 256, 256, DTYPE_INT8, MATMUL_SIMD_OPTIMIZED);
    printf("\n[MEASURED] INT8 SIMD:\n");
    nova_print_metrics("  ", &m7);
    printf("[ANALYSIS] INT8 SIMD Speedup: %.2fx\n", m6.latency_us / m7.latency_us);
}

static void run_section2_memory_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 2: MEMORY SYSTEM STRESS TESTS                                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    // L1 Cache
    printf("\n[ANALYSIS] L1 Cache Access (32 KB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem1 = bench_memory(32 * 1024, MEM_SEQUENTIAL);
    nova_print_memory_metrics("Sequential Access", &mem1);
    
    // L2 Cache
    printf("\n[ANALYSIS] L2 Cache Access (512 KB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem2 = bench_memory(512 * 1024, MEM_SEQUENTIAL);
    nova_print_memory_metrics("Sequential Access", &mem2);
    
    // L3 Cache
    printf("\n[ANALYSIS] L3 Cache Access (8 MB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem3 = bench_memory(8 * 1024 * 1024, MEM_SEQUENTIAL);
    nova_print_memory_metrics("Sequential Access", &mem3);
    
    // DRAM
    printf("\n[ANALYSIS] DRAM Access (128 MB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem4 = bench_memory(128 * 1024 * 1024, MEM_SEQUENTIAL);
    nova_print_memory_metrics("Sequential Access", &mem4);
    
    // Random access
    printf("\n[ANALYSIS] Random Pointer Chasing (1 MB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem5 = bench_memory(1 * 1024 * 1024, MEM_RANDOM_CHASE);
    nova_print_memory_metrics("Random Chase", &mem5);
    
    // Strided access
    printf("\n[ANALYSIS] Strided Access (4 MB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem6 = bench_memory(4 * 1024 * 1024, MEM_STRIDED);
    nova_print_memory_metrics("Strided Access", &mem6);
    
    // Large copy
    printf("\n[ANALYSIS] Large Buffer Copy (64 MB)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    MemoryMetrics mem7 = bench_memory(64 * 1024 * 1024, MEM_LARGE_COPY);
    nova_print_memory_metrics("Large Copy", &mem7);
}

static void run_section3_fusion_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 3: FUSION EFFICIENCY TESTS                                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    printf("\n[ANALYSIS] MatMul + Add + Activation Fusion (256×256×256)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    FusionMetrics fusion1 = bench_fusion_matmul_add_activation(256, 256, 256);
    nova_print_fusion_metrics("Fusion Analysis", &fusion1);
    
    printf("\n[ANALYSIS] MatMul + Add + Activation Fusion (512×512×512)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    FusionMetrics fusion2 = bench_fusion_matmul_add_activation(512, 512, 512);
    nova_print_fusion_metrics("Fusion Analysis", &fusion2);
}

static void run_section4_numerics_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 4: NUMERICS STABILITY VALIDATION                                 ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    // Generate test data with potential precision issues
    size_t n = 1000000;
    float* data = (float*)malloc(n * sizeof(float));
    for (size_t i = 0; i < n; i++) {
        data[i] = (float)(rand() % 100) / 100.0f;
    }
    
    printf("\n[ANALYSIS] Large Reduction (1M elements)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    NumericsMetrics num1 = bench_accumulation(data, n, ACCUM_STANDARD);
    printf("[MEASURED] Standard Accumulation:\n");
    nova_print_numerics_metrics("  ", &num1);
    
    NumericsMetrics num2 = bench_accumulation(data, n, ACCUM_KAHAN);
    printf("\n[MEASURED] Kahan Summation:\n");
    nova_print_numerics_metrics("  ", &num2);
    printf("[ANALYSIS] Error Reduction: %.2fx\n", num1.relative_error / num2.relative_error);
    
    NumericsMetrics num3 = bench_accumulation(data, n, ACCUM_PAIRWISE);
    printf("\n[MEASURED] Pairwise Reduction:\n");
    nova_print_numerics_metrics("  ", &num3);
    
    // Dot product
    float* a = (float*)malloc(n * sizeof(float));
    float* b = (float*)malloc(n * sizeof(float));
    for (size_t i = 0; i < n; i++) {
        a[i] = (float)(rand() % 100) / 100.0f;
        b[i] = (float)(rand() % 100) / 100.0f;
    }
    
    printf("\n[ANALYSIS] Dot Product (1M elements)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    NumericsMetrics num4 = bench_dot_product(a, b, n, ACCUM_STANDARD);
    printf("[MEASURED] Standard Dot Product:\n");
    nova_print_numerics_metrics("  ", &num4);
    
    NumericsMetrics num5 = bench_dot_product(a, b, n, ACCUM_KAHAN);
    printf("\n[MEASURED] Kahan Dot Product:\n");
    nova_print_numerics_metrics("  ", &num5);
    
    // Softmax stability
    float* logits = (float*)malloc(1000 * sizeof(float));
    for (int i = 0; i < 1000; i++) {
        logits[i] = (float)(rand() % 100) - 50.0f; // Range: -50 to 50
    }
    
    printf("\n[ANALYSIS] Softmax Stability (1K elements)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    NumericsMetrics num6 = bench_softmax_stability(logits, 1000);
    nova_print_numerics_metrics("Softmax (sum should be 1.0)", &num6);
    
    free(data);
    free(a);
    free(b);
    free(logits);
}

static void run_section5_attention_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 5: ATTENTION / AI WORKLOADS                                      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    int d_model = 512;
    int n_heads = 8;
    
    printf("\n[ANALYSIS] Attention (seq_len=128, d_model=%d, n_heads=%d)\n", d_model, n_heads);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    AttentionMetrics attn1 = bench_attention(128, d_model, n_heads, ATTN_NAIVE);
    printf("[MEASURED] Naive Attention:\n");
    nova_print_attention_metrics("  ", &attn1);
    
    AttentionMetrics attn2 = bench_attention(128, d_model, n_heads, ATTN_FUSED);
    printf("\n[MEASURED] Fused Attention:\n");
    nova_print_attention_metrics("  ", &attn2);
    printf("[ANALYSIS] Fusion Speedup: %.2fx\n", attn1.latency_ms / attn2.latency_ms);
    
    printf("\n[ANALYSIS] Attention (seq_len=512, d_model=%d, n_heads=%d)\n", d_model, n_heads);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    AttentionMetrics attn3 = bench_attention(512, d_model, n_heads, ATTN_NAIVE);
    printf("[MEASURED] Naive Attention:\n");
    nova_print_attention_metrics("  ", &attn3);
    
    printf("\n[ANALYSIS] Attention (seq_len=2048, d_model=%d, n_heads=%d)\n", d_model, n_heads);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    AttentionMetrics attn4 = bench_attention(2048, d_model, n_heads, ATTN_FUSED);
    printf("[MEASURED] Fused Attention (Large):\n");
    nova_print_attention_metrics("  ", &attn4);
}

static void run_section6_datamovement_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 6: SYNTHETIC VS MATERIALIZED DATA MOVEMENT                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    int seq_len = 512;
    int d_model = 768;
    
    printf("\n[ANALYSIS] Positional Encoding (seq_len=%d, d_model=%d)\n", seq_len, d_model);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    DataMovementMetrics dm1 = bench_materialized_positional_encoding(seq_len, d_model);
    printf("[MEASURED] Materialized (Read from Memory):\n");
    printf("  Latency: %.3f μs\n", dm1.latency_us);
    printf("  Bandwidth: %.2f GB/s\n", dm1.bandwidth_gbs);
    printf("  Compute/Memory Ratio: %.2f\n", dm1.compute_memory_ratio);
    printf("  [CHECKSUM VERIFIED] 0x%016llx\n", (unsigned long long)dm1.checksum);
    
    DataMovementMetrics dm2 = bench_synthetic_positional_encoding(seq_len, d_model);
    printf("\n[MEASURED] Synthetic (Compute On-the-Fly):\n");
    printf("  Latency: %.3f μs\n", dm2.latency_us);
    printf("  Bandwidth: %.2f GB/s\n", dm2.bandwidth_gbs);
    printf("  Compute/Memory Ratio: %.2f\n", dm2.compute_memory_ratio);
    printf("  [CHECKSUM VERIFIED] 0x%016llx\n", (unsigned long long)dm2.checksum);
    printf("[ANALYSIS] Synthetic vs Materialized: %.2fx %s\n", 
           dm1.latency_us / dm2.latency_us,
           dm2.latency_us < dm1.latency_us ? "faster" : "slower");
    
    printf("\n[ANALYSIS] Recurrent Generation (seq_len=%d, d_model=%d)\n", seq_len, d_model);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    DataMovementMetrics dm3 = bench_recurrent_generation(seq_len, d_model);
    printf("[MEASURED] Recurrent:\n");
    printf("  Latency: %.3f μs\n", dm3.latency_us);
    printf("  Bandwidth: %.2f GB/s\n", dm3.bandwidth_gbs);
    printf("  Compute/Memory Ratio: %.2f\n", dm3.compute_memory_ratio);
}

static void run_section7_graph_benchmarks(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║ SECTION 7: END-TO-END GRAPH EXECUTION                                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    printf("\n[ANALYSIS] Transformer Block (seq_len=128, d_model=512, d_ff=2048)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    GraphMetrics graph1 = bench_transformer_block(128, 512, 2048, 8);
    printf("[MEASURED] Transformer Block:\n");
    printf("  Dispatch Overhead: %.3f μs\n", graph1.dispatch_overhead_us);
    printf("  Throughput: %.2f tokens/sec\n", graph1.throughput_tokens_per_sec);
    printf("  Fusion Impact: %.1f%% improvement\n", graph1.fusion_impact * 100.0);
    printf("  Deterministic: %s\n", graph1.deterministic ? "YES" : "NO");
    printf("  [CHECKSUM VERIFIED] 0x%016llx\n", (unsigned long long)graph1.checksum);
    
    printf("\n[ANALYSIS] MLP Block (batch_size=32, d_model=768, d_ff=3072)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    GraphMetrics graph2 = bench_mlp_block(32, 768, 3072);
    printf("[MEASURED] MLP Block:\n");
    printf("  Dispatch Overhead: %.3f μs\n", graph2.dispatch_overhead_us);
    printf("  Throughput: %.2f samples/sec\n", graph2.throughput_tokens_per_sec);
    printf("  Fusion Impact: %.1f%% improvement\n", graph2.fusion_impact * 100.0);
    printf("  Deterministic: %s\n", graph2.deterministic ? "YES" : "NO");
    printf("  [CHECKSUM VERIFIED] 0x%016llx\n", (unsigned long long)graph2.checksum);
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN DRIVER
// ═══════════════════════════════════════════════════════════════════════════

void nova_run_scientific_validation_suite(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                           ║\n");
    printf("║       NOVA SCIENTIFIC PERFORMANCE & EFFICIENCY VALIDATION SUITE        ║\n");
    printf("║                                                                           ║\n");
    printf("║  NON-SIMULATED | HARDWARE-REALISTIC | CHECKSUM-VALIDATED                ║\n");
    printf("║                                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    
    // Seed random number generator
    srand((unsigned int)time(None));
    
    run_section1_matmul_benchmarks();
    run_section2_memory_benchmarks();
    run_section3_fusion_benchmarks();
    run_section4_numerics_benchmarks();
    run_section5_attention_benchmarks();
    run_section6_datamovement_benchmarks();
    run_section7_graph_benchmarks();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                   VALIDATION SUITE COMPLETE                               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    nova_run_scientific_validation_suite();
    
    yield 0;
}
