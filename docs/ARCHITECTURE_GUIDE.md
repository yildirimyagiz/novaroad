# Nova Architecture Guide

## Overview

Nova is designed with a multi-layered architecture that provides:

- **High Performance**: Zero-cost abstractions and aggressive optimizations
- **Safety**: Memory safety without garbage collection
- **Expressiveness**: Modern language features with minimal boilerplate
- **Interoperability**: Seamless integration with C, Rust, and Python

## Core Architecture Layers

### 1. Frontend (Parsing & Analysis)

```
Source Code → Lexer → Parser → AST → Semantic Analysis → HIR
```

- **Lexer**: Tokenizes source code with unicode support
- **Parser**: Builds Abstract Syntax Tree (AST)
- **Semantic Analysis**: Type checking, borrow checking, lifetime analysis
- **HIR**: High-level Intermediate Representation

### 2. Middle-end (Optimization)

```
HIR → MIR → Optimization Passes → Optimized MIR
```

- **MIR**: Mid-level IR suitable for optimizations
- **Optimization**: Dead code elimination, inlining, constant folding
- **Pattern Matching**: Advanced pattern compilation

### 3. Backend (Code Generation)

```
Optimized MIR → LLVM IR → Machine Code
```

- **LLVM Backend**: Leverages LLVM for code generation
- **JIT Support**: On-the-fly compilation for REPL
- **Cross-compilation**: Multiple target architectures

## Key Subsystems

### Memory Management

- **Ownership System**: Compile-time memory safety
- **Borrow Checker**: Prevents data races and use-after-free
- **Arena Allocator**: Fast allocation for temporary data
- **GC Option**: Optional garbage collection for scripting

### Type System

- **Generics**: Full type parametrization with monomorphization
- **Traits**: Interface-like abstractions
- **Dependent Types**: Value-level type dependencies (experimental)
- **Unit Algebra**: Type-safe dimensional analysis

### Concurrency

- **Async/Await**: Ergonomic asynchronous programming
- **Actor Model**: Message-passing concurrency
- **Work-Stealing Scheduler**: Efficient task distribution
- **Lock-Free Data Structures**: High-performance concurrent collections

## Documentation

See individual component documentation in `docs/architecture/` directory.
