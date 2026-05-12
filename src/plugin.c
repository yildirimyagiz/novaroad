/**
 * @file plugin.c
 * @brief Nova runtime plugin system implementation
 */

#include "plugin/plugin.h"
#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ========================================================================
 * Internal Structures
 * ======================================================================== */

struct nova_plugin {
  void *handle;                      /**< dlopen handle */
  char *path;                        /**< Plugin file path */
  nova_plugin_info_t *info;          /**< Plugin information */
  nova_plugin_init_fn init_fn;       /**< Init function */
  nova_plugin_cleanup_fn cleanup_fn; /**< Cleanup function */
  bool initialized;                  /**< Whether plugin was initialized */
  bool unloading;                    /**< Whether plugin is being unloaded */
};

/* Global state */
static pthread_mutex_t plugin_mutex = PTHREAD_MUTEX_INITIALIZER;
static nova_plugin_event_callback_t event_callback = NULL;
static char *last_error = NULL;

/* Plugin registry */
#define MAX_PLUGINS 256
static struct {
  char *names[MAX_PLUGINS];
  nova_plugin_t *plugins[MAX_PLUGINS];
  size_t count;
} plugin_registry;

/* ========================================================================
 * Error Handling
 * ======================================================================== */

static void set_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  free(last_error);
  vasprintf(&last_error, fmt, args);

  va_end(args);
}

const char *nova_plugin_error(void) { return last_error; }

void nova_plugin_clear_error(void) {
  free(last_error);
  last_error = NULL;
}

/* ========================================================================
 * Plugin Loading/Unloading
 * ======================================================================== */

nova_plugin_t *nova_plugin_load(const char *path) {
  if (!path) {
    set_error("Plugin path is NULL");
    return NULL;
  }

  /* Check if file exists */
  struct stat st;
  if (stat(path, &st) != 0) {
    set_error("Plugin file does not exist: %s", path);
    return NULL;
  }

  /* Check if it's a regular file */
  if (!S_ISREG(st.st_mode)) {
    set_error("Plugin path is not a regular file: %s", path);
    return NULL;
  }

  /* Allocate plugin structure */
  nova_plugin_t *plugin = calloc(1, sizeof(nova_plugin_t));
  if (!plugin) {
    set_error("Failed to allocate plugin structure");
    return NULL;
  }

  /* Store path */
  plugin->path = strdup(path);
  if (!plugin->path) {
    set_error("Failed to duplicate plugin path");
    free(plugin);
    return NULL;
  }

  /* Load shared library */
  plugin->handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
  if (!plugin->handle) {
    set_error("Failed to load plugin: %s", dlerror());
    free(plugin->path);
    free(plugin);
    return NULL;
  }

  /* Clear any existing dlerror */
  dlerror();

  /* Get plugin functions */
  plugin->init_fn = dlsym(plugin->handle, "nova_plugin_init");
  const char *err = dlerror();
  if (err) {
    plugin->init_fn = NULL; /* Optional */
  }

  plugin->cleanup_fn = dlsym(plugin->handle, "nova_plugin_cleanup");
  err = dlerror();
  if (err) {
    plugin->cleanup_fn = NULL; /* Optional */
  }

  nova_plugin_get_info_fn get_info_fn =
      dlsym(plugin->handle, "nova_plugin_get_info");
  err = dlerror();
  if (!err && get_info_fn) {
    plugin->info = (nova_plugin_info_t *)get_info_fn();
  }

  /* Initialize plugin if init function exists */
  if (plugin->init_fn) {
    int result = plugin->init_fn();
    if (result != 0) {
      set_error("Plugin initialization failed with code %d", result);
      dlclose(plugin->handle);
      free(plugin->path);
      free(plugin);
      return NULL;
    }
    plugin->initialized = true;
  }

  /* Fire loaded event */
  if (event_callback) {
    nova_plugin_event_t event = {
        .type = NOVA_PLUGIN_EVENT_LOADED, .plugin = plugin, .message = NULL};
    event_callback(&event);
  }

  return plugin;
}

void nova_plugin_unload(nova_plugin_t *plugin) {
  if (!plugin)
    return;

  /* Prevent double unloading */
  if (plugin->unloading)
    return;
  plugin->unloading = true;

  /* Call cleanup function if plugin was initialized */
  if (plugin->initialized && plugin->cleanup_fn) {
    plugin->cleanup_fn();
  }

  /* Fire unloaded event */
  if (event_callback) {
    nova_plugin_event_t event = {
        .type = NOVA_PLUGIN_EVENT_UNLOADED, .plugin = plugin, .message = NULL};
    event_callback(&event);
  }

  /* Close shared library */
  if (plugin->handle) {
    dlclose(plugin->handle);
  }

  /* Free resources */
  free(plugin->path);
  free(plugin);
}

