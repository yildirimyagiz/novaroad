/**
 * @file backend_registry.c
 * @brief Backend registry - manages all compilation backends
 */

#include "compiler/codegen.h"
#include "std/alloc.h"
#include <stdio.h>
#include <string.h>

typedef struct backend_entry {
    const char *name;
    nova_backend_type_t type;
    void *(*create_fn)(void);
    int (*compile_fn)(void *, nova_ir_module_t *);
    void (*destroy_fn)(void *);
    struct backend_entry *next;
} backend_entry_t;

static backend_entry_t *backend_registry = NULL;

/* Register a backend */
void nova_backend_register(const char *name, nova_backend_type_t type, void *(*create_fn)(void),
                           int (*compile_fn)(void *, nova_ir_module_t *),
                           void (*destroy_fn)(void *))
{
    backend_entry_t *entry = nova_alloc(sizeof(backend_entry_t));
    entry->name = name;
    entry->type = type;
    entry->create_fn = create_fn;
    entry->compile_fn = compile_fn;
    entry->destroy_fn = destroy_fn;
    entry->next = backend_registry;
    backend_registry = entry;
}

/* Count registered backends */
int nova_backend_count(void)
{
    int count = 0;
    backend_entry_t *entry = backend_registry;
    while (entry) {
        count++;
        entry = entry->next;
    }
    return count;
}

/* Find backend by name */
backend_entry_t *nova_backend_find(const char *name)
{
    backend_entry_t *entry = backend_registry;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/* Forward declarations for backend implementations */
extern void *nova_llvm_backend_create(void);
extern int nova_llvm_backend_compile(void *backend, nova_ir_module_t *ir);
extern void nova_llvm_backend_destroy(void *backend);

extern void *nova_vm_backend_create(void);
extern int nova_vm_backend_compile(void *backend, nova_ir_module_t *ir);
extern void nova_vm_backend_destroy(void *backend);

extern void *nova_jit_create(void);
extern int nova_jit_compile(void *backend, nova_ir_module_t *ir);
extern void nova_jit_destroy(void *backend);

extern void *nova_wasm_backend_create(void);
extern int nova_wasm_compile(void *backend, nova_ir_module_t *ir);
extern void nova_wasm_backend_destroy(void *backend);

extern void *nova_cranelift_backend_create(void);
extern int nova_cranelift_compile(void *backend, nova_ir_module_t *ir);
extern void nova_cranelift_backend_destroy(void *backend);

extern void *nova_metal_backend_create(void);
extern int nova_metal_compile(void *backend, nova_ir_module_t *ir);
extern void nova_metal_backend_destroy(void *backend);

extern void *nova_spirv_backend_create(void);
extern int nova_spirv_compile(void *backend, nova_ir_module_t *ir);
extern void nova_spirv_backend_destroy(void *backend);

/* 🔗 MLIR Codegen Backend */
extern void *nova_mlir_codegen_backend_create(void);
extern int nova_mlir_codegen_compile(void *backend, nova_ir_module_t *ir);
extern void nova_mlir_codegen_backend_destroy(void *backend);

/* 🦅 GPU-Army 4LUA Backend */
extern void *nova_gpu_army_backend_create(void);
extern int nova_gpu_army_compile(void *backend, nova_ir_module_t *ir);
extern void nova_gpu_army_backend_destroy(void *backend);

/* Initialize all backends */
void nova_backends_init(void)
{
    printf("🚀 Initializing Nova Compiler Backend Registry...\n");

    /* Register LLVM backend (primary optimization backend) */
    nova_backend_register("llvm", NOVA_BACKEND_LLVM, nova_llvm_backend_create,
                          nova_llvm_backend_compile, nova_llvm_backend_destroy);

    /* Register VM backend (bytecode interpreter) */
    nova_backend_register("vm", NOVA_BACKEND_VM, nova_vm_backend_create, nova_vm_backend_compile,
                          nova_vm_backend_destroy);

    /* Register JIT backend (x86_64/ARM64 native) */
    nova_backend_register("jit", NOVA_BACKEND_JIT, nova_jit_create, nova_jit_compile,
                          nova_jit_destroy);

    /* Register WASM backend (web deployment) */
    nova_backend_register("wasm", NOVA_BACKEND_WASM, nova_wasm_backend_create, nova_wasm_compile,
                          nova_wasm_backend_destroy);

    /* Register Cranelift backend (fast debug builds) */
    nova_backend_register("cranelift", NOVA_BACKEND_NATIVE, nova_cranelift_backend_create,
                          nova_cranelift_compile, nova_cranelift_backend_destroy);

#ifdef __APPLE__
    /* Register Metal backend (Apple GPU) */
    nova_backend_register("metal", NOVA_BACKEND_NATIVE, nova_metal_backend_create,
                          nova_metal_compile, nova_metal_backend_destroy);
#endif

    /* Register SPIRV backend (Vulkan/OpenCL) */
    nova_backend_register("spirv", NOVA_BACKEND_NATIVE, nova_spirv_backend_create,
                          nova_spirv_compile, nova_spirv_backend_destroy);

    /* 🔗 Register MLIR Codegen backend (IR generation for all 4 systems) */
    nova_backend_register("mlir", NOVA_BACKEND_MLIR_CODEGEN, nova_mlir_codegen_backend_create,
                          nova_mlir_codegen_compile, nova_mlir_codegen_backend_destroy);

    /* 🦅 Register GPU-Army 4LUA backend (Tiered Compute Orchestration) */
    nova_backend_register("gpu-army", NOVA_BACKEND_GPU_ARMY_4LUA, nova_gpu_army_backend_create,
                          nova_gpu_army_compile, nova_gpu_army_backend_destroy);

    printf("✅ Registered %d compiler backends\n", nova_backend_count());
}

/* List all available backends */
void nova_backends_list(void)
{
    printf("╔═══════════════════════════════════════╗\n");
    printf("║  Available Nova Compiler Backends    ║\n");
    printf("╠═══════════════════════════════════════╣\n");

    backend_entry_t *entry = backend_registry;
    int count = 0;
    while (entry) {
        printf("║  %2d. %-30s ║\n", ++count, entry->name);
        entry = entry->next;
    }

    printf("╚═══════════════════════════════════════╝\n");
    printf("Total: %d backend(s)\n", count);
}

/* Get backend by type */
void *nova_backend_create(nova_backend_type_t type)
{
    backend_entry_t *entry = backend_registry;
    while (entry) {
        if (entry->type == type) {
            return entry->create_fn();
        }
        entry = entry->next;
    }
    return NULL;
}

/* Compile using backend by name */
int nova_backend_compile_by_name(const char *name, nova_ir_module_t *ir)
{
    backend_entry_t *entry = nova_backend_find(name);
    if (!entry) {
        printf("❌ Backend '%s' not found\n", name);
        return -1;
    }

    void *backend = entry->create_fn();
    if (!backend) {
        printf("❌ Failed to create backend '%s'\n", name);
        return -1;
    }

    printf("🔨 Compiling with backend: %s\n", name);
    int result = entry->compile_fn(backend, ir);

    entry->destroy_fn(backend);
    return result;
}
