#!/bin/bash
set -u

# Setup
mkdir -p build/bench_bin
BUILD_DIR="build/bench_bin"
FLAGS="-O3 -Wall -Wno-unused-parameter -fPIC -I."
LDFLAGS="-lm -lpthread -ldl -framework Metal -framework Foundation"
# Use -fno-objc-arc for Metal backend compilation
METAL_FLAGS="-x objective-c -fno-objc-arc"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   NOVA GPU (METAL) CHARACTERIZATION SUITE              ║"
echo "╚══════════════════════════════════════════════════════════╝"

# Compile Metal Backend Object
if [[ "$OSTYPE" == "darwin"* ]]; then
    clang $FLAGS $METAL_FLAGS src/backends/metal/nova_metal_gpu.c -c -o ${BUILD_DIR}/nova_metal.o
else
    echo "❌ Skipping Metal tests (Not macOS)"
    exit 0
fi

# ==============================================================================
# 1. GPU KERNEL THROUGHPUT (Scaling GFLOPS)
# ==============================================================================
echo -n "1️⃣  Running Kernel Throughput (Scaling)... "

cat <<EOF > ${BUILD_DIR}/bench_metal_throughput.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

extern int64_t nova_metal_init(void);
extern int64_t nova_metal_matmul(const float *a, const float *b, float *c, int64_t m, int64_t n, int64_t k);

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    if(nova_metal_init() != 1) return 1;
    
    int sizes[] = {128, 256, 512, 1024, 2048, 4096};
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
        nova_metal_matmul(A, B, C, 64, 64, 64);
        
        double start = get_time();
        nova_metal_matmul(A, B, C, N, N, N);
        double end = get_time();
        
        double time = end - start;
        double gflops = (2.0 * N * N * N * 1e-9) / time;
        
        printf("   %-10d | %-10.4f | %-10.2f\n", N, time, gflops);
        
        free(A); free(B); free(C);
    }
    return 0;
}
EOF

clang $FLAGS ${BUILD_DIR}/bench_metal_throughput.c ${BUILD_DIR}/nova_metal.o -o ${BUILD_DIR}/bench_metal_perf $LDFLAGS
./${BUILD_DIR}/bench_metal_perf


# ==============================================================================
# 2. GPU KERNEL LAUNCH OVERHEAD
# ==============================================================================
echo ""
echo -n "2️⃣  Measuring Kernel Launch Overhead... "

cat <<EOF > ${BUILD_DIR}/bench_metal_overhead.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

extern int64_t nova_metal_init(void);
extern int64_t nova_metal_add(const float *a, const float *b, float *c, int64_t n);

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    if(nova_metal_init() != 1) return 1;
    
    // Very small workload to isolate launch cost
    int N = 64; 
    size_t bytes = (size_t)N * sizeof(float);
    float *A = malloc(bytes);
    float *B = malloc(bytes);
    float *C = malloc(bytes);
    
    // Warmup
    nova_metal_add(A, B, C, N);
    
    int iterations = 1000;
    double start = get_time();
    for(int i=0; i<iterations; i++) {
        nova_metal_add(A, B, C, N);
    }
    double end = get_time();
    
    double avg_latency_us = ((end - start) / iterations) * 1e6;
    
    printf("\n   Iterations: %d\n", iterations);
    printf("   Total Time: %.4fs\n", end - start);
    printf("   🚀 Avg Launch Latency: %.2f us\n", avg_latency_us);
    
    free(A); free(B); free(C);
    return 0;
}
EOF

clang $FLAGS ${BUILD_DIR}/bench_metal_overhead.c ${BUILD_DIR}/nova_metal.o -o ${BUILD_DIR}/bench_metal_overhead $LDFLAGS
./${BUILD_DIR}/bench_metal_overhead


# ==============================================================================
# 3. GPU MEMORY BANDWIDTH (Simulated via Vector Add)
# ==============================================================================
echo ""
echo -n "3️⃣  Measuring GPU Memory Bandwidth... "

cat <<EOF > ${BUILD_DIR}/bench_metal_bandwidth.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>

extern int64_t nova_metal_init(void);
extern int64_t nova_metal_add(const float *a, const float *b, float *c, int64_t n);

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    if(nova_metal_init() != 1) return 1;
    
    // Large vector to saturate bandwidth
    int N = 64 * 1024 * 1024; // 64M floats = 256MB
    size_t bytes = (size_t)N * sizeof(float);
    
    float *A = malloc(bytes);
    float *B = malloc(bytes);
    float *C = malloc(bytes);
    if(!A || !B || !C) { printf("Malloc failed\n"); return 1; }
    
    // Fill 
    for(int i=0; i<1024; i++) A[i] = 1.0f; 
    
    // Warmup
    nova_metal_add(A, B, C, 1024);
    
    double start = get_time();
    nova_metal_add(A, B, C, N);
    double end = get_time();
    
    double time = end - start;
    // VADD = Read A + Read B + Write C = 3 * bytes
    double total_bytes = 3.0 * bytes;
    double gb = total_bytes / (1024.0*1024.0*1024.0);
    double bw = gb / time;
    
    printf("\n   Vector Size: %d M floats\n", N/1024/1024);
    printf("   Data Transfer: %.2f GB\n", gb);
    printf("   Time: %.4fs\n", time);
    printf("   🚀 Effective Bandwidth: %.2f GB/s\n", bw);
    
    free(A); free(B); free(C);
    return 0;
}
EOF

clang $FLAGS ${BUILD_DIR}/bench_metal_bandwidth.c ${BUILD_DIR}/nova_metal.o -o ${BUILD_DIR}/bench_metal_bw $LDFLAGS
./${BUILD_DIR}/bench_metal_bw

echo ""
echo "✅ GPU Phase Complete."
