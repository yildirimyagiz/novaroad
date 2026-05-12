#!/bin/bash
set -u

# Setup
mkdir -p build/bin build/lib
CFLAGS="-O3 -Wall -Wno-unused-parameter -Wno-unused-function -fPIC -I."
# Add -fno-objc-arc to ensure Metal compiles correctly
METAL_CFLAGS="-x objective-c -fno-objc-arc" 
LDFLAGS="-lm -lpthread -ldl"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   NOVA PERFORMANCE BENCHMARK (ISOLATED MODE)           ║"
echo "╚══════════════════════════════════════════════════════════╝"

# ==============================================================================
# TEST 1: CPU BENCHMARK (The Silicon Melter)
# ==============================================================================
echo -n "1️⃣  CPU Backend (Silicon Melter)... "

cat <<EOF > build/bench_cpu.c
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
    int M=1024, N=1024, K=1024;
    size_t size = (size_t)M*N*sizeof(float);
    float *A = malloc((size_t)M*K*sizeof(float));
    float *B = malloc((size_t)K*N*sizeof(float));
    float *C = malloc(size);
    if(!A || !B || !C) return 1;

    // Heat up
    nova_cpu_matmul(A, B, C, 64, 64, 64);

    double start = get_time();
    nova_cpu_matmul(A, B, C, M, N, K);
    double end = get_time();
    
    double gflops = (2.0 * M * N * K) * 1e-9 / (end - start);
    printf("OK | %.2f GFLOPS (Time: %.4fs)\n", gflops, end - start);
    return 0;
}
EOF

clang $CFLAGS -c src/backends/cpu/nova_cpu_backend.c -o build/lib/nova_cpu_backend.o
clang $CFLAGS build/bench_cpu.c build/lib/nova_cpu_backend.o -o build/bin/bench_cpu $LDFLAGS

if ./build/bin/bench_cpu; then :; else echo "FAILED ❌"; fi


# ==============================================================================
# TEST 2: METAL BENCHMARK (The GPU Accelerator)
# ==============================================================================
echo -n "2️⃣  Metal Backend (GPU)...          "

if [[ "$OSTYPE" == "darwin"* ]]; then
    cat <<EOF > build/bench_metal.c
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
    if(nova_metal_init() != 1) { printf("SKIP (Init Failed)\n"); return 0; }
    
    int M=1024, N=1024, K=1024;
    size_t size = (size_t)M*N*sizeof(float);
    float *A = calloc(M*K, sizeof(float)); // calloc to zero init
    float *B = calloc(K*N, sizeof(float));
    float *C = malloc(size);
    if(!A || !B || !C) return 1;
    
    // Warmup
    nova_metal_matmul(A, B, C, 64, 64, 64);
    
    double start = get_time();
    nova_metal_matmul(A, B, C, M, N, K);
    double end = get_time();
    
    double gflops = (2.0 * M * N * K) * 1e-9 / (end - start);
    printf("OK | %.2f GFLOPS (Time: %.4fs)\n", gflops, end - start);
    return 0;
}
EOF

    # Compile Metal with Objective-C flags
    clang $CFLAGS $METAL_CFLAGS -c src/backends/metal/nova_metal_gpu.c -o build/lib/nova_metal_gpu.o
    clang $CFLAGS build/bench_metal.c build/lib/nova_metal_gpu.o -o build/bin/bench_metal $LDFLAGS -framework Metal -framework Foundation

    if ./build/bin/bench_metal; then :; else echo "CRASH 💥"; fi
else
    echo "SKIP (Not macOS)"
fi


# ==============================================================================
# TEST 3: LLVM JIT BENCHMARK (The Engine)
# ==============================================================================
echo -n "3️⃣  LLVM JIT (The Reality Engine)... "

LLVM_CONFIG="/opt/homebrew/opt/llvm/bin/llvm-config"
if [ ! -f "$LLVM_CONFIG" ]; then LLVM_CONFIG="llvm-config"; fi

if $LLVM_CONFIG --version >/dev/null 2>&1; then
    cat <<EOF > build/bench_llvm.c
#include <stdio.h>
#include <sys/time.h>
#include "src/backends/llvm/nova_llvm_backend.h"

double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    double start = get_time();
    NovaLLVMContext *ctx = nova_llvm_create_context("bench_module");
    if (!ctx) { printf("FAIL\n"); return 1; }
    
    // Create 'add' function logic
    LLVMTypeRef param_types[] = { LLVMInt64TypeInContext(ctx->context), LLVMInt64TypeInContext(ctx->context) };
    LLVMTypeRef ret_type = LLVMInt64TypeInContext(ctx->context);
    LLVMValueRef func = LLVMAddFunction(ctx->module, "add", LLVMFunctionType(ret_type, param_types, 2, 0));
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, func, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);
    LLVMValueRef sum = LLVMBuildAdd(ctx->builder, LLVMGetParam(func, 0), LLVMGetParam(func, 1), "sum");
    LLVMBuildRet(ctx->builder, sum);
    
    nova_llvm_verify_module(ctx);
    
    int64_t args[] = { 100, 200 };
    int64_t res = nova_llvm_run_function(ctx, "add", args, 2);
    
    double end = get_time();
    
    if (res == 300) {
        printf("OK | JIT Compile+Run Time: %.4fs\n", end - start);
    } else {
        printf("FAIL (Result: %lld)\n", (long long)res);
    }
    
    nova_llvm_destroy_context(ctx);
    return 0;
}
EOF

    LLVM_CFLAGS=$($LLVM_CONFIG --cflags)
    LLVM_LDFLAGS=$($LLVM_CONFIG --ldflags --libs all --system-libs)
    
    clang $CFLAGS -c src/backends/llvm/nova_llvm_backend.c -o build/lib/nova_llvm_backend.o $LLVM_CFLAGS
    clang $CFLAGS build/bench_llvm.c build/lib/nova_llvm_backend.o -o build/bin/bench_llvm $LDFLAGS $LLVM_LDFLAGS $LLVM_CFLAGS
    
    if ./build/bin/bench_llvm; then :; else echo "CRASH 💥"; fi
else
    echo "SKIP (llvm-config missing)"
fi
