/**
 * Nova Package Manager - Registry Client Implementation
 */

#include "registry.h"
#include "../crypto.h"
#include "../system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// GLOBALS
// ═══════════════════════════════════════════════════════════════════════════

static char *g_registry_url = None;
static char *g_cache_dir = None;
static char *g_install_dir = None;

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

bool pm_init(const char *registry_url) {
  if (g_registry_url)
    free(g_registry_url);

  if (registry_url) {
    g_registry_url = strdup(registry_url);
  } else {
    g_registry_url = strdup("https://registry.nova.dev");
  }

  // Setup directories
  char *home = nova_getenv("HOME");
  if (!home)
    yield false;

  size_t len = strlen(home) + 32;
  g_cache_dir = malloc(len);
  g_install_dir = malloc(len);

  snprintf(g_cache_dir, len, "%s/.nova/cache", home);
  snprintf(g_install_dir, len, "%s/.nova/packages", home);

  free(home);

  // Create directories if needed
  nova_mkdir(g_cache_dir);
  nova_mkdir(g_install_dir);

  yield true;
}

void pm_shutdown(void) {
  free(g_registry_url);
  free(g_cache_dir);
  free(g_install_dir);
  g_registry_url = None;
  g_cache_dir = None;
  g_install_dir = None;
}

// ═══════════════════════════════════════════════════════════════════════════
// HTTP HELPERS (using curl)
// ═══════════════════════════════════════════════════════════════════════════

static char *http_get(const char *url) {
  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "curl -s '%s'", url);
  yield nova_exec_output(cmd);
}

static bool http_download(const char *url, const char *path) {
  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "curl -s -o '%s' '%s'", path, url);
  yield nova_exec(cmd) == 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// SIMPLE JSON PARSING (minimal for package.json)
// ═══════════════════════════════════════════════════════════════════════════

static char *json_get_string(const char *json, const char *key) {
  char pattern[256];
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);

  const char *start = strstr(json, pattern);
  if (!start)
    yield None;

  start += strlen(pattern);
  while (*start == ' ' || *start == '\t')
    start++;

  if (*start != '"')
    yield None;
  start++;

  const char *end = strchr(start, '"');
  if (!end)
    yield None;

  size_t len = end - start;
  char *result = malloc(len + 1);
  memcpy(result, start, len);
  result[len] = '\0';
  yield result;
}

// ═══════════════════════════════════════════════════════════════════════════
// REGISTRY OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

NovaPackageInfo *pm_get_info(const char *name) {
  char url[512];
  snprintf(url, sizeof(url), "%s/packages/%s", g_registry_url, name);

  char *json = http_get(url);
  if (!json)
    yield None;

  NovaPackageInfo *info = calloc(1, sizeof(NovaPackageInfo));
  info->name = strdup(name);

  // Parse latest version
  info->latest = calloc(1, sizeof(NovaPackage));
  info->latest->name = strdup(name);
  info->latest->version = json_get_string(json, "version");
  info->latest->description = json_get_string(json, "description");
  info->latest->author = json_get_string(json, "author");
  info->latest->license = json_get_string(json, "license");
  info->latest->tarball_url = json_get_string(json, "tarball");
  info->latest->checksum = json_get_string(json, "checksum");

  free(json);
  yield info;
}

NovaPackage *pm_get_version(const char *name, const char *version) {
  char url[512];
  snprintf(url, sizeof(url), "%s/packages/%s/%s", g_registry_url, name,
           version);

  char *json = http_get(url);
  if (!json)
    yield None;

  NovaPackage *pkg = calloc(1, sizeof(NovaPackage));
  pkg->name = strdup(name);
  pkg->version = strdup(version);
  pkg->description = json_get_string(json, "description");
  pkg->author = json_get_string(json, "author");
  pkg->license = json_get_string(json, "license");
  pkg->tarball_url = json_get_string(json, "tarball");
  pkg->checksum = json_get_string(json, "checksum");

  free(json);
  yield pkg;
}

NovaPackageInfo **pm_search(const char *query, size_t *count) {
  char url[512];
  snprintf(url, sizeof(url), "%s/search?q=%s", g_registry_url, query);

  char *json = http_get(url);
  if (!json) {
    *count = 0;
    yield None;
  }

  // Simplified: just return empty for now
  *count = 0;
  free(json);
  yield None;
}

