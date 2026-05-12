#!/bin/bash
set -e

# Setup Directories
mkdir -p build/lib build/bin

# Flags
CFLAGS="-O3 -Wall -Wno-unused-parameter -Wno-unused-function -fPIC"
LDFLAGS="-lm -lpthread -ldl"

echo "🚀 Building Nova Native Backends..."

# 1. Compile Dispatcher & Core
echo "Compiling dispatcher..."
clang $CFLAGS -c src/backends/nova_backend_dispatch.c -o build/lib/nova_backend_dispatch.o

# 2. Compile CPU Backend (HyperFlash + Tiled)
echo "Compiling CPU backend..."
# Use platform specific SIMD flags if needed (-mavx2 etc), but let clang auto-detect (-march=native)
clang $CFLAGS -march=native -c src/backends/cpu/nova_cpu_backend.c -o build/lib/nova_cpu_backend.o

# 3. Compile Metal Backend (macOS only)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Compiling Metal backend..."
    # Force Objective-C mode for .c file containing ObjC code
    # Using -fno-objc-arc avoids complex bridging casts for C interop
    clang $CFLAGS -x objective-c -fno-objc-arc -c src/backends/metal/nova_metal_gpu.c -o build/lib/nova_metal_gpu.o
    LDFLAGS="$LDFLAGS -framework Metal -framework Foundation"
fi

# 4. Compile Other GPU Backends (Stubs or Impl)
echo "Compiling GPU backends..."
clang $CFLAGS -c src/backends/cuda/nova_cuda.c -o build/lib/nova_cuda.o
clang $CFLAGS -c src/backends/opencl/nova_opencl.c -o build/lib/nova_opencl.o
clang $CFLAGS -c src/backends/rocm/nova_rocm.c -o build/lib/nova_rocm.o
clang $CFLAGS -c src/backends/vulkan/nova_vulkan.c -o build/lib/nova_vulkan.o

# 5. Compile Cross-Compiler & Mobile CodeGen
echo "Compiling Cross Tools..."
clang $CFLAGS -c src/backends/cross/nova_cross_compiler.c -o build/lib/nova_cross_compiler.o
clang $CFLAGS -c src/backends/mobile/nova_mobile_codegen.c -o build/lib/nova_mobile_codegen.o

# 6. Compile LLVM Backend
LLVM_CONFIG="/opt/homebrew/opt/llvm/bin/llvm-config"
if [ ! -f "$LLVM_CONFIG" ]; then
    LLVM_CONFIG="llvm-config" # Fallback to PATH
fi

HAS_LLVM=0
if $LLVM_CONFIG --version >/dev/null 2>&1; then
    echo "Compiling LLVM Backend..."
    LLVM_CFLAGS=$($LLVM_CONFIG --cflags)
    # Link against ALL LLVM libs to ensure JIT/Target support
    LLVM_LDFLAGS=$($LLVM_CONFIG --ldflags --libs all --system-libs)
    
    clang $CFLAGS -c src/backends/llvm/nova_llvm_backend.c -o build/lib/nova_llvm_backend.o $LLVM_CFLAGS
    HAS_LLVM=1
else
    echo "⚠️  llvm-config not found. Skipping LLVM Backend compilation."
fi

# 7. Create Test Source
echo "Creating test source..."
cat <<EOF > build/test_main.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include "src/backends/nova_backend_dispatch.h"

// Conditional include based on build flag (passed via -D)
#ifdef HAS_LLVM
#include "src/backends/llvm/nova_llvm_backend.h"
#endif

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    printf("🧪 Nova Native Backend Performance Test\n");
    nova_backend_init(NOVA_BACKEND_AUTO);
    
    // --- LLVM JIT Test ---
#ifdef HAS_LLVM
    printf("\n>>> Running LLVM JIT Test <<<\n");
    NovaLLVMContext *ctx = nova_llvm_create_context("nova_jit_module");
    if (ctx) {
        // Simple function: int add(int a, int b) { return a + b; }
        LLVMTypeRef param_types[] = { LLVMInt64TypeInContext(ctx->context), LLVMInt64TypeInContext(ctx->context) };
        LLVMTypeRef ret_type = LLVMInt64TypeInContext(ctx->context);
        LLVMValueRef func = LLVMAddFunction(ctx->module, "add", LLVMFunctionType(ret_type, param_types, 2, 0));
        LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, func, "entry");
        LLVMPositionBuilderAtEnd(ctx->builder, entry);
        LLVMValueRef sum = LLVMBuildAdd(ctx->builder, LLVMGetParam(func, 0), LLVMGetParam(func, 1), "sum");
        LLVMBuildRet(ctx->builder, sum);
        
        nova_llvm_verify_module(ctx);
        // Optimization skipped for compatibility
        
        int64_t args[] = { 10, 32 };
        int64_t result = nova_llvm_run_function(ctx, "add", args, 2);
        printf("   JIT Result: 10 + 32 = %lld %s\n", (long long)result, (result == 42) ? "✅" : "❌");
        
        nova_llvm_destroy_context(ctx);
    } else {
        printf("   LLVM Context creation failed.\n");
    }
