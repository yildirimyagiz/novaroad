# Nova Native AI Engine

This directory contains the core native components for AI and Machine Learning
operations. It provides high-performance kernels, model infrastructures, and
training utilities.

## Directory Structure

- **`/kernels`**: Optimized compute kernels (SIMD, AVX, Metal, CUDA) for tensor
  operations.
- **`/models`**: Native implementation of standard model architectures
  (Transformers, CNNs, etc.).
- **`/training`**: Backpropagation, gradient descent, and optimization
  algorithms.
- **`/numerics`**: Specialized numerical types and precision management (FP8,
  BF16).
- **`/io`**: High-performance data loaders and tensor serialization logic.

## Purpose

The Native AI Engine is designed to bridge the gap between high-level Nova
code and raw hardware performance, ensuring that `nova_nn` operations run at
peak efficiency on all supported devices.