void *nova_plugin_symbol(nova_plugin_t *plugin, const char *symbol) {
  if (!plugin || !plugin->handle || !symbol) {
    set_error("Invalid plugin or symbol");
    return NULL;
  }

  /* Clear dlerror */
  dlerror();

  /* Get symbol */
  void *ptr = dlsym(plugin->handle, symbol);
  const char *err = dlerror();
  if (err) {
    set_error("Failed to get symbol '%s': %s", symbol, err);
    return NULL;
  }

  return ptr;
}

const nova_plugin_info_t *nova_plugin_info(nova_plugin_t *plugin) {
  return plugin ? plugin->info : NULL;
}

bool nova_plugin_is_loaded(nova_plugin_t *plugin) {
  return plugin && plugin->handle && !plugin->unloading;
}

const char *nova_plugin_path(nova_plugin_t *plugin) {
  return plugin ? plugin->path : NULL;
}

/* ========================================================================
 * Plugin Registry
 * ======================================================================== */

int nova_plugin_register(nova_plugin_t *plugin, const char *name) {
  if (!plugin || !name) {
    set_error("Invalid plugin or name");
    return -1;
  }

  pthread_mutex_lock(&plugin_mutex);

  /* Check if name already exists */
  for (size_t i = 0; i < plugin_registry.count; i++) {
    if (strcmp(plugin_registry.names[i], name) == 0) {
      pthread_mutex_unlock(&plugin_mutex);
      set_error("Plugin name already registered: %s", name);
      return -1;
    }
  }

  /* Check if we have space */
  if (plugin_registry.count >= MAX_PLUGINS) {
    pthread_mutex_unlock(&plugin_mutex);
    set_error("Plugin registry full");
    return -1;
  }

  /* Add to registry */
  plugin_registry.names[plugin_registry.count] = strdup(name);
  if (!plugin_registry.names[plugin_registry.count]) {
    pthread_mutex_unlock(&plugin_mutex);
    set_error("Failed to duplicate plugin name");
    return -1;
  }

  plugin_registry.plugins[plugin_registry.count] = plugin;
  plugin_registry.count++;

  pthread_mutex_unlock(&plugin_mutex);
  return 0;
}

int nova_plugin_unregister(const char *name) {
  if (!name) {
    set_error("Invalid name");
    return -1;
  }

  pthread_mutex_lock(&plugin_mutex);

  /* Find plugin */
  for (size_t i = 0; i < plugin_registry.count; i++) {
    if (strcmp(plugin_registry.names[i], name) == 0) {
      /* Remove from registry */
      free(plugin_registry.names[i]);
      for (size_t j = i; j < plugin_registry.count - 1; j++) {
        plugin_registry.names[j] = plugin_registry.names[j + 1];
        plugin_registry.plugins[j] = plugin_registry.plugins[j + 1];
      }
      plugin_registry.count--;
      pthread_mutex_unlock(&plugin_mutex);
      return 0;
    }
  }

  pthread_mutex_unlock(&plugin_mutex);
  set_error("Plugin not found: %s", name);
  return -1;
}

nova_plugin_t *nova_plugin_get(const char *name) {
  if (!name)
    return NULL;

  pthread_mutex_lock(&plugin_mutex);

  for (size_t i = 0; i < plugin_registry.count; i++) {
    if (strcmp(plugin_registry.names[i], name) == 0) {
      nova_plugin_t *plugin = plugin_registry.plugins[i];
      pthread_mutex_unlock(&plugin_mutex);
      return plugin;
    }
  }

  pthread_mutex_unlock(&plugin_mutex);
  return NULL;
}

char **nova_plugin_list(size_t *count) {
  if (!count)
    return NULL;

  pthread_mutex_lock(&plugin_mutex);

  *count = plugin_registry.count;
  if (plugin_registry.count == 0) {
    pthread_mutex_unlock(&plugin_mutex);
    return NULL;
  }

  char **names = malloc(plugin_registry.count * sizeof(char *));
  if (!names) {
    pthread_mutex_unlock(&plugin_mutex);
    *count = 0;
    return NULL;
  }

  for (size_t i = 0; i < plugin_registry.count; i++) {
    names[i] = strdup(plugin_registry.names[i]);
    if (!names[i]) {
      /* Clean up on failure */
      for (size_t j = 0; j < i; j++) {
        free(names[j]);
      }
      free(names);
      pthread_mutex_unlock(&plugin_mutex);
      *count = 0;
      return NULL;
    }
  }

  pthread_mutex_unlock(&plugin_mutex);
  return names;
}

/* ========================================================================
 * Plugin Events
 * ======================================================================== */

void nova_plugin_set_event_callback(nova_plugin_event_callback_t callback) {
  event_callback = callback;
}

/* ========================================================================
 * Plugin Dependencies (Stub Implementation)
 * ======================================================================== */

int nova_plugin_check_dependencies(nova_plugin_t *plugin) {
  /* TODO: Implement dependency checking */
  (void)plugin;
  return 0;
}

nova_plugin_dependency_t *nova_plugin_get_dependencies(nova_plugin_t *plugin,
                                                       size_t *count) {
  /* TODO: Implement dependency enumeration */
  (void)plugin;
  if (count)
    *count = 0;
  return NULL;
}
