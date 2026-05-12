# Nova Stdlib - Core Module

## Overview

The core module provides fundamental language features and primitives that form
the foundation of Nova programs.

## Modules

### core.zn

**Purpose**: Core language utilities and primitives

- Basic data type operations
- String manipulation functions
- Core utility functions (min, max, abs, etc.)
- Type conversion utilities

### collections.zn

**Purpose**: Basic collection data structures

- Array operations
- List manipulation
- Basic iteration utilities

### io.zn

**Purpose**: Basic input/output operations

- Console I/O functions
- Basic file operations
- Stream utilities

### **compatibility**.zen

**Purpose**: Cross-version compatibility layer

- API compatibility functions
- Version detection
- Migration utilities

### **version**.zen

**Purpose**: Version information and metadata

- Library version constants
- Build information
- Feature flags

### universal_constants.zen

**Purpose**: Universal physical and mathematical constants

- Physical constants (speed of light, Planck's constant, etc.)
- Mathematical constants (pi, e, golden ratio, etc.)
- Astronomical constants

## Usage Examples

```cpp
// Import core functionality
import std::core;

// Use basic utilities
let result = core::max(10, 20);
let str = core::to_string(42);

// Use collections
let arr = [1, 2, 3, 4, 5];
let sum = collections::sum(arr);

// Basic I/O
io::println("Hello, Nova!");
let input = io::read_line();
```

## Dependencies

- None (this is the foundation layer)

## Testing

```bash
nova test core/
```

## Performance Notes

- Core functions are optimized for minimal overhead
- Memory allocations are kept to minimum
- Functions are designed for inlining where appropriate
