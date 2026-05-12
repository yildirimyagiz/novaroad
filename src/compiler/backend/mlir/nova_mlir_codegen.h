/**
 * 🔗 nova_mlir_codegen.h - MLIR Code Generator Interface
 *
 * Public API for the MLIR IR generation used by the 4LUA (4-Layer Army)
 * systems. This is the C-native counterpart of MLIRGenerator in mlir_backend.zn.
 */

#ifndef NOVA_MLIR_CODEGEN_H
#define NOVA_MLIR_CODEGEN_H

/**
 * Generate a CPU kernel in MLIR-style IR.
 * Caller must free() the returned string.
 */
char *nova_mlir_gen_cpu_kernel(const char *name, const char *body);

/**
 * Generate a SIMD-vectorized kernel in MLIR-style IR.
 * Caller must free() the returned string.
 */
char *nova_mlir_gen_simd_kernel(const char *name, const char *simd_strategy);

/**
 * Generate a tiled (cache-blocked) kernel in MLIR-style IR.
 * Caller must free() the returned string.
 */
char *nova_mlir_gen_tiled_kernel(const char *name, const char *tile_config);

/**
 * Generate a GPU kernel reference in MLIR-style IR.
 * Caller must free() the returned string.
 */
char *nova_mlir_gen_gpu_kernel(const char *name, const char *shader_path);

/**
 * Generate a distributed compute kernel reference in MLIR-style IR.
 * Caller must free() the returned string.
 */
char *nova_mlir_gen_distributed_kernel(const char *name, const char *strategy);

#endif // NOVA_MLIR_CODEGEN_H
