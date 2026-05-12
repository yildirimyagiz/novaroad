/**
 * 🔗 nova_mlir_bridge.h - Bridge to MLIR Compiler Infrastructure
 *
 * Provides an interface for the Nova/nova frontend to interact with
 * MLIR-based optimizations and backend generation.
 */

#ifndef NOVA_MLIR_BRIDGE_H
#define NOVA_MLIR_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct NovaMLIRContext NovaMLIRContext;
typedef struct NovaMLIRModule NovaMLIRModule;
typedef struct NovaMLIROp NovaMLIROp;

/**
 * Initialize the MLIR infrastructure
 */
NovaMLIRContext *nova_mlir_init(void);

/**
 * Create a new MLIR module (a top-level unit of compilation)
 */
NovaMLIRModule *nova_mlir_module_create(NovaMLIRContext *ctx, const char *name);

/**
 * Add a compute kernel to the MLIR module
 */
void nova_mlir_add_kernel(NovaMLIRModule *module, const char *kernel_id,
                          const char *ir_content);

/**
 * Compile the MLIR module to an executable backend representation
 */
bool nova_mlir_compile(NovaMLIRModule *module, const char *backend_target);

/**
 * Destroy and cleanup MLIR resources
 */
void nova_mlir_shutdown(NovaMLIRContext *ctx);

/**
 * High-performance JIT direct engine for the 4-Layer Army (4LUA)
 */
typedef struct {
  char *kernel_name;
  void *jit_pointer;
} NovaMLIRJitResult;

NovaMLIRJitResult nova_mlir_jit_execute(const char *op_type, void *args);

#endif // NOVA_MLIR_BRIDGE_H
