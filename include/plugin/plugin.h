/**
 * @file plugin.h
 * @brief Nova runtime plugin system
 *
 * Provides dynamic loading and management of runtime plugins.
 * Supports native shared libraries (.so, .dylib, .dll) with
 * symbol resolution and lifecycle management.
 */

#ifndef NOVA_PLUGIN_H
#define NOVA_PLUGIN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Plugin Types
 * ======================================================================== */

/// Plugin handle
typedef struct nova_plugin nova_plugin_t;

/// Plugin information
typedef struct {
    const char *name;           /**< Plugin name */
    const char *version;        /**< Plugin version */
    const char *description;    /**< Plugin description */
    const char *author;         /**< Plugin author */
    uint32_t api_version;       /**< API version plugin was built for */
} nova_plugin_info_t;

/// Plugin initialization function signature
typedef int (*nova_plugin_init_fn)(void);

/// Plugin cleanup function signature
typedef void (*nova_plugin_cleanup_fn)(void);

/// Plugin get info function signature
typedef const nova_plugin_info_t* (*nova_plugin_get_info_fn)(void);

/* ========================================================================
 * Plugin Management
 * ======================================================================== */

/**
 * @brief Load plugin from file
 * @param path Plugin file path (.so, .dylib, .dll)
 * @return Plugin handle, or NULL on failure
 *
 * Loads a shared library and initializes the plugin.
 * The plugin must export:
 * - nova_plugin_init: Initialization function
 * - nova_plugin_cleanup: Cleanup function
 * - nova_plugin_get_info: Information function
 */
nova_plugin_t *nova_plugin_load(const char *path);

/**
 * @brief Unload plugin
 * @param plugin Plugin handle
 *
 * Calls cleanup function and unloads the shared library.
 */
void nova_plugin_unload(nova_plugin_t *plugin);

/**
 * @brief Get plugin symbol
 * @param plugin Plugin handle
 * @param symbol Symbol name
 * @return Symbol pointer, or NULL if not found
 *
 * Looks up a symbol in the plugin's shared library.
 */
void *nova_plugin_symbol(nova_plugin_t *plugin, const char *symbol);

/**
 * @brief Get plugin information
 * @param plugin Plugin handle
 * @return Plugin information, or NULL on failure
 */
const nova_plugin_info_t *nova_plugin_info(nova_plugin_t *plugin);

/**
 * @brief Check if plugin is loaded
 * @param plugin Plugin handle
 * @return true if loaded and valid
 */
bool nova_plugin_is_loaded(nova_plugin_t *plugin);

/**
 * @brief Get plugin file path
 * @param plugin Plugin handle
 * @return File path, or NULL on failure
 */
const char *nova_plugin_path(nova_plugin_t *plugin);

/* ========================================================================
 * Plugin Registry
 * ======================================================================== */

/**
 * @brief Register plugin in global registry
 * @param plugin Plugin handle
 * @param name Registry name
 * @return 0 on success, -1 on failure
 *
 * Registers a plugin under a name for later retrieval.
 */
int nova_plugin_register(nova_plugin_t *plugin, const char *name);

/**
 * @brief Unregister plugin from global registry
 * @param name Registry name
 * @return 0 on success, -1 on failure
 */
int nova_plugin_unregister(const char *name);

/**
 * @brief Get plugin from global registry
 * @param name Registry name
 * @return Plugin handle, or NULL if not found
 */
nova_plugin_t *nova_plugin_get(const char *name);

/**
 * @brief List all registered plugins
 * @param count Output: number of plugins
 * @return Array of plugin names (caller must free)
 */
char **nova_plugin_list(size_t *count);

/* ========================================================================
 * Plugin Events
 * ======================================================================== */

/**
 * @brief Plugin event types
 */
typedef enum {
    NOVA_PLUGIN_EVENT_LOADED,     /**< Plugin loaded */
    NOVA_PLUGIN_EVENT_UNLOADED,   /**< Plugin unloaded */
    NOVA_PLUGIN_EVENT_ERROR       /**< Plugin error */
} nova_plugin_event_type_t;

/// Plugin event data
typedef struct {
    nova_plugin_event_type_t type;    /**< Event type */
    nova_plugin_t *plugin;            /**< Plugin handle */
    const char *message;              /**< Event message (for errors) */
} nova_plugin_event_t;

/// Plugin event callback
typedef void (*nova_plugin_event_callback_t)(const nova_plugin_event_t *event);

/**
 * @brief Register plugin event callback
 * @param callback Event callback function
 */
void nova_plugin_set_event_callback(nova_plugin_event_callback_t callback);

/* ========================================================================
 * Plugin Dependencies
 * ======================================================================== */

/**
 * @brief Plugin dependency information
 */
typedef struct {
    const char *name;           /**< Required plugin name */
    const char *version_min;    /**< Minimum version required */
    const char *version_max;    /**< Maximum version allowed */
    bool optional;              /**< Whether dependency is optional */
} nova_plugin_dependency_t;

/**
 * @brief Check plugin dependencies
 * @param plugin Plugin handle
 * @return 0 if all dependencies satisfied, -1 otherwise
 */
int nova_plugin_check_dependencies(nova_plugin_t *plugin);

/**
 * @brief Get plugin dependencies
 * @param plugin Plugin handle
 * @param count Output: number of dependencies
 * @return Array of dependencies (caller must free)
 */
nova_plugin_dependency_t *nova_plugin_get_dependencies(nova_plugin_t *plugin, size_t *count);

/* ========================================================================
 * Error Handling
 * ======================================================================== */

/**
 * @brief Get last plugin error
 * @return Error message, or NULL if no error
 */
const char *nova_plugin_error(void);

/**
 * @brief Clear last plugin error
 */
void nova_plugin_clear_error(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_PLUGIN_H */
