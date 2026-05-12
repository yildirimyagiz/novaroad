# Nova C Headers - Complete Reference

## 📊 Overview

**Total Headers:** 38
**Total Lines:** ~9,000+
**Total Functions:** ~950+

## 🎯 Nova-Specific Features

### New Headers for Nova Language:
1. **compiler/dimensions.h** - Dimensional analysis (kg, m/s², etc.)
2. **compiler/flow.h** - Reactive flow types
3. **compiler/contracts.h** - Design by contract (require, ensure, proof)

### Enhanced Existing Headers:
- **compiler/types.h** - Added dimensional, flow, tensor shape types
- **compiler/ast.h** - Full Nova AST nodes
- **ai/tensor.h** - Named dimensions support

## 📚 Categories

### Core Standard Library (std/)
- alloc.h - Memory allocation with arena/pool
- string.h - UTF-8 strings with rope
- collections.h - Vec, HashMap, BTree
- io.h - File I/O, buffering
- net.h - TCP/UDP sockets, TLS

### Runtime System (runtime/)
- runtime.h - Runtime lifecycle
- gc.h - Generational GC with weak refs
- async.h - Event loop, coroutines, futures
- actor.h - Actor model
- thread.h - Thread pool, mutex, channels
- value.h - Tagged union values

### Compiler (compiler/)
- compiler.h - Pipeline control
- lexer.h - Tokenization with source maps
- parser.h - Recursive descent parsing
- ast.h - Complete AST (90+ node types)
- types.h - Type system with Nova features
- ir.h - SSA-form IR
- codegen.h - Multi-backend codegen
- **dimensions.h** - ⭐ NEW: Dimensional analysis
- **flow.h** - ⭐ NEW: Reactive streams
- **contracts.h** - ⭐ NEW: Design by contract

### AI/ML (ai/)
- tensor.h - N-dim tensors with named dims
- nn.h - Neural network layers
- inference.h - Model inference engine
- autograd.h - Automatic differentiation

### Security (security/)
- capabilities.h - Capability-based security
- sandbox.h - Process sandboxing
- crypto.h - Hash, AEAD, KDF, signing

### Kernel (kernel/)
- kernel.h - Kernel lifecycle
- sched.h - Process/thread scheduler
- ipc.h - Message passing
- syscall.h - System call interface
- memory.h - Memory management
- hal.h - Hardware abstraction

### Platform (platform/)
- platform.h - Platform detection
- atomic.h - Lock-free operations
- simd.h - SIMD intrinsics (AVX2/NEON)
- intrinsics.h - CPU intrinsics

## 🚀 Usage Example

```c
#include <nova/nova.h>

int main() {
    // Initialize Nova runtime
    nova_runtime_init();
    
    // Dimensional analysis
    nova_dimension_t *mass = nova_dimension_create("kg", 1, 0, 0);
    nova_dimensional_value_t m = nova_dimensional_value(5.0, mass);
    
    // Flow programming
    nova_flow_t *sensor = nova_flow_create(NOVA_FLOW_HOT);
    nova_flow_emit(sensor, &data);
    
    // Contracts
    nova_contract_t *contract = nova_contract_create();
    nova_contract_require(contract, "x > 0", check_positive);
    
    // Cleanup
    nova_runtime_shutdown();
    return 0;
}
```

## 📝 Next Steps

1. ✅ Headers complete
2. 🔄 Implement C source files
3. 🔄 Add unit tests
4. 🔄 Integration with Nova self-hosted compiler
5. 🔄 Cross-platform testing

---
Generated: 2026-02-24
