# Nova AI Kernels - Cross-Platform ML/AI Infrastructure

## Overview

This directory contains the unified kernel infrastructure for Nova's ML/AI capabilities, supporting multiple hardware backends:

- **CPU**: Optimized C implementations with SIMD (AVX/SSE on x86, NEON on ARM)
- **Metal**: Apple GPU acceleration for M1/M2/M3 chips
- **CUDA**: NVIDIA GPU acceleration (conditional compilation)
- **ROCm**: AMD GPU acceleration (planned)

## Architecture

```
nova_kernels.h              # Unified API header
nova_kernel_capabilities.c  # Runtime backend detection
nova_kernel_dispatcher.c    # Automatic backend selection
nova_kernels_cuda.cu        # CUDA implementations
nova_kernels_simd.c         # SIMD optimized CPU kernels
nova_kernels_fused.c        # Fused operation kernels
../backends/metal/            # Metal GPU kernels
../backends/cpu/              # CPU fallback kernels
```

## Backend Detection

The system automatically detects available backends at runtime:

```c
NovaKernelCapabilities caps = nova_kernel_get_capabilities();
if (caps.metal_available) {
    // Use Metal on Apple Silicon
} else if (caps.cuda_available) {
    // Use CUDA on NVIDIA
} else {
    // Fallback to optimized CPU
}
```

## Compilation

### With CUDA Support (NVIDIA GPUs)

```bash
# Requires CUDA Toolkit installed
make  # Automatically detects nvcc
```

### Without CUDA (Apple Silicon, CPU-only)

```bash
# Automatically falls back to Metal/CPU
make
```

## Kernel Operations

All kernels are backend-agnostic and automatically dispatch to the best available implementation:

- Matrix operations: matmul, transpose, reshape
- Activations: ReLU, GELU, SiLU, Sigmoid, Tanh, Softmax
- Normalization: LayerNorm, GroupNorm, BatchNorm
- Convolutions: Conv2D, DepthwiseConv2D
- Attention: Scaled dot-product, multi-head attention
- Reductions: Sum, Max, Mean
- Element-wise: Add, Mul, Div, Sub

## Performance Targets

- **Metal (M1/M2)**: 5+ TFLOPS (FP32), 10+ TFLOPS (FP16)
- **CUDA (RTX 3090)**: 30+ TFLOPS (FP32), 60+ TFLOPS (FP16)
- **CPU SIMD**: 100+ GFLOPS (optimized for small batches)

## Adding New Kernels

1. Add function signature to `nova_kernels.h`
2. Implement CPU version in `../backends/cpu/nova_kernels_cpu.c`
3. (Optional) Add SIMD version in `nova_kernels_simd.c`
4. (Optional) Add Metal version in `../backends/metal/nova_metal_gpu.c`
5. (Optional) Add CUDA version in `nova_kernels_cuda.cu`

## Conditional Compilation

CUDA kernels use conditional compilation to allow building on systems without CUDA:

```c
#ifndef NOVA_CUDA_AVAILABLE
#error "This file requires CUDA. Define NOVA_CUDA_AVAILABLE or compile with CUDA toolkit."
#endif
```

The build system automatically handles this via Makefile detection.
