# Nova Native Runtime

This directory contains the core runtime implementation for the Nova
language, written in C. These components provide the low-level logic required
for the language execution, memory management, and system interaction.

## Core Components

- **`nova_context.c`**: Manages the execution context, including hardware
  detection, memory pool allocation, and formal verification modes.
- **`nova_runtime_io.c`**: Implements basic I/O operations such as console
  printing, file reading/writing, and input parsing.
- **`nova_gc.c` / `nova_gc_concurrent.c`**: Managed memory and garbage
  collection logic (Precise/Concurrent).
- **`nova_deterministic.c`**: Ensures deterministic execution patterns for
  formal verification and reproducible AI workloads.
- **`nova_jit.c`**: Provides the native hooks for Just-In-Time compilation and
  hot-patching.
- **`nova_parallel.c`**: Orchestrates parallel execution and multi-threading
  primitives.

## Structure

- `/sandbox`: Contains isolated execution environments for untrusted code.
- `/telemetry`: Runtime profiling and performance monitoring tools.

## Integration

These C files are typically compiled and linked into the final binary during the
bootstrap process or called via FFI (Foreign Function Interface) from the
Nova standard library.
