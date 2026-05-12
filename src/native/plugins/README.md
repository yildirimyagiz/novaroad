# Nova Plugin System

This directory contains plugins that extend the Nova compiler and runtime.

## Available Plugins

### Example Optimizer (`example_optimizer/`)
Demonstrates custom optimization passes including constant folding, dead code elimination, and function inlining.

## Creating a Plugin

### 1. Plugin Structure

```
plugins/my_plugin/
├── my_plugin.c          # Plugin implementation
├── Makefile             # Build configuration
└── README.md            # Documentation
```

### 2. Basic Plugin Template

```c
#include "plugin/plugin.h"
#include <stdio.h>

// Define plugin metadata
NOVA_PLUGIN_DEFINE(
    "my_plugin",           // Name
    "1.0.0",              // Version
    "Your Name",          // Author
    "Plugin description"  // Description
);

// Initialization (optional)
int plugin_init(void) {
    printf("My plugin initialized!\n");
    return 0;  // 0 = success
}

// Cleanup (optional)
void plugin_cleanup(void) {
    printf("My plugin shutting down\n");
}

// Export lifecycle functions
NOVA_PLUGIN_INIT(plugin_init);
NOVA_PLUGIN_CLEANUP(plugin_cleanup);

// Export your custom functions
extern void my_plugin_function(void) {
    printf("Hello from my plugin!\n");
}
```

### 3. Building

Create a `Makefile`:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -I../../native/include
LDFLAGS = -shared

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = my_plugin.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = my_plugin.so
endif

all: $(TARGET)

$(TARGET): my_plugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
```

Then build:

```bash
cd plugins/my_plugin
make
```

### 4. Loading Your Plugin

```c
#include "plugin/plugin.h"

// Load plugin
nova_plugin_t *plugin = nova_plugin_load("plugins/my_plugin/my_plugin.dylib");
if (!plugin) {
    fprintf(stderr, "Failed to load plugin: %s\n", nova_plugin_error());
    return 1;
}

// Get plugin info
const nova_plugin_info_t *info = nova_plugin_get_info(plugin);
printf("Loaded: %s v%s by %s\n", info->name, info->version, info->author);

// Get function from plugin
void (*my_func)(void) = nova_plugin_get_symbol(plugin, "my_plugin_function");
if (my_func) {
    my_func();  // Call plugin function
}

// Unload when done
nova_plugin_unload(plugin);
```

## Plugin Types

Nova supports several types of plugins:

### Backend Plugins
Add support for new hardware accelerators or execution targets.

```c
nova_plugin_register_capability(
    plugin,
    NOVA_PLUGIN_CAP_BACKEND,
    &my_backend_impl
);
```

### Optimizer Plugins
Implement custom optimization passes.

```c
nova_plugin_register_capability(
    plugin,
    NOVA_PLUGIN_CAP_OPTIMIZER,
    &my_optimizer_impl
);
```

### Profiler Plugins
Add custom profiling and instrumentation.

```c
nova_plugin_register_capability(
    plugin,
    NOVA_PLUGIN_CAP_PROFILER,
    &my_profiler_impl
);
```

### Allocator Plugins
Provide custom memory allocation strategies.

```c
nova_plugin_register_capability(
    plugin,
    NOVA_PLUGIN_CAP_ALLOCATOR,
    &my_allocator_impl
);
```

## API Reference

See `native/include/plugin.h` for complete API documentation.

### Core Functions

- `nova_plugin_load(path)` - Load a plugin
- `nova_plugin_unload(plugin)` - Unload a plugin
- `nova_plugin_reload(plugin)` - Hot-reload a plugin
- `nova_plugin_get_info(plugin)` - Get plugin metadata
- `nova_plugin_get_symbol(plugin, name)` - Get exported symbol

### Registry Functions

- `nova_plugin_register(name, plugin)` - Register plugin by name
- `nova_plugin_find(name)` - Find plugin by name
- `nova_plugin_get_all(out, max)` - Get all loaded plugins

### Capability Functions

- `nova_plugin_register_capability(plugin, type, impl)` - Register capability
- `nova_plugin_find_by_capability(type, out, max)` - Find plugins by capability

## Best Practices

1. **Always check return values** - Plugin loading can fail
2. **Use NOVA_PLUGIN_DEFINE macro** - Ensures consistent metadata
3. **Clean up resources** - Implement cleanup function
4. **Thread safety** - Make plugins thread-safe if needed
5. **Version compatibility** - Check Nova API version
6. **Error handling** - Provide clear error messages

## Examples

See the `example_optimizer/` directory for a complete working example.

## Testing Plugins

Create a test program:

```c
#include "plugin/plugin.h"
#include <stdio.h>

int main(void) {
    nova_plugin_system_init();
    
    nova_plugin_t *plugin = nova_plugin_load("my_plugin.dylib");
    if (!plugin) {
        fprintf(stderr, "Error: %s\n", nova_plugin_error());
        return 1;
    }
    
    printf("Plugin loaded successfully!\n");
    
    nova_plugin_unload(plugin);
    nova_plugin_system_shutdown();
    
    return 0;
}
```

Compile and run:

```bash
gcc -I../native/include -L../native/lib -lnova test.c -o test
./test
```

## Troubleshooting

### Plugin won't load
- Check file path is correct
- Verify plugin has correct extension (.so/.dylib/.dll)
- Check permissions (file must be readable/executable)
- Use `nova_plugin_error()` to get error message

### Symbol not found
- Ensure function is declared `extern`
- Check function name (no C++ name mangling)
- Verify plugin was built with `-fPIC` flag

### Crashes on load
- Check plugin doesn't have missing dependencies
- Verify plugin API version matches Nova version
- Use debugger to find crash location

## Contributing

When contributing a plugin:

1. Create a directory under `plugins/`
2. Include Makefile and README.md
3. Add comprehensive documentation
4. Provide usage examples
5. Test on multiple platforms

## License

Plugins inherit the license of the Nova project unless otherwise specified.
