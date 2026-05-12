#include "nova_kernels_advanced.h"
#include "ml/nova_tensor.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * NOVA KERNEL PERFORMANCE BENCHMARK
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define ITERATIONS 100

static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

typedef struct {
    double cpu_time;
    double simd_time;
    double cuda_time;
    double speedup_simd;
    double speedup_cuda;
} BenchmarkResult;

void print_result(const char *op_name, BenchmarkResult *result) {
    printf("\n📊 %s Benchmark:\n", op_name);
    printf("  CPU:   %.2f ms\n", result->cpu_time);
    
    if (result->simd_time > 0) {
        printf("  SIMD:  %.2f ms  [%.1fx faster]\n", 
               result->simd_time, result->speedup_simd);
    }
    
    if (result->cuda_time > 0) {
        printf("  CUDA:  %.2f ms  [%.1fx faster]\n", 
               result->cuda_time, result->speedup_cuda);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GELU Benchmark
// ═══════════════════════════════════════════════════════════════════════════

BenchmarkResult benchmark_gelu(size_t size) {
    BenchmarkResult result = {0};
    
    int64_t shape[] = {(int64_t)size};
    NovaTensor *x = nova_tensor_randn(NULL, shape, 1);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    // CPU Baseline
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        nova_kernel_gelu(x, out);
    }
    result.cpu_time = (get_time_ms() - start) / ITERATIONS;
    
    // SIMD
#ifdef __AVX2__
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        nova_simd_gelu(x, out);
    }
    result.simd_time = (get_time_ms() - start) / ITERATIONS;
    result.speedup_simd = result.cpu_time / result.simd_time;
#endif
    
    // CUDA
#ifdef USE_CUDA
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        nova_cuda_gelu(x, out);
    }
    result.cuda_time = (get_time_ms() - start) / ITERATIONS;
    result.speedup_cuda = result.cpu_time / result.cuda_time;
#endif
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// SiLU Benchmark
// ═══════════════════════════════════════════════════════════════════════════

BenchmarkResult benchmark_silu(size_t size) {
    BenchmarkResult result = {0};
    
    int64_t shape[] = {(int64_t)size};
    NovaTensor *x = nova_tensor_randn(NULL, shape, 1);
    NovaTensor *out = nova_tensor_create(NULL, shape, 1, NOVA_DTYPE_FP32);
    
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        nova_kernel_silu(x, out);
    }
    result.cpu_time = (get_time_ms() - start) / ITERATIONS;
    
#ifdef __AVX2__
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        nova_simd_silu(x, out);
    }
    result.simd_time = (get_time_ms() - start) / ITERATIONS;
    result.speedup_simd = result.cpu_time / result.simd_time;
#endif
    
#ifdef USE_CUDA
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        nova_cuda_silu(x, out);
    }
    result.cuda_time = (get_time_ms() - start) / ITERATIONS;
    result.speedup_cuda = result.cpu_time / result.cuda_time;
#endif
    
    nova_tensor_destroy(x);
    nova_tensor_destroy(out);
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// MatMul Benchmark
// ═══════════════════════════════════════════════════════════════════════════

BenchmarkResult benchmark_matmul(int M, int N, int K) {
    BenchmarkResult result = {0};
    
    int64_t shape_a[] = {M, K};
    int64_t shape_b[] = {K, N};
    int64_t shape_c[] = {M, N};
    
    NovaTensor *A = nova_tensor_randn(NULL, shape_a, 2);
    NovaTensor *B = nova_tensor_randn(NULL, shape_b, 2);
    NovaTensor *C = nova_tensor_create(NULL, shape_c, 2, NOVA_DTYPE_FP32);
    
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS / 10; i++) {  // Fewer iterations for matmul
        nova_kernel_matmul(A, B, C);
    }
    result.cpu_time = (get_time_ms() - start) / (ITERATIONS / 10);
    
#ifdef USE_CUDA
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS / 10; i++) {
        nova_cuda_matmul(A, B, C);
    }
    result.cuda_time = (get_time_ms() - start) / (ITERATIONS / 10);
    result.speedup_cuda = result.cpu_time / result.cuda_time;
#endif
    
    nova_tensor_destroy(A);
    nova_tensor_destroy(B);
    nova_tensor_destroy(C);
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Fused Operations Benchmark
// ═══════════════════════════════════════════════════════════════════════════

void benchmark_fused_operations(int M, int N, int K) {
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║         FUSED vs SEPARATE OPERATIONS BENCHMARK               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    
    int64_t shape_a[] = {M, K};
    int64_t shape_b[] = {K, N};
    int64_t shape_c[] = {M, N};
    
    NovaTensor *A = nova_tensor_randn(NULL, shape_a, 2);
    NovaTensor *B = nova_tensor_randn(NULL, shape_b, 2);
    NovaTensor *C = nova_tensor_create(NULL, shape_c, 2, NOVA_DTYPE_FP32);
    NovaTensor *temp = nova_tensor_create(NULL, shape_c, 2, NOVA_DTYPE_FP32);
    
    // Separate: MatMul + GELU
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS / 10; i++) {
        nova_kernel_matmul(A, B, temp);
        nova_kernel_gelu(temp, C);
    }
    double separate_time = (get_time_ms() - start) / (ITERATIONS / 10);
    
    // Fused: MatMul+GELU
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS / 10; i++) {
        fused_matmul_gelu(A, B, C);
    }
    double fused_time = (get_time_ms() - start) / (ITERATIONS / 10);
    
    printf("\nMatMul + GELU (%dx%d):\n", M, N);
    printf("  Separate: %.2f ms\n", separate_time);
    printf("  Fused:    %.2f ms  [%.1fx faster]\n", 
           fused_time, separate_time / fused_time);
    
    nova_tensor_destroy(A);
    nova_tensor_destroy(B);
    nova_tensor_destroy(C);
    nova_tensor_destroy(temp);
}

// ═══════════════════════════════════════════════════════════════════════════
// Main Benchmark Suite
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║        NOVA KERNEL PERFORMANCE BENCHMARK SUITE             ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    
    nova_dispatcher_init();
    
    printf("\n🔬 Running benchmarks with %d iterations...\n", ITERATIONS);
    
    // Element-wise operations
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  ELEMENT-WISE OPERATIONS (1M elements)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    BenchmarkResult gelu_result = benchmark_gelu(1000000);
    print_result("GELU", &gelu_result);
    
    BenchmarkResult silu_result = benchmark_silu(1000000);
    print_result("SiLU", &silu_result);
    
    // Matrix multiplication
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  MATRIX MULTIPLICATION\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    BenchmarkResult matmul_result = benchmark_matmul(512, 512, 512);
    print_result("MatMul (512x512)", &matmul_result);
    
    // Fused operations
    benchmark_fused_operations(512, 512, 512);
    
    // Summary
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                      SUMMARY                                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    
    printf("\nAverage Speedups:\n");
    if (gelu_result.speedup_simd > 0) {
        printf("  SIMD:  %.1fx (over CPU)\n", 
               (gelu_result.speedup_simd + silu_result.speedup_simd) / 2.0);
    }
    if (gelu_result.speedup_cuda > 0) {
        printf("  CUDA:  %.1fx (over CPU)\n", 
               (gelu_result.speedup_cuda + silu_result.speedup_cuda) / 2.0);
    }
    
    printf("\n✅ Benchmark complete!\n\n");
    
    return 0;
}
