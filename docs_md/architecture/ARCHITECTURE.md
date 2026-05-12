# Nova Architecture Overview

## System Layers

```text

┌─────────────────────────────────────────┐
│         Application Layer               │
│    (Nova User Programs)                 │
├─────────────────────────────────────────┤
│         Compiler Frontend               │
│  Lexer → Parser → Type Checker → AST   │
├─────────────────────────────────────────┤
│         Compiler Backend                │
│  IR → Optimizer → Codegen (VM/JIT)     │
├─────────────────────────────────────────┤
│         Runtime System                  │
│  GC | Async | Actors | Threads         │
├─────────────────────────────────────────┤
│         AI Subsystem                    │
│  Tensors | NN | Inference | Autograd   │
├─────────────────────────────────────────┤
│         Standard Library                │
│  Collections | I/O | Networking         │
├─────────────────────────────────────────┤
│         Security Layer                  │
│  Capabilities | Sandbox | Crypto        │
├─────────────────────────────────────────┤
│         Kernel Layer                    │
│  Scheduler | Memory | IPC | Drivers    │
├─────────────────────────────────────────┤
│    Hardware Abstraction Layer (HAL)    │
│  x86_64 | ARM64 | RISC-V | WASM       │
└─────────────────────────────────────────┘
```

## Key Components

### 1. Kernel (`src/kernel/`)

**Purpose**: Microkernel providing core OS services

**Modules**:

- **Core**: Boot, scheduler, IPC, syscalls
- **Memory**: Frame allocator, VMM, paging, heap
- **HAL**: Architecture-specific implementations
- **Drivers**: Display, input, storage, network, GPU

**Design Principles**:

- Microkernel architecture
- Capability-based security
- Message-passing IPC
- Preemptive multitasking

### 2. Runtime (`src/runtime/`)

**Purpose**: Language runtime and execution environment

**Features**:

- **GC**: Mark-sweep and generational collectors
- **Async**: Event loop, coroutines, futures
- **Concurrency**: Threads, channels, mutexes
- **Actors**: Message-passing concurrency model
- **FFI**: C and Python interoperability

### 3. Compiler (`src/compiler/`)

**Pipeline**:

```text

Source → Lexer → Parser → AST → Semantic Analysis → IR → Optimizer → Codegen
```

**Components**:

- **Frontend**: Lexical analysis, recursive descent parsing, AST generation.
- **Unified Type System**: Centralized in `nova_types.h`, supporting HM inference, borrow checking, and Nova-specific types (Tensors, Flows, Effects).
- **Semantic Analysis**: Multi-pass analysis including symbol resolution and trait detection.
- **Unified IR**: SSA-form intermediate representation with effect tracking and proof levels.
- **Optimizer**: Constant folding, DCE, and target-independent optimizations.
- **Backend**:
  - **VM**: High-performance register-based bytecode interpreter.
  - **JIT**: Native x86_64/AArch64 code generation (Stable for Stage 1 bootstrap).
  - **GPU**: Metal (macOS) and SPIR-V backends for compute kernels.
  - **LLVM**: Production-grade backend (In Integration).

### 4. AI Subsystem (`src/ai/`)

**Purpose**: First-class AI/ML support integrated at the language level.

**Features**:

- **Tensors**: N-dimensional arrays with native language support via `tensor<T>[Shape]`.
- **Hardware Acceleration**: Automatic dispatch to CPU (SIMD), GPU (Metal/Vulkan), or NPU.
- **Neural Networks**: High-level layers and autograd support.
- **Inference**: Support for GGUF/ONNX formats and native quantization.

## Build System

Nova utilizes **CMake** as the primary build system, producing the following core artifacts:

- `libnova_std.a`: Standard library.
- `libnova_runtime.a`: Language runtime.
- `libnova_compiler.a`: Unified compiler infrastructure.
- `novac`: The Nova compiler driver.

## Concurrency Model

Nova provides multiple concurrency models:

1. **Green Threads** (M:N scheduler with low overhead).
2. **Async/Await** (Stackless coroutines for I/O bound tasks).
3. **Actors** (Distributed-ready message-passing).
4. **Flows**: Reactive/async data streams integrated into the type system (`flow<T>`).

## Future Enhancements

- [x] Unified Type System Infrastructure
- [x] Stable x86_64 JIT for Bootstrap
- [ ] Complete LLVM backend integration
- [ ] Full RISC-V support
- [ ] WebAssembly runtime
- [ ] Distributed actor system
- [ ] Advanced dependent types and linear logic integration
