/**
 * Nova Package Manager - Registry Client
 */

#ifndef NOVA_PM_REGISTRY_H
#define NOVA_PM_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// PACKAGE TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *name;
  char *version;
  char *author;
  char *description;
  char *license;
  char **dependencies;
  size_t dependency_count;
  char *tarball_url;
  char *checksum;
} NovaPackage;

typedef struct {
  char *name;
  char **versions;
  size_t version_count;
  NovaPackage *latest;
} NovaPackageInfo;

// ═══════════════════════════════════════════════════════════════════════════
// REGISTRY OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

// Initialize registry client (default: registry.nova.dev)
bool pm_init(const char *registry_url);

// Search packages
NovaPackageInfo **pm_search(const char *query, size_t *count);

// Get package info
NovaPackageInfo *pm_get_info(const char *name);

// Get specific version
NovaPackage *pm_get_version(const char *name, const char *version);

// Download package tarball
bool pm_download(NovaPackage *pkg, const char *dest_path);

// Verify checksum
bool pm_verify(const char *path, const char *expected_checksum);

// ═══════════════════════════════════════════════════════════════════════════
// LOCAL OPERATIONS
// ═══════════════════════════════════════════════════════════════════════════

// Install package locally
bool pm_install(const char *name, const char *version);

// Uninstall package
bool pm_uninstall(const char *name);

// List installed packages
NovaPackage **pm_list_installed(size_t *count);

// Check if package is installed
bool pm_is_installed(const char *name);

// Get installed version
char *pm_installed_version(const char *name);

// ═══════════════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════════════

void pm_package_free(NovaPackage *pkg);
void pm_package_info_free(NovaPackageInfo *info);
void pm_shutdown(void);

#endif // NOVA_PM_REGISTRY_H
