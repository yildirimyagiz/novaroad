#!/bin/bash
set -u

# Setup Directories
mkdir -p build/bench_bin
BUILD_DIR="build/bench_bin"
FLAGS="-O3 -Wall -Wno-unused-parameter -fPIC -I."
LDFLAGS="-lm -lpthread -ldl"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   NOVA CPU CHARACTERIZATION SUITE                      ║"
echo "╚══════════════════════════════════════════════════════════╝"

# ==============================================================================
# 1. CPU COMPUTE SCALING (GFLOPS vs Matrix Size)
# ==============================================================================
echo -n "1️⃣  Running Compute Scaling (GFLOPS)... "

cat <<EOF > ${BUILD_DIR}/bench_cpu_scaling.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

// Link against our optimized backend
extern int64_t nova_cpu_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k);
extern void nova_cpu_backend_init(void);

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    nova_cpu_backend_init();
    
    int sizes[] = {64, 128, 256, 512, 1024, 2048};
    int num_sizes = 6;
    
    printf("\n   %-10s | %-10s | %-10s\n", "Size", "Time(s)", "GFLOPS");
    printf("   -----------+------------+-----------\n");
    
    for(int i=0; i<num_sizes; i++) {
        int N = sizes[i];
        size_t bytes = (size_t)N * N * sizeof(float);
        
        float *A = malloc(bytes);
        float *B = malloc(bytes);
        float *C = malloc(bytes);
        
        // Warmup
        nova_cpu_matmul(A, B, C, N, N, N);
        
        double start = get_time();
        int iterations = (N < 1024) ? 5 : 1; // More iters for small sizes
        for(int iter=0; iter<iterations; iter++) {
            nova_cpu_matmul(A, B, C, N, N, N);
        }
        double end = get_time();
        
        double total_time = end - start;
        double avg_time = total_time / iterations;
        double ops = 2.0 * N * N * N;
        double gflops = (ops * 1e-9) / avg_time;
        
        printf("   %-10d | %-10.4f | %-10.2f\n", N, avg_time, gflops);
        
        free(A); free(B); free(C);
    }
    return 0;
}
EOF

# Compile & Run
clang $FLAGS src/backends/cpu/nova_cpu_backend.c -c -o ${BUILD_DIR}/nova_cpu.o
clang $FLAGS ${BUILD_DIR}/bench_cpu_scaling.c ${BUILD_DIR}/nova_cpu.o -o ${BUILD_DIR}/bench_scaling $LDFLAGS
./${BUILD_DIR}/bench_scaling


# ==============================================================================
# 2. CPU CACHE BEHAVIOR (Latency vs Size)
# ==============================================================================
echo ""
echo -n "2️⃣  Running Cache Behavior Analysis... "

cat <<EOF > ${BUILD_DIR}/bench_cpu_cache.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

extern int64_t nova_cpu_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k);
extern void nova_cpu_backend_init(void);

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    nova_cpu_backend_init();
    
    // Mix of sizes likely to fit in L1, L2, and spill to DRAM
    // M1 assumption: L1 ~192KB, L2 ~12MB
    // Float matrix N*N*4 bytes. 
    // N=64 -> 16KB (L1)
    // N=256 -> 256KB (L2 region)
    // N=1024 -> 4MB (L2/SLC)
    // N=2048 -> 16MB (DRAM spill likely)
    int sizes[] = {64, 128, 256, 512, 1024, 2048, 3072};
    int num_sizes = 7;
    
    printf("\n   %-10s | %-15s | %-15s\n", "Size", "Time/Op (ns)", "Bandwidth Est.");
    printf("   -----------+-----------------+----------------\n");
    
    for(int i=0; i<num_sizes; i++) {
        int N = sizes[i];
        size_t bytes = (size_t)N * N * sizeof(float);
        
        float *A = malloc(bytes);
        float *B = malloc(bytes);
        float *C = malloc(bytes);
        if(!A) break; 
        
        // Warmup
        nova_cpu_matmul(A, B, C, N, N, N);
        
        double start = get_time();
        nova_cpu_matmul(A, B, C, N, N, N);
        double end = get_time();
        
        double time_sec = end - start;
        double ops = 2.0 * N * N * N;
        double time_per_op_ns = (time_sec / ops) * 1e9;
        
        // Simplified bandwidth metric (Total Data / Time)
        // Read A+B, Write C -> 3 * N*N floats
        double data_gb = (3.0 * bytes) / (1024.0*1024.0*1024.0);
        double bw_gbs = data_gb / time_sec;
        
        printf("   %-10d | %-15.4f | %-10.2f GB/s\n", N, time_per_op_ns, bw_gbs);
        
        free(A); free(B); free(C);
    }
    return 0;
}
EOF

clang $FLAGS ${BUILD_DIR}/bench_cpu_cache.c ${BUILD_DIR}/nova_cpu.o -o ${BUILD_DIR}/bench_cache $LDFLAGS
./${BUILD_DIR}/bench_cache


# ==============================================================================
# 3. CPU SIMD EFFICIENCY (Scalar vs Vectorized Backend)
# ==============================================================================
echo ""
echo -n "3️⃣  Running SIMD Efficiency Test... "

cat <<EOF > ${BUILD_DIR}/bench_cpu_simd.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

extern int64_t nova_cpu_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k);
extern void nova_cpu_backend_init(void);

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

// Naive Scalar Implementation (No SIMD, No Tiling)
void scalar_matmul(const float *A, const float *B, float *C, int N) {
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            float sum = 0.0f;
            for(int k=0; k<N; k++) {
                sum += A[i*N + k] * B[k*N + j];
            }
            C[i*N + j] = sum;
        }
    }
}

int main() {
    nova_cpu_backend_init();
    int N = 512; // Sufficient size to show SIMD gain
    size_t bytes = (size_t)N * N * sizeof(float);
    
    float *A = malloc(bytes);
    float *B = malloc(bytes);
    float *C1 = malloc(bytes);
    float *C2 = malloc(bytes);
    
    // 1. Measure SCALAR
    double start = get_time();
    scalar_matmul(A, B, C1, N);
    double t_scalar = get_time() - start;
    
    // Prevent Dead Code Elimination
    if (C1[0] == -123.0f) printf("Should not happen");
    
    // 2. Measure SIMD (Nova Backend)
    start = get_time();
    nova_cpu_matmul(A, B, C2, N, N, N);
    double t_simd = get_time() - start;
    
    double speedup = t_scalar / t_simd;
    double gflops_scalar = (2.0*N*N*N*1e-9) / t_scalar;
    double gflops_simd   = (2.0*N*N*N*1e-9) / t_simd;
    
    printf("\n   Mode     | Time(s) | GFLOPS\n");
    printf("   ---------+---------+-------\n");
    printf("   Scalar   | %-7.4f | %.2f\n", t_scalar, gflops_scalar);
    printf("   SIMD     | %-7.4f | %.2f\n", t_simd, gflops_simd);
    printf("\n   🚀 SIMD Speedup: %.2fx\n", speedup);
    
    free(A); free(B); free(C1); free(C2);
    return 0;
}
EOF

clang $FLAGS ${BUILD_DIR}/bench_cpu_simd.c ${BUILD_DIR}/nova_cpu.o -o ${BUILD_DIR}/bench_simd $LDFLAGS
./${BUILD_DIR}/bench_simd

echo ""
echo "✅ CPU Phase Complete."
