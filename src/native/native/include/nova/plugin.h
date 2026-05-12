/**
 * @file plugin.h
 * @brief Nova Runtime Plugin System
 * 
 * Dynamic plugin loading, event system, and extension points for the Nova runtime.
 * Supports hot-reloading, versioning, and dependency management.
 */

#ifndef NOVA_PLUGIN_H
#define NOVA_PLUGIN_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Plugin API Version
 * ======================================================================== */

#define NOVA_PLUGIN_API_VERSION_MAJOR 1
#define NOVA_PLUGIN_API_VERSION_MINOR 0
#define NOVA_PLUGIN_API_VERSION_PATCH 0

#define NOVA_PLUGIN_API_VERSION \
    ((NOVA_PLUGIN_API_VERSION_MAJOR << 16) | \
     (NOVA_PLUGIN_API_VERSION_MINOR << 8) | \
     NOVA_PLUGIN_API_VERSION_PATCH)

/* ========================================================================
 * Plugin Types
 * ======================================================================== */

typedef struct nova_plugin nova_plugin_t;

/**
 * Plugin information structure
 * Exported by each plugin via nova_plugin_get_info()
 */
typedef struct {
    const char *name;           /**< Plugin name */
    const char *version;        /**< Semantic version (e.g., "1.0.0") */
    const char *author;         /**< Plugin author */
    const char *description;    /**< Brief description */
    uint32_t api_version;       /**< Required Nova API version */
    
    /* Optional fields */
    const char **dependencies;  /**< NULL-terminated array of dependency names */
    const char *license;        /**< License identifier (e.g., "MIT") */
    const char *homepage;       /**< URL to plugin homepage */
} nova_plugin_info_t;

/* ========================================================================
 * Plugin Lifecycle Functions
 * ======================================================================== */

/**
 * Plugin initialization function signature
 * Called when plugin is loaded
 * @return 0 on success, non-zero error code on failure
 */
typedef int (*nova_plugin_init_fn)(void);

/**
 * Plugin cleanup function signature
 * Called before plugin is unloaded
 */
typedef void (*nova_plugin_cleanup_fn)(void);

/**
 * Plugin info getter function signature
 * Must be exported by every plugin as "nova_plugin_get_info"
 */
typedef const nova_plugin_info_t* (*nova_plugin_get_info_fn)(void);

/* ========================================================================
 * Plugin Events
 * ======================================================================== */

typedef enum {
    NOVA_PLUGIN_EVENT_LOADED,      /**< Plugin was loaded */
    NOVA_PLUGIN_EVENT_INITIALIZED, /**< Plugin was initialized */
    NOVA_PLUGIN_EVENT_UNLOADING,   /**< Plugin is being unloaded */
    NOVA_PLUGIN_EVENT_UNLOADED,    /**< Plugin was unloaded */
    NOVA_PLUGIN_EVENT_ERROR,       /**< Plugin error occurred */
} nova_plugin_event_type_t;

typedef struct {
    nova_plugin_event_type_t type;
    nova_plugin_t *plugin;
    const char *message;           /**< Optional message (for errors) */
    void *user_data;
} nova_plugin_event_t;

/**
 * Plugin event callback function
 */
typedef void (*nova_plugin_event_callback_t)(const nova_plugin_event_t *event);

/* ========================================================================
 * Plugin Management API
 * ======================================================================== */

/**
 * Initialize the plugin system
 * Must be called before any other plugin functions
 * @return true on success
 */
bool nova_plugin_system_init(void);

/**
 * Shutdown the plugin system
 * Unloads all plugins and cleans up resources
 */
void nova_plugin_system_shutdown(void);

/**
 * Set event callback for plugin lifecycle events
 * @param callback Callback function (NULL to disable)
 */
void nova_plugin_set_event_callback(nova_plugin_event_callback_t callback);

/**
 * Load a plugin from file
 * @param path Path to plugin shared library (.so, .dylib, .dll)
 * @return Plugin handle or NULL on error
 */
nova_plugin_t *nova_plugin_load(const char *path);

/**
 * Unload a plugin
 * Calls cleanup function and frees resources
 * @param plugin Plugin to unload
 */
void nova_plugin_unload(nova_plugin_t *plugin);

/**
 * Reload a plugin (hot-reload)
 * @param plugin Plugin to reload
 * @return New plugin handle or NULL on error
 */
