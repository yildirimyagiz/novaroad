# Nova Compiler Backends

## Overview

Nova supports multiple compilation targets through its modular backend architecture.

## Available Backends

### Native Compilation

#### CPU Backends

- **`cpu/`** - Native CPU code generation
  - `x86_64.zn` - x86-64 instruction set
  - `arm64.zn` - ARM64/AArch64
  - `risc_v.zn` - RISC-V architecture

#### LLVM Backend

- **`llvm/`** - LLVM-based compilation
  - Full optimization pipeline
  - Support for all LLVM targets
  - Debug info generation

#### Cranelift JIT

- **`jit/`** - Just-in-time compilation
  - Fast compilation for development
  - Cranelift-based code generation

---

### GPU Backends

#### NVIDIA CUDA

- **`cuda/`** - NVIDIA GPU compute
  - PTX code generation
  - CUDA kernel compilation
  - Compute capabilities: sm_70+

#### AMD ROCm

- **`rocm/`** - AMD GPU compute
  - HIP C++ generation
  - HSA runtime support
  - GFX9+ support

#### Apple Metal

- **`metal/`** - Apple GPU compute
  - Metal Shading Language (MSL)
  - Optimized for macOS/iOS/iPadOS
  - Metal 2.4+ support

#### Vulkan

- **`vulkan/`** - Cross-platform GPU
  - SPIR-V generation
  - Compute shaders
  - Vulkan 1.2+

#### OpenCL

- **`opencl/`** - Open Compute Language
  - OpenCL C generation
  - Cross-vendor GPU support
  - OpenCL 2.0+

---

### Web & Mobile

#### WebAssembly

- **`wasm/`** - WebAssembly compilation
  - WASM binary encoding
  - SIMD support (wasm-simd)
  - Threads support (wasm-threads)

#### Mobile

- **`mobile/`** - iOS & Android
  - iOS: Swift bridge + ARM64
  - Android: JNI bridge + ARM/ARM64
  - Native UI integration

---

### Advanced Backends

#### MLIR

- **`mlir/`** - Multi-Level IR
  - Advanced optimization framework
  - Custom dialects
  - Progressive lowering

#### SIMD

- **`simd/`** - Vectorization
  - Auto-vectorization
  - AVX/AVX2/AVX512 (x86)
  - NEON/SVE (ARM)

#### Cross-Compilation

- **`cross/`** - Cross-platform builds
  - Target triple management
  - Sysroot configuration
  - Multi-arch support

#### HyperFlash

- **`hyperflash/`** - Ultra-fast builds
  - Incremental compilation
  - Compilation cache
  - Development mode

#### VM

- **`vm/`** - Bytecode interpreter
  - Nova bytecode execution
  - Garbage collection
  - Dynamic runtime

---

## Backend Selection

### Automatic Selection

```bash
# Native binary for current platform
nova build

# WebAssembly
nova build --target wasm

# iOS
nova build --target ios

# Android
nova build --target android
```

### Manual Selection

```bash
# Specific backend
nova build --backend llvm
nova build --backend cranelift
nova build --backend wasm

# Specific GPU
nova build --backend cuda
nova build --backend metal
nova build --backend vulkan
```

---

## Backend Features Matrix

| Backend   | Speed      | Optimization | Debug Info | Platforms |
| --------- | ---------- | ------------ | ---------- | --------- |
| LLVM      | ⭐⭐⭐     | ⭐⭐⭐⭐⭐   | ✅         | All       |
| Cranelift | ⭐⭐⭐⭐⭐ | ⭐⭐⭐       | ✅         | Most      |
| WASM      | ⭐⭐⭐⭐   | ⭐⭐⭐⭐     | ✅         | Web       |
| CUDA      | ⭐⭐⭐⭐   | ⭐⭐⭐⭐⭐   | ✅         | NVIDIA    |
| Metal     | ⭐⭐⭐⭐   | ⭐⭐⭐⭐⭐   | ✅         | Apple     |
| ROCm      | ⭐⭐⭐⭐   | ⭐⭐⭐⭐     | ✅         | AMD       |
| VM        | ⭐⭐⭐⭐⭐ | ⭐⭐         | ✅         | All       |

---

## Performance Notes

### Production Builds

Use LLVM backend with optimization level 3:

```bash
nova build --backend llvm --opt 3 --release
```

### Development Builds

Use HyperFlash or Cranelift for fast iteration:

```bash
nova build --backend hyperflash
nova build --backend cranelift
```

### GPU Compute

Select based on hardware:

- NVIDIA: `--backend cuda`
- AMD: `--backend rocm`
- Apple: `--backend metal`
- Cross-platform: `--backend vulkan` or `--backend opencl`

---

## Adding a New Backend

1. Create directory: `backend/mybackend/`
2. Create `mod.zn` with backend implementation
3. Register in `backend/mod.zn`
4. Add tests in `tests/backend_tests.zn`

Example:

```nova
expose data MyBackend {
    // Backend state
}

skill MyBackend {
    expose fn compile(&mut self, ir: &IR) -> Result<Vec<u8>, String> {
        // Code generation logic
    }
}
```

---

## Architecture

```
IR (Nova Intermediate Representation)
    ↓
Backend Selection
    ↓
    ├─→ LLVM → Native Binary
    ├─→ WASM → WebAssembly
    ├─→ CUDA → PTX → GPU Binary
    ├─→ Metal → MSL → GPU Binary
    ├─→ VM → Bytecode
    └─→ ...
```

---

## Implementation Status

- ✅ CPU (x86-64, ARM64)
- ✅ LLVM
- ✅ Cranelift JIT
- ✅ WebAssembly
- ✅ CUDA (NVIDIA)
- ✅ Metal (Apple)
- ✅ ROCm (AMD)
- ✅ Vulkan
- ✅ OpenCL
- ✅ MLIR
- ✅ SIMD
- ✅ Cross-compilation
- ✅ Mobile (iOS/Android)
- ✅ VM
- ✅ HyperFlash

**All backends are now implemented!** 🎉
