#!/bin/bash

# Ultra Cross-Platform Matmul Build Script
# Supports CUDA, ROCm, DirectX, OpenCL, Vulkan, Metal

set -e

# Detect OS and available backends
OS=$(uname -s)
ARCH=$(uname -m)

echo "🎯 ULTRA CROSS-PLATFORM MATMUL BUILD"
echo "====================================="
echo "OS: $OS, Architecture: $ARCH"
echo

# Backend detection
BACKENDS_AVAILABLE=()

# CUDA detection (NVIDIA)
if command -v nvcc &> /dev/null; then
    BACKENDS_AVAILABLE+=("CUDA")
    echo "✅ CUDA detected (NVIDIA)"
else
    echo "❌ CUDA not detected"
fi

# ROCm detection (AMD)
if [[ -d "/opt/rocm" ]] || command -v hipcc &> /dev/null; then
    BACKENDS_AVAILABLE+=("ROCm")
    echo "✅ ROCm detected (AMD)"
else
    echo "❌ ROCm not detected"
fi

# DirectX detection (Windows)
if [[ "$OS" == "MINGW"* ]] || [[ "$OS" == "MSYS"* ]]; then
    BACKENDS_AVAILABLE+=("DirectX")
    echo "✅ DirectX detected (Windows)"
fi

# OpenCL detection
if [[ -f "/usr/include/CL/cl.h" ]] || [[ -f "/usr/local/include/CL/cl.h" ]]; then
    BACKENDS_AVAILABLE+=("OpenCL")
    echo "✅ OpenCL detected"
else
    echo "❌ OpenCL not detected"
fi

# Vulkan detection
if command -v vulkaninfo &> /dev/null || pkg-config --exists vulkan; then
    BACKENDS_AVAILABLE+=("Vulkan")
    echo "✅ Vulkan detected"
else
    echo "❌ Vulkan not detected"
fi

# Metal detection (macOS/iOS)
if [[ "$OS" == "Darwin" ]]; then
    BACKENDS_AVAILABLE+=("Metal")
    echo "✅ Metal detected (Apple)"
fi

echo
echo "Available backends: ${BACKENDS_AVAILABLE[*]}"
echo

# Build flags based on OS and available backends
BUILD_FLAGS=""
LINK_FLAGS=""

case $OS in
    "Linux")
        echo "🏗️  Building for Linux..."

        # CUDA
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " CUDA " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__CUDA__"
            LINK_FLAGS="$LINK_FLAGS -lcuda -lcudart"
        fi

        # ROCm
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " ROCm " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__HIP__"
            export HIP_PATH=/opt/rocm/hip
            LINK_FLAGS="$LINK_FLAGS -L/opt/rocm/lib -lhiprtc -lamdhip64"
        fi

        # OpenCL
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " OpenCL " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__OPENCL__"
            LINK_FLAGS="$LINK_FLAGS -lOpenCL"
        fi

        # Vulkan
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " Vulkan " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__VULKAN__"
            LINK_FLAGS="$LINK_FLAGS $(pkg-config --libs vulkan)"
        fi

        # Common Linux flags
        CXX=g++
        BUILD_FLAGS="$BUILD_FLAGS -std=c++17 -O3 -march=native -fopenmp"
        LINK_FLAGS="$LINK_FLAGS -fopenmp -lpthread"
        ;;

    "Darwin")
        echo "🏗️  Building for macOS..."

        # Metal
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " Metal " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__APPLE__"
            LINK_FLAGS="$LINK_FLAGS -framework Metal -framework Foundation"
        fi

        # OpenCL (deprecated on macOS, but still available)
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " OpenCL " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__OPENCL__"
            LINK_FLAGS="$LINK_FLAGS -framework OpenCL"
        fi

        CXX=clang++
        BUILD_FLAGS="$BUILD_FLAGS -std=c++17 -O3 -march=native"
        LINK_FLAGS="$LINK_FLAGS -lpthread"
        ;;

    MINGW*|MSYS*)
        echo "🏗️  Building for Windows..."

        # DirectX
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " DirectX " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D_WIN32"
            LINK_FLAGS="$LINK_FLAGS -ld3d12 -ldxgi -ldxguid"
        fi

        # CUDA (Windows)
        if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " CUDA " ]]; then
            BUILD_FLAGS="$BUILD_FLAGS -D__CUDA__"
            LINK_FLAGS="$LINK_FLAGS -lcuda -lcudart"
        fi

        CXX=g++
        BUILD_FLAGS="$BUILD_FLAGS -std=c++17 -O3 -march=native"
        LINK_FLAGS="$LINK_FLAGS -lpthread"
        ;;

    *)
        echo "❌ Unsupported OS: $OS"
        exit 1
        ;;
esac

echo "Build flags: $BUILD_FLAGS"
echo "Link flags: $LINK_FLAGS"
echo

# Build the cross-platform executable
echo "🔨 Compiling ultra_cross_platform_matmul.cpp..."

if [[ " ${BACKENDS_AVAILABLE[@]} " =~ " CUDA " ]]; then
    # CUDA build (separate compilation)
    echo "Building CUDA components..."
    nvcc -c cuda_backend.cu -o cuda_backend.o -Xcompiler -fPIC
    $CXX $BUILD_FLAGS -c ultra_cross_platform_matmul.cpp -o main.o
    $CXX main.o cuda_backend.o -o ultra_cross_platform_matmul $LINK_FLAGS
elif [[ " ${BACKENDS_AVAILABLE[@]} " =~ " ROCm " ]]; then
    # ROCm build
    echo "Building ROCm components..."
    hipcc -c hip_backend.hip -o hip_backend.o -fPIC
    $CXX $BUILD_FLAGS -c ultra_cross_platform_matmul.cpp -o main.o
    $CXX main.o hip_backend.o -o ultra_cross_platform_matmul $LINK_FLAGS
else
    # Standard build
    $CXX $BUILD_FLAGS ultra_cross_platform_matmul.cpp -o ultra_cross_platform_matmul $LINK_FLAGS
fi

echo "✅ Build successful!"
echo

# Run tests if requested
if [[ "$1" == "test" ]]; then
    echo "🧪 Running cross-platform benchmark..."

    if [[ -f "./ultra_cross_platform_matmul" ]]; then
        ./ultra_cross_platform_matmul
    else
        echo "❌ Executable not found!"
        exit 1
    fi
fi

echo
echo "🎯 Cross-platform matmul ready!"
echo "Available backends: ${BACKENDS_AVAILABLE[*]}"
echo "Run: ./ultra_cross_platform_matmul"
echo
echo "🚀 GPU Army supports ALL server types!"