nova_plugin_t *nova_plugin_reload(nova_plugin_t *plugin);

/**
 * Get plugin information
 * @param plugin Plugin handle
 * @return Plugin info structure or NULL
 */
const nova_plugin_info_t *nova_plugin_get_info(nova_plugin_t *plugin);

/**
 * Get plugin symbol (function or variable)
 * @param plugin Plugin handle
 * @param symbol Symbol name
 * @return Pointer to symbol or NULL if not found
 */
void *nova_plugin_get_symbol(nova_plugin_t *plugin, const char *symbol);

/* ========================================================================
 * Plugin Registry
 * ======================================================================== */

/**
 * Register a plugin by name
 * Allows plugins to be found by name
 * @param name Plugin name
 * @param plugin Plugin handle
 * @return true on success
 */
bool nova_plugin_register(const char *name, nova_plugin_t *plugin);

/**
 * Unregister a plugin by name
 * @param name Plugin name
 */
void nova_plugin_unregister(const char *name);

/**
 * Find a plugin by name
 * @param name Plugin name
 * @return Plugin handle or NULL if not found
 */
nova_plugin_t *nova_plugin_find(const char *name);

/**
 * Get all loaded plugins
 * @param out_plugins Output array (can be NULL to query count)
 * @param max_count Maximum number of plugins to return
 * @return Number of plugins
 */
size_t nova_plugin_get_all(nova_plugin_t **out_plugins, size_t max_count);

/* ========================================================================
 * Error Handling
 * ======================================================================== */

/**
 * Get last error message
 * @return Error string or NULL if no error
 */
const char *nova_plugin_error(void);

/**
 * Clear last error
 */
void nova_plugin_clear_error(void);

/* ========================================================================
 * Plugin Development Macros
 * ======================================================================== */

/**
 * Define plugin metadata
 * Use in plugin implementation files
 */
#define NOVA_PLUGIN_DEFINE(name, version, author, desc) \
    static const nova_plugin_info_t _nova_plugin_info = { \
        .name = name, \
        .version = version, \
        .author = author, \
        .description = desc, \
        .api_version = NOVA_PLUGIN_API_VERSION, \
        .dependencies = NULL, \
        .license = NULL, \
        .homepage = NULL \
    }; \
    const nova_plugin_info_t* nova_plugin_get_info(void) { \
        return &_nova_plugin_info; \
    }

/**
 * Export plugin initialization function
 */
#define NOVA_PLUGIN_INIT(func) \
    int nova_plugin_init(void) { return (func)(); }

/**
 * Export plugin cleanup function
 */
#define NOVA_PLUGIN_CLEANUP(func) \
    void nova_plugin_cleanup(void) { (func)(); }

/* ========================================================================
 * Plugin Capabilities (Extension Points)
 * ======================================================================== */

/**
 * Plugin capability types
 * Plugins can register capabilities to extend Nova functionality
 */
typedef enum {
    NOVA_PLUGIN_CAP_BACKEND,        /**< Custom backend (GPU, accelerator) */
    NOVA_PLUGIN_CAP_OPTIMIZER,      /**< Custom optimizer pass */
    NOVA_PLUGIN_CAP_PROFILER,       /**< Custom profiler */
    NOVA_PLUGIN_CAP_ALLOCATOR,      /**< Custom memory allocator */
    NOVA_PLUGIN_CAP_KERNEL,         /**< Custom kernel implementation */
    NOVA_PLUGIN_CAP_SERIALIZER,     /**< Custom serialization format */
    NOVA_PLUGIN_CAP_TRANSFORM,      /**< Custom IR transformation */
} nova_plugin_capability_type_t;

/**
 * Register a plugin capability
 * @param plugin Plugin handle
 * @param type Capability type
 * @param implementation Pointer to implementation structure
 * @return true on success
 */
bool nova_plugin_register_capability(
    nova_plugin_t *plugin,
    nova_plugin_capability_type_t type,
    void *implementation
);

/**
 * Find plugins with specific capability
 * @param type Capability type
 * @param out_plugins Output array
 * @param max_count Maximum number of plugins
 * @return Number of matching plugins
 */
size_t nova_plugin_find_by_capability(
    nova_plugin_capability_type_t type,
    nova_plugin_t **out_plugins,
    size_t max_count
);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_PLUGIN_H */
