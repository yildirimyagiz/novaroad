# Nova MLIR Bridge - Mojo-Level Performance

This directory contains the bridge implementations between Nova's HIR (High-level IR) and MLIR, along with the Gödel symbolic verification engine integration.

## Overview

The MLIR bridge provides two main components:

1. **Gödel MLIR Bridge** - Formal verification and optimization
2. **HIR to MLIR Lowering** - Convert Nova HIR to MLIR with Mojo-specific optimizations

## Architecture

```
┌─────────────────┐
│  Nova HIR     │
│  (Frontend)     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐      ┌──────────────────┐
│ HIR to MLIR     │─────▶│  Gödel Bridge    │
│ Lowering        │      │  (Verification)  │
└────────┬────────┘      └──────────────────┘
         │
         ▼
┌─────────────────┐
│  MLIR IR        │
│  (Nova        │
│   Dialect)      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  LLVM Backend   │
│  (Native Code)  │
└─────────────────┘
```

## Files

### Core Implementation

- **`godel_mlir_bridge.h/cpp`** - Gödel symbolic verification engine bridge
  - Formal verification of MLIR operations
  - Memory safety verification
  - SIMD alignment checking
  - Mojo-level optimization hints

- **`hir_to_mlir.h/cpp`** - HIR to MLIR lowering
  - Convert Nova HIR to MLIR
  - Type conversion (including SIMD types)
  - Mojo-specific optimizations
  - Compile-time evaluation hints

### Testing

- **`test_bridge.cpp`** - Comprehensive test suite
  - Gödel bridge verification tests
  - HIR lowering tests
  - Mojo-specific feature tests

- **`CMakeLists.txt`** - Build configuration

## Mojo-Specific Features

### 1. SIMD Operations

The bridge supports Mojo-style SIMD operations with automatic vectorization:

```cpp
// HIR SIMD type
HIRSIMDType(f32Type, 8);  // simd<f32, 8>

// Lowered to MLIR with vectorization hints
mlir::VectorType::get({8}, f32Type);
op->setAttr("nova.vectorize", builder.getUnitAttr());
op->setAttr("nova.simd_width", builder.getI32IntegerAttr(8));
```

### 2. Compile-Time Evaluation

Functions can be marked for compile-time evaluation (Mojo-style):

```cpp
func->isCompileTime = true;
// Lowered to:
funcOp->setAttr("nova.const_eval", builder.getUnitAttr());
```

### 3. Inline Hints

Mojo-style inlining for zero-cost abstractions:

```cpp
func->isInline = true;
// Lowered to:
funcOp->setAttr("nova.inline", builder.getUnitAttr());
```

### 4. FMA (Fused Multiply-Add)

Automatic detection and optimization of FMA patterns:

```cpp
// a * b + c is automatically detected and marked
addOp->setAttr("nova.fma", builder.getUnitAttr());
```

## HIR Type System

The bridge implements a complete HIR type system:

### Basic Types

- **`HIRIntegerType`** - Integer types (i8, i16, i32, i64)
- **`HIRFloatType`** - Floating-point types (f32, f64)
- **`HIRSIMDType`** - SIMD vector types (Mojo-specific)

### Expressions

- **`HIRLiteral`** - Integer and floating-point literals
- **`HIRVariable`** - Variable references
- **`HIRBinaryOp`** - Binary operations (Add, Sub, Mul, Div, etc.)
- **`HIRSIMDOp`** - SIMD operations (Load, Store, Add, Mul, FMA)

### Statements

- **`HIRVarDecl`** - Variable declarations
- **`HIRReturn`** - Return statements

### Functions and Modules

- **`HIRFunction`** - Function definitions with parameters and body
- **`HIRModule`** - Module containing multiple functions

## Gödel Verification

The Gödel bridge provides formal verification for:

### 1. Memory Safety

- No use-after-free
- No double-free
- Proper ownership transfer
- Valid borrow scopes

### 2. SIMD Alignment

