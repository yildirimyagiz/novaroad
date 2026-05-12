# Example Optimizer Plugin

This is an example Nova plugin that demonstrates how to extend the Nova compiler with custom optimization passes.

## Features

- **Constant Folding**: Evaluates constant expressions at compile-time
- **Dead Code Elimination**: Removes unreachable code
- **Function Inlining**: Inlines small functions for better performance
- **Configurable Optimization Levels**: 0-3 (similar to -O0 to -O3)

## Building

```bash
make
```

This will produce `example_optimizer.so` (Linux), `example_optimizer.dylib` (macOS), or `example_optimizer.dll` (Windows).

## Installation

```bash
make install
```

This copies the plugin to `../../native/plugins/`.

## Usage

### From C Code

```c
#include "plugin/plugin.h"

// Load the plugin
nova_plugin_t *opt = nova_plugin_load("plugins/example_optimizer.dylib");

// Get the optimization function
void* (*optimize)(void*) = nova_plugin_get_symbol(opt, "nova_plugin_optimize");

// Configure optimization level
void (*set_level)(int) = nova_plugin_get_symbol(opt, "nova_plugin_set_opt_level");
set_level(3);

// Run optimization
void *optimized_ir = optimize(my_ir_module);

// Get statistics
size_t (*get_stats)(void) = nova_plugin_get_symbol(opt, "nova_plugin_get_stats");
printf("Applied %zu optimizations\n", get_stats());

// Cleanup
nova_plugin_unload(opt);
```

### From Nova Compiler

```bash
nova compile --plugin=example_optimizer.dylib --opt-level=3 myfile.zn
```

## API

### Configuration Functions

- `nova_plugin_set_opt_level(int level)`: Set optimization level (0-3)
- `nova_plugin_enable_inlining(bool enable)`: Enable/disable aggressive inlining
- `nova_plugin_get_stats()`: Get count of applied optimizations

### Main Function

- `nova_plugin_optimize(void* ir_module)`: Run optimization pipeline on IR module

## Extending

To add your own optimization passes:

1. Create a new function in `optimizer_plugin.c`
2. Call it from `run_optimization_pipeline()`
3. Export it if needed for external access
4. Rebuild with `make`

## License

MIT License - See main Nova repository for details.
