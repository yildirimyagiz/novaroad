# Nova Native Backends

This directory houses the hardware-specific backends and low-level drivers
required for multi-platform support.

## Supported Backends

- **`cpu`**: Generic x86_64/ARM implementation.
- **`metal`**: High-performance backend for Apple Silicon (macOS/iOS).
- **`vulkan`**: Cross-platform GPU acceleration for Linux, Windows, and Android.
- **`cuda` / `nvlink`**: NVIDIA specific GPU and interconnect optimizations.
- **`wasm`**: Backend for WebAssembly and browser-based execution.

## Purpose

The backends layer abstracts away the complexity of different hardware
architectures, providing a unified interface for the Nova compiler and
runtime to dispatch compute tasks.