#endif
    
    // --- MatMul Benchmark ---
    int M=1024, N=1024, K=1024;
    printf("\n>>> Running MatMul Benchmark [%dx%dx%d]...\n", M, N, K);
    
    size_t size = (size_t)M*N*sizeof(float);
    float *A = malloc((size_t)M*K*sizeof(float));
    float *B = malloc((size_t)K*N*sizeof(float));
    float *C = malloc(size);
    
    if(!A || !B || !C) { fprintf(stderr, "Malloc failed for MatMul\n"); return 1; }
    
    for(int i=0; i<M*K; i++) A[i] = ((float)rand()/RAND_MAX);
    for(int i=0; i<K*N; i++) B[i] = ((float)rand()/RAND_MAX);
    
    double start = get_time();
    int64_t rc = nova_dispatch_matmul(A, B, C, M, N, K);
    double end = get_time();
    
    if (rc < 0) {
        printf("   ⚠️ MatMul skipped (Backend returned %lld)\n", (long long)rc);
    } else {
        double elapsed = end - start;
        double gflops = (2.0 * M * N * K) * 1e-9 / elapsed;
        printf("   Time: %.4f s\n", elapsed);
        printf("   Performance: %.2f GFLOPS 🚀\n", gflops);
    }
    
    free(A); free(B); free(C);
    
    // --- HyperFlash Attention Benchmark ---
    int L=1024, D=64;
    printf("\n>>> Running HyperFlash Attention Benchmark [L=%d, D=%d]...\n", L, D);
    
    float *Q = malloc((size_t)L*D*sizeof(float));
    float *K_mem = malloc((size_t)L*D*sizeof(float));
    float *V = malloc((size_t)L*D*sizeof(float));
    float *Out = malloc((size_t)L*D*sizeof(float));

    if(!Q || !K_mem || !V || !Out) { fprintf(stderr, "Malloc failed for Attention\n"); return 1; }
    
    for(int i=0; i<L*D; i++) { Q[i]=0.1f; K_mem[i]=0.1f; V[i]=1.0f; }
    
    start = get_time();
    rc = nova_dispatch_flash_attention(Q, K_mem, V, Out, L, D);
    end = get_time();
    
    if (rc < 0) {
        printf("   ⚠️ Flash Attention skipped (Backend returned %lld)\n", (long long)rc);
    } else {
        double elapsed = end - start;
        printf("   Time: %.4f s\n", elapsed);
        printf("   Throughput: %.2f M tokens/s (approx)\n", (double)L / elapsed / 1e6);
    }

    free(Q); free(K_mem); free(V); free(Out);
    
    nova_backend_cleanup();
    return 0;
}
EOF

# 8. Compile Final Test Binary (Static Link)
echo "Linking test binary..."
SOURCES="src/backends/nova_backend_dispatch.c src/backends/cpu/nova_cpu_backend.c src/backends/opencl/nova_opencl.c src/backends/rocm/nova_rocm.c src/backends/vulkan/nova_vulkan.c src/backends/cuda/nova_cuda.c src/backends/cross/nova_cross_compiler.c src/backends/mobile/nova_mobile_codegen.c"

# Add Metal source if on macOS (as ObjC)
if [[ "$OSTYPE" == "darwin"* ]]; then
    # We compile Metal separately as .o above, so link the object file instead of source
    # But wait, static compilation from source is easier if we can mix modes
    # Actually, simpler to link the .o file we just compiled
    EXTRA_OBJS="build/lib/nova_metal_gpu.o"
else
    EXTRA_OBJS=""
fi

if [ "$HAS_LLVM" -eq 1 ]; then
    SOURCES="$SOURCES src/backends/llvm/nova_llvm_backend.c"
    # Link: sources + metal obj + LLVM flags
    clang $CFLAGS -DHAS_LLVM -I. build/test_main.c $SOURCES $EXTRA_OBJS -o build/bin/nova_test $LDFLAGS $LLVM_LDFLAGS $LLVM_CFLAGS
else
    clang $CFLAGS -I. build/test_main.c $SOURCES $EXTRA_OBJS -o build/bin/nova_test $LDFLAGS
fi

echo "✅ Build Complete! Run ./build/bin/nova_test"
./build/bin/nova_test
