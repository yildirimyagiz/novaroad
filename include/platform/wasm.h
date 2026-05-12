/**
 * @file wasm.h
 * @brief WebAssembly/WASM Backend Support
 */

#ifndef NOVA_WASM_H
#define NOVA_WASM_H

#include "../config/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WASM module handle */
typedef struct nova_wasm_module nova_wasm_module_t;

/* WASM instance handle */
typedef struct nova_wasm_instance nova_wasm_instance_t;

/* WASM function handle */
typedef struct nova_wasm_function nova_wasm_function_t;

/* WASM memory handle */
typedef struct nova_wasm_memory nova_wasm_memory_t;

/* WASM value types */
typedef enum {
    NOVA_WASM_TYPE_I32,
    NOVA_WASM_TYPE_I64,
    NOVA_WASM_TYPE_F32,
    NOVA_WASM_TYPE_F64,
} nova_wasm_val_type_t;

/* WASM values */
typedef union {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
} nova_wasm_val_t;

/* ============================================================================
 * WASM Module Operations
 * ========================================================================== */

/**
 * Load WASM module from bytecode
 */
nova_wasm_module_t *nova_wasm_module_load(const void *bytecode, size_t size);

/**
 * Destroy WASM module
 */
void nova_wasm_module_destroy(nova_wasm_module_t *module);

/**
 * Get exported function
 */
nova_wasm_function_t *nova_wasm_module_get_function(nova_wasm_module_t *module, const char *name);

/**
 * Get exported memory
 */
nova_wasm_memory_t *nova_wasm_module_get_memory(nova_wasm_module_t *module, const char *name);

/* ============================================================================
 * WASM Instance Operations
 * ========================================================================== */

/**
 * Create instance from module
 */
nova_wasm_instance_t *nova_wasm_instance_create(nova_wasm_module_t *module);

/**
 * Destroy instance
 */
void nova_wasm_instance_destroy(nova_wasm_instance_t *instance);

/**
 * Get function from instance
 */
nova_wasm_function_t *nova_wasm_instance_get_function(nova_wasm_instance_t *instance, const char *name);

/**
 * Get memory from instance
 */
nova_wasm_memory_t *nova_wasm_memory_get(nova_wasm_instance_t *instance, const char *name);

/* ============================================================================
 * WASM Function Operations
 * ========================================================================== */

/**
 * Call WASM function
 */
int nova_wasm_function_call(nova_wasm_function_t *func,
                           const nova_wasm_val_t *args, size_t num_args,
                           nova_wasm_val_t *results, size_t num_results);

/**
 * Get function signature
 */
int nova_wasm_function_signature(nova_wasm_function_t *func,
                                nova_wasm_val_type_t *param_types, size_t *num_params,
                                nova_wasm_val_type_t *result_types, size_t *num_results);

/* ============================================================================
 * WASM Memory Operations
 * ========================================================================== */

/**
 * Get memory size in pages (64KB each)
 */
size_t nova_wasm_memory_size(nova_wasm_memory_t *memory);

/**
 * Grow memory by specified pages
 */
int nova_wasm_memory_grow(nova_wasm_memory_t *memory, size_t pages);

/**
 * Read from WASM memory
 */
int nova_wasm_memory_read(nova_wasm_memory_t *memory, size_t offset,
                         void *buffer, size_t size);

/**
 * Write to WASM memory
 */
int nova_wasm_memory_write(nova_wasm_memory_t *memory, size_t offset,
                          const void *buffer, size_t size);

/* ============================================================================
 * WASM Runtime
 * ========================================================================== */

/**
 * Initialize WASM runtime
 */
int nova_wasm_runtime_init(void);

/**
 * Shutdown WASM runtime
 */
void nova_wasm_runtime_shutdown(void);

/**
 * Check if WASM is supported
 */
bool nova_wasm_supported(void);

/* ============================================================================
 * Nova -> WASM Compilation
 * ========================================================================== */

/**
 * Compile Nova code to WASM bytecode
 */
int nova_compile_to_wasm(const char *nova_code, size_t code_size,
                        void **wasm_bytecode, size_t *wasm_size);

/**
 * Load Nova standard library for WASM
 */
nova_wasm_module_t *nova_wasm_load_stdlib(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_WASM_H */
