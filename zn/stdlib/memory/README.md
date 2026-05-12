# Nova Standard Library - Memory Module

The `memory` module provides advanced memory management utilities, smart
pointers, memory pools, and garbage collection interfaces for Nova
applications requiring fine-grained control over memory allocation and lifecycle
management.

## Architecture

```
memory/
├── allocators/           # Custom memory allocators
│   ├── arena.zn         # Arena-based allocation
│   ├── pool.zn          # Memory pools
│   └── bump.zn          # Bump allocation
├── smart_ptr.zn         # Smart pointers (Rc, Arc, Box)
├── gc.zn                # Garbage collection interfaces
├── buffer.zn            # Memory buffers and views
├── alignment.zn         # Memory alignment utilities
└── memory.zn            # High-level memory management
```

## Key Features

### Smart Pointers

- `Rc<T>`: Reference-counted smart pointer for single-threaded use
- `Arc<T>`: Atomic reference-counted smart pointer for multi-threaded use
- `Box<T>`: Unique ownership smart pointer
- `Weak<T>`: Weak references to prevent cycles

### Memory Allocators

- Arena allocator for temporary allocations
- Pool allocator for fixed-size objects
- Bump allocator for linear allocation patterns
- Custom allocator interfaces

### Memory Utilities

- Memory-mapped files
- Buffer management with automatic cleanup
- Memory alignment helpers
- Memory statistics and profiling

## Usage Examples

### Smart Pointers

```cpp
use memory::smart_ptr::{Rc, Arc, Box};

fn main() {
    // Reference counting
    let shared = Rc::new("Hello");
    let shared2 = shared.clone();
    println("References: {}", shared.strong_count());

    // Atomic reference counting for threads
    let atomic_shared = Arc::new(vec![1, 2, 3]);

    // Unique ownership
    let boxed = Box::new(42);
}
```

### Memory Pools

```cpp
use memory::allocators::pool::Pool;

fn main() {
    let mut pool = Pool::new(sizeof::<MyStruct>());

    // Allocate from pool
    let obj1 = pool.allocate();
    let obj2 = pool.allocate();

    // Objects automatically freed when pool is dropped
}
```

### Arena Allocation

```cpp
use memory::allocators::arena::Arena;

fn main() {
    let arena = Arena::new();

    // All allocations from this arena
    let data1 = arena.allocate::<Vec<i32>>();
    let data2 = arena.allocate::<String>();

    // All memory freed when arena goes out of scope
}
```

## Performance Characteristics

- **Zero-cost abstractions**: Smart pointers compile to efficient reference
  counting
- **Memory pools**: Reduce allocation overhead for frequent small allocations
- **Arena allocation**: Minimize heap allocations for temporary data
- **Memory profiling**: Track allocation patterns and detect leaks

## Integration with Other Modules

- **Concurrency**: `Arc<T>` integrates with the concurrency module for
  thread-safe sharing
- **System**: Memory-mapped files work with system I/O operations
- **Core**: Fundamental memory types used throughout the standard library

## Safety

- Bounds checking on buffer operations
- Automatic memory cleanup prevents leaks
- Thread-safe reference counting
- Memory corruption detection in debug builds

## Testing

Run the memory module tests:

```bash
nova test stdlib/memory/
```

## Contributing

When adding new memory management features:

1. Ensure thread safety for shared types
2. Provide both safe and unsafe APIs where appropriate
3. Include comprehensive tests and benchmarks
4. Document memory ownership semantics clearly
