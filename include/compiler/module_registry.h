/**
 * nova/compiler/module_registry.h
 * ──────────────────────────────────────────────────────────────────────────────
 * Module registry for Nova's import/export system.
 */

#pragma once

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Exported symbol ────────────────────────────────────────────────────────── */

typedef struct nova_export_t {
    char *name;        /**< Symbol name as declared */
    nova_type_t *type; /**< Type (owned by this struct) */
    bool is_fn;        /**< True if a function, false if a variable */
    struct nova_export_t *next;
} nova_export_t;

/* ── Module descriptor ─────────────────────────────────────────────── */

typedef enum {
    NOVA_MODULE_PENDING,  /**< Not yet compiled */
    NOVA_MODULE_COMPILED, /**< Successfully parsed + analyzed */
    NOVA_MODULE_ERROR,    /**< Compile error */
} nova_module_state_t;

#ifndef NOVA_MODULE_TYPE_DEFINED
#define NOVA_MODULE_TYPE_DEFINED
typedef struct nova_module_t {
    char *name;     /**< Module name (e.g. "mymodule") */
    char *filepath; /**< Absolute or relative path to .nova/.zn file */
    nova_module_state_t state;
    nova_export_t *exports; /**< Linked list of exported symbols */
    size_t export_count;
    struct nova_module_t *next; /**< Next in registry linked list */
} nova_module_t;
#endif

/* ── Registry ────────────────────────────────────────────────────────── */

typedef struct {
    nova_module_t *modules; /**< Linked list of registered modules */
    size_t count;
    char *search_path; /**< Colon-separated directory search path */
} nova_module_registry_t;

/* ── API ─────────────────────────────────────────────────────────────────────── */

/** Create an empty module registry. */
nova_module_registry_t *nova_module_registry_create(void);

/** Destroy the registry and all its modules. */
void nova_module_registry_destroy(nova_module_registry_t *reg);

/**
 * Register a module by name and file path.
 */
nova_module_t *nova_module_registry_add(nova_module_registry_t *reg, const char *name,
                                        const char *filepath);

/**
 * Look up a module by name.
 */
nova_module_t *nova_module_registry_find(nova_module_registry_t *reg, const char *name);

/**
 * Auto-discover and register a module by name.
 */
nova_module_t *nova_module_registry_discover(nova_module_registry_t *reg, const char *name);

/**
 * Compile a module.
 */
bool nova_module_registry_compile(nova_module_registry_t *reg, nova_module_t *module);

/**
 * Add a symbol to a module's export list.
 */
void nova_module_add_export(nova_module_t *module, const char *name, nova_type_t *type, bool is_fn);

/**
 * Resolve an import into a symbol table.
 */
bool nova_module_registry_resolve_import(nova_module_registry_t *reg, void *symbol_table,
                                         void *symbol_table_add_fn, const char *module_name,
                                         const char **symbol_names, size_t symbol_count);

/**
 * Set the search path.
 */
void nova_module_registry_set_search_path(nova_module_registry_t *reg, const char *path);

#ifdef __cplusplus
}
#endif
