#!/bin/bash

# Ultra Advanced CUDA Matmul Build Script
# Targets: RTX A5000, RTX 4090, H100 GPUs

set -e

echo "🔧 Building Ultra Advanced CUDA Matmul for GPU Servers..."
echo "🎯 Target GPUs: RTX A5000, RTX 4090, H100"
echo "📊 Target Performance: 10+ TFLOPS"
echo

# Detect CUDA version and GPU architecture
CUDA_VERSION=$(nvcc --version | grep "release" | awk '{print $6}' | cut -d',' -f1 | cut -d'V' -f2)
echo "CUDA Version: $CUDA_VERSION"

# GPU architectures for different cards
# RTX A5000: Ampere (sm_86)
# RTX 4090: Ada Lovelace (sm_89)
# H100: Hopper (sm_90)
ARCH_FLAGS="-gencode=arch=compute_86,code=sm_86 \
            -gencode=arch=compute_89,code=sm_89 \
            -gencode=arch=compute_90,code=sm_90 \
            -gencode=arch=compute_86,code=compute_86"

# Compiler flags for ultra performance
NVCC_FLAGS="-O3 \
            --use_fast_math \
            --ptxas-options=-v \
            --maxrregcount=255 \
            --expt-relaxed-constexpr \
            --expt-extended-lambda \
            -Xcompiler -fopenmp \
            -Xptxas -dlcm=ca \
            -Xptxas -dscm=wt \
            --default-stream per-thread"

# Build the CUDA executable
echo "🚀 Compiling ultra_advanced_matmul_cuda.cu..."
nvcc $NVCC_FLAGS $ARCH_FLAGS \
     -o ultra_advanced_matmul_cuda \
     ultra_advanced_matmul_cuda.cu \
     -lcuda -lcudart -lcublas

echo "✅ Build successful!"
echo

# Show GPU information
echo "🖥️  GPU Information:"
nvidia-smi --query-gpu=name,memory.total,memory.free,driver_version --format=csv,noheader,nounits
echo

# Run benchmark if requested
if [ "$1" = "run" ]; then
    echo "🏃 Running benchmarks..."
    echo
    ./ultra_advanced_matmul_cuda
fi

echo "🎯 Ultra CUDA Matmul ready for GPU Army deployment!"
echo "📊 Expected: 10+ TFLOPS across multi-GPU configurations"
