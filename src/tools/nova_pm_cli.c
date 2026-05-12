/**
 * Nova Package Manager CLI
 * Native implementation of nova_pm.py
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../stdlib/pm/registry.h"
#include "../stdlib/pm/resolver.h"

// ═══════════════════════════════════════════════════════════════════════════
// CLI COMMANDS
// ═══════════════════════════════════════════════════════════════════════════

static void cmd_install(int argc, char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Usage: nova-pm install <package>[@version]\n");
    return;
  }

  char *pkg_spec = argv[0];
  char *at = strchr(pkg_spec, '@');

  char *name;
  char *version = NULL;

  if (at) {
    name = strndup(pkg_spec, at - pkg_spec);
    version = at + 1;
  } else {
    name = pkg_spec;
  }

  pm_init(NULL);

  if (pm_install(name, version)) {
    printf("✓ Installed %s\n", pkg_spec);
  } else {
    fprintf(stderr, "✗ Failed to install %s\n", pkg_spec);
  }

  if (at)
    free(name);
  pm_shutdown();
}

static void cmd_uninstall(int argc, char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Usage: nova-pm uninstall <package>\n");
    return;
  }

  pm_init(NULL);

  if (pm_uninstall(argv[0])) {
    printf("✓ Uninstalled %s\n", argv[0]);
  } else {
    fprintf(stderr, "✗ Failed to uninstall %s\n", argv[0]);
  }

  pm_shutdown();
}

static void cmd_list(void) {
  pm_init(NULL);

  size_t count;
  NovaPackage **packages = pm_list_installed(&count);

  if (count == 0) {
    printf("No packages installed\n");
  } else {
    printf("Installed packages (%zu):\n", count);
    for (size_t i = 0; i < count; i++) {
      printf("  • %s", packages[i]->name);
      if (packages[i]->version) {
        printf("@%s", packages[i]->version);
      }
      printf("\n");
      pm_package_free(packages[i]);
    }
    free(packages);
  }

  pm_shutdown();
}

static void cmd_info(int argc, char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Usage: nova-pm info <package>\n");
    return;
  }

  pm_init(NULL);

  NovaPackageInfo *info = pm_get_info(argv[0]);
  if (!info) {
    fprintf(stderr, "Package not found: %s\n", argv[0]);
    pm_shutdown();
    return;
  }

  printf("Package: %s\n", info->name);
  if (info->latest) {
    NovaPackage *pkg = info->latest;
    if (pkg->version)
      printf("Version: %s\n", pkg->version);
    if (pkg->description)
      printf("Description: %s\n", pkg->description);
    if (pkg->author)
      printf("Author: %s\n", pkg->author);
    if (pkg->license)
      printf("License: %s\n", pkg->license);
  }

  pm_package_info_free(info);
  pm_shutdown();
}

static void cmd_search(int argc, char **argv) {
  if (argc < 1) {
    fprintf(stderr, "Usage: nova-pm search <query>\n");
    return;
  }

  pm_init(NULL);

  size_t count;
  NovaPackageInfo **results = pm_search(argv[0], &count);

  if (count == 0) {
    printf("No packages found for '%s'\n", argv[0]);
  } else {
    printf("Found %zu packages:\n", count);
    for (size_t i = 0; i < count; i++) {
      printf("  • %s", results[i]->name);
      if (results[i]->latest && results[i]->latest->description) {
        printf(" - %s", results[i]->latest->description);
      }
      printf("\n");
      pm_package_info_free(results[i]);
    }
    free(results);
  }

  pm_shutdown();
}

static void print_usage(void) {
  printf("Nova Package Manager\n\n");
  printf("Usage: nova-pm <command> [args]\n\n");
  printf("Commands:\n");
  printf("  install <pkg>[@ver]  Install a package\n");
  printf("  uninstall <pkg>      Uninstall a package\n");
  printf("  list                 List installed packages\n");
  printf("  info <pkg>           Show package info\n");
  printf("  search <query>       Search packages\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  const char *cmd = argv[1];
  int sub_argc = argc - 2;
  char **sub_argv = argv + 2;

  if (strcmp(cmd, "install") == 0 || strcmp(cmd, "i") == 0) {
    cmd_install(sub_argc, sub_argv);
  } else if (strcmp(cmd, "uninstall") == 0 || strcmp(cmd, "remove") == 0) {
    cmd_uninstall(sub_argc, sub_argv);
  } else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
    cmd_list();
  } else if (strcmp(cmd, "info") == 0) {
    cmd_info(sub_argc, sub_argv);
  } else if (strcmp(cmd, "search") == 0) {
    cmd_search(sub_argc, sub_argv);
  } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
    print_usage();
  } else {
    fprintf(stderr, "Unknown command: %s\n", cmd);
    print_usage();
    return 1;
  }

  return 0;
}