- Proper vector alignment (Mojo requirement)
- Correct SIMD widths
- Platform-specific optimizations

### 3. Effect System

- Pure function verification
- I/O effect tracking
- Memory effect tracking
- Exception handling

### 4. Optimization Hints

Automatic detection and marking of:

- Vectorizable operations
- Compile-time constant expressions
- Inlining candidates (zero-cost abstractions)

## Usage Example

### Basic HIR Lowering

```cpp
#include "hir_to_mlir.h"

mlir::MLIRContext context;
context.getOrLoadDialect<mlir::func::FuncDialect>();
context.getOrLoadDialect<mlir::arith::ArithDialect>();

nova::HIRToMLIRLowering lowering(&context);

// Create HIR module
auto* hirModule = new nova::HIRModule("my_module");
// ... populate with functions ...

// Lower to MLIR
auto mlirModule = lowering.lower(*hirModule);
mlirModule->dump();
```

### Gödel Verification

```cpp
#include "godel_mlir_bridge.h"

mlir::MLIRContext context;
auto bridge = nova::createGodelBridge(&context);

// Verify MLIR module
auto result = bridge->lowerWithVerification(mlirModule);
if (result.failed()) {
  // Verification failed
}

// Apply Mojo optimizations
bridge->applyMojoOptimizations(mlirModule);
```

## Building

```bash
cd compiler/mlir/bridge
mkdir build && cd build
cmake ..
make

# Run tests
./test_mlir_bridge
```

## Integration with Nova Compiler

The bridge integrates into the Nova compiler pipeline:

1. **Frontend** → Parse Nova source to AST
2. **Semantic Analysis** → Type checking, borrow checking
3. **HIR Generation** → Convert AST to HIR
4. **HIR to MLIR** → Lower HIR to MLIR (this bridge)
5. **Gödel Verification** → Verify MLIR operations
6. **MLIR Optimization** → Apply MLIR passes
7. **LLVM Backend** → Lower to LLVM IR and native code

## Performance Goals

Based on Mojo's architecture:

| Feature | Mojo | Nova Target | Status |
|---------|------|---------------|--------|
| SIMD Width | 8-32 | 8-32 | ✓ Implemented |
| Vectorization | Auto | Auto | ✓ Implemented |
| Inline Hints | Yes | Yes | ✓ Implemented |
| Compile-time Eval | Yes | Yes | ✓ Implemented |
| FMA Fusion | Yes | Yes | ✓ Implemented |
| Zero-cost Abstractions | Yes | Yes | ✓ Implemented |

## Attributes Reference

### Function Attributes

- `nova.inline` - Mark function for inlining
- `nova.const_eval` - Mark for compile-time evaluation
- `nova.vectorize` - Enable vectorization
- `nova.simd_width` - SIMD width hint (4, 8, 16, 32)
- `nova.fma` - Mark for FMA fusion

### Type Attributes

- SIMD types map to `mlir::VectorType`
- Preserve element type and width information

## Future Enhancements

- [ ] GPU kernel lowering (Metal/CUDA)
- [ ] Async/await lowering
- [ ] Advanced loop optimizations
- [ ] Parametric polymorphism support
- [ ] Dependent type lowering
- [ ] Effect system integration
- [ ] Borrow checker integration

## Contributing

When extending the bridge:

1. Add new HIR types in `hir_to_mlir.cpp`
2. Implement type conversion in `convertType()`
3. Add lowering logic in `lowerExpression()` or `lowerStatement()`
4. Add verification in Gödel bridge if needed
5. Add tests in `test_bridge.cpp`
6. Update this README

## References

- [MLIR Documentation](https://mlir.llvm.org/)
- [Mojo Language](https://docs.modular.com/mojo/)
- [Nova Compiler Architecture](../../NOVA_SUPREME_COMPILER_ARCHITECTURE.md)
- [MLIR Integration Roadmap](../../core/mlir_integration_roadmap.md)

## License

Part of the Nova Project. See LICENSE for details.