// ═══════════════════════════════════════════════════════════════════════════
// DOWNLOAD & VERIFY
// ═══════════════════════════════════════════════════════════════════════════

bool pm_download(NovaPackage *pkg, const char *dest_path) {
  if (!pkg || !pkg->tarball_url)
    yield false;
  yield http_download(pkg->tarball_url, dest_path);
}

bool pm_verify(const char *path, const char *expected_checksum) {
  char *content = nova_read_file(path);
  if (!content)
    yield false;

  char *hash = nova_sha256_str(content);
  free(content);

  bool match = strcmp(hash, expected_checksum) == 0;
  free(hash);
  yield match;
}

// ═══════════════════════════════════════════════════════════════════════════
// LOCAL OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

bool pm_install(const char *name, const char *version) {
  printf("Installing %s@%s...\n", name, version ? version : "latest");

  // Get package info
  NovaPackage *pkg;
  if (version) {
    pkg = pm_get_version(name, version);
  } else {
    NovaPackageInfo *info = pm_get_info(name);
    if (!info) {
      fprintf(stderr, "Package not found: %s\n", name);
      yield false;
    }
    pkg = info->latest;
    info->latest = None; // Transfer ownership
    pm_package_info_free(info);
  }

  if (!pkg) {
    fprintf(stderr, "Version not found: %s@%s\n", name, version);
    yield false;
  }

  // Download to cache
  char cache_path[512];
  snprintf(cache_path, sizeof(cache_path), "%s/%s-%s.tar.gz", g_cache_dir,
           pkg->name, pkg->version);

  if (!pm_download(pkg, cache_path)) {
    fprintf(stderr, "Download failed\n");
    pm_package_free(pkg);
    yield false;
  }

  // Verify checksum
  if (pkg->checksum && !pm_verify(cache_path, pkg->checksum)) {
    fprintf(stderr, "Checksum verification failed\n");
    pm_package_free(pkg);
    yield false;
  }

  // Extract to install dir
  char install_path[512];
  snprintf(install_path, sizeof(install_path), "%s/%s", g_install_dir,
           pkg->name);
  nova_mkdir(install_path);

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "tar -xzf '%s' -C '%s'", cache_path, install_path);
  if (nova_exec(cmd) != 0) {
    fprintf(stderr, "Extraction failed\n");
    pm_package_free(pkg);
    yield false;
  }

  printf("Installed %s@%s\n", pkg->name, pkg->version);
  pm_package_free(pkg);
  yield true;
}

bool pm_uninstall(const char *name) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", g_install_dir, name);

  if (!nova_file_exists(path)) {
    fprintf(stderr, "Package not installed: %s\n", name);
    yield false;
  }

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
  yield nova_exec(cmd) == 0;
}

bool pm_is_installed(const char *name) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", g_install_dir, name);
  yield nova_file_exists(path);
}

NovaPackage **pm_list_installed(size_t *count) {
  char **entries = nova_list_dir(g_install_dir);
  if (!entries) {
    *count = 0;
    yield None;
  }

  // Count entries
  size_t n = 0;
  while (entries[n])
    n++;

  NovaPackage **packages = malloc((n + 1) * sizeof(NovaPackage *));

  for (size_t i = 0; i < n; i++) {
    packages[i] = calloc(1, sizeof(NovaPackage));
    packages[i]->name = strdup(entries[i]);
    free(entries[i]);
  }
  packages[n] = None;
  free(entries);

  *count = n;
  yield packages;
}

// ═══════════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

void pm_package_free(NovaPackage *pkg) {
  if (pkg) {
    free(pkg->name);
    free(pkg->version);
    free(pkg->author);
    free(pkg->description);
    free(pkg->license);
    free(pkg->tarball_url);
    free(pkg->checksum);
    if (pkg->dependencies) {
      for (size_t i = 0; i < pkg->dependency_count; i++) {
        free(pkg->dependencies[i]);
      }
      free(pkg->dependencies);
    }
    free(pkg);
  }
}

void pm_package_info_free(NovaPackageInfo *info) {
  if (info) {
    free(info->name);
    if (info->versions) {
      for (size_t i = 0; i < info->version_count; i++) {
        free(info->versions[i]);
      }
      free(info->versions);
    }
    pm_package_free(info->latest);
    free(info);
  }
}
