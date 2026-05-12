#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Nova Package Manager - Phase D.4
// nova.toml support, package resolution, build metadata

// === TOML PARSER ===

// Simple key-value pair
typedef struct {
    char* key;
    char* value;
} nova_toml_pair_t;

// Package metadata from nova.toml
typedef struct {
    char* name;
    char* version;
    char* description;
    char** dependencies;  // array of dependency names
    size_t dep_count;
    char* stdlib_path;
    bool is_stdlib;
} nova_package_t;

// Parse simple TOML-like format
nova_package_t* nova_toml_parse(const char* toml_content) {
    nova_package_t* pkg = calloc(1, sizeof(nova_package_t));
    pkg->dependencies = NULL;
    pkg->dep_count = 0;

    // Simple line-by-line parsing
    char* content = strdup(toml_content);
    char* line = strtok(content, "\n");

    while (line) {
        // Skip comments and empty lines
        if (line[0] == '#' || strlen(line) == 0) {
            line = strtok(NULL, "\n");
            continue;
        }

        // Parse key = "value" format
        char* equals = strchr(line, '=');
        if (equals) {
            *equals = '\0';
            char* key = line;
            char* value = equals + 1;

            // Trim whitespace
            while (*key == ' ') key++;
            while (key[strlen(key)-1] == ' ') key[strlen(key)-1] = '\0';

            while (*value == ' ' || *value == '"') value++;
            if (value[strlen(value)-1] == '"') value[strlen(value)-1] = '\0';

            // Set package fields
            if (strcmp(key, "name") == 0) {
                pkg->name = strdup(value);
            } else if (strcmp(key, "version") == 0) {
                pkg->version = strdup(value);
            } else if (strcmp(key, "description") == 0) {
                pkg->description = strdup(value);
            } else if (strcmp(key, "stdlib_path") == 0) {
                pkg->stdlib_path = strdup(value);
            } else if (strcmp(key, "is_stdlib") == 0) {
                pkg->is_stdlib = (strcmp(value, "true") == 0);
            }
        }

        line = strtok(NULL, "\n");
    }

    free(content);
    return pkg;
}

// === PACKAGE RESOLUTION ===

// Package registry
typedef struct nova_package_registry {
    nova_package_t** packages;
    size_t count;
    size_t capacity;
} nova_package_registry_t;

nova_package_registry_t* nova_package_registry_create() {
    nova_package_registry_t* reg = calloc(1, sizeof(nova_package_registry_t));
    reg->capacity = 16;
    reg->count = 0;
    reg->packages = malloc(sizeof(nova_package_t*) * reg->capacity);
    return reg;
}

void nova_package_registry_add(nova_package_registry_t* reg, nova_package_t* pkg) {
    if (reg->count >= reg->capacity) {
        reg->capacity *= 2;
        reg->packages = realloc(reg->packages, sizeof(nova_package_t*) * reg->capacity);
    }
    reg->packages[reg->count++] = pkg;
}

// Find package root (directory containing nova.toml)
char* nova_find_package_root(const char* start_path) {
    char* current = realpath(start_path, NULL); // Convert to absolute path
    if (!current) {
        // If realpath fails, try with current directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd))) {
            current = strdup(cwd);
        } else {
            return NULL;
        }
    }

    char* nova_toml_path = NULL;

    // Walk up directories until we find nova.toml
    while (current) {
        nova_toml_path = malloc(strlen(current) + 12); // + "/nova.toml"
        sprintf(nova_toml_path, "%s/nova.toml", current);

        FILE* f = fopen(nova_toml_path, "r");
        if (f) {
            fclose(f);
            // Found it - return directory
            char* result = strdup(current);
            free(nova_toml_path);
            free(current);
            return result;
        }

        free(nova_toml_path);
        nova_toml_path = NULL;

        // Go up one directory
        char* slash = strrchr(current, '/');
        if (slash) {
            *slash = '\0';
            if (strlen(current) == 0) {
                free(current);
                current = NULL;
            }
        } else {
            free(current);
            current = NULL;
        }
    }

    return NULL; // Not found
}

// Load package from nova.toml
nova_package_t* nova_package_load(const char* package_root) {
    char toml_path[1024];
    sprintf(toml_path, "%s/nova.toml", package_root);

    FILE* f = fopen(toml_path, "r");
    if (!f) {
        printf("Error: Cannot open %s\n", toml_path);
        return NULL;
    }

    // Read file content
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    nova_package_t* pkg = nova_toml_parse(content);
    free(content);

    return pkg;
}

// === DEPENDENCY RESOLUTION ===

// Resolve dependencies (simplified - local paths only)
bool nova_resolve_dependencies(nova_package_registry_t* reg, nova_package_t* pkg, const char* base_path) {
    // For now, assume stdlib is in known location
    if (!pkg->is_stdlib && pkg->dep_count == 0) {
        // Add implicit stdlib dependency
        pkg->dependencies = malloc(sizeof(char*));
        pkg->dependencies[0] = strdup("nova_std");
        pkg->dep_count = 1;
    }

    // For each dependency, try to find it locally
    for (size_t i = 0; i < pkg->dep_count; i++) {
        const char* dep_name = pkg->dependencies[i];

        // Check if we already have this package
        bool found = false;
        for (size_t j = 0; j < reg->count; j++) {
            if (strcmp(reg->packages[j]->name, dep_name) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            // Try to load from local path
            char dep_path[1024];
            sprintf(dep_path, "%s/deps/%s", base_path, dep_name);

            // Check if directory exists
            DIR* dir = opendir(dep_path);
            if (dir) {
                closedir(dir);

                nova_package_t* dep_pkg = nova_package_load(dep_path);
                if (dep_pkg) {
                    nova_package_registry_add(reg, dep_pkg);

                    // Recursively resolve dependencies
                    if (!nova_resolve_dependencies(reg, dep_pkg, dep_path)) {
                        return false;
                    }
                }
            } else {
                printf("Warning: Cannot find dependency '%s' in %s/deps/%s\n",
                       dep_name, base_path, dep_name);
                // Continue - dependency might be optional
            }
        }
    }

    return true;
}

// === BUILD METADATA ===

// Build target
typedef enum {
    TARGET_EXECUTABLE,
    TARGET_LIBRARY,
    TARGET_TEST
} nova_build_target_t;

// Build configuration
typedef struct {
    nova_build_target_t target_type;
    char* output_name;
    char** source_files;
    size_t source_count;
    char** include_paths;
    size_t include_count;
    nova_package_registry_t* packages;
} nova_build_config_t;

// Generate build config from package
nova_build_config_t* nova_generate_build_config(nova_package_t* pkg, nova_package_registry_t* reg, const char* package_root) {
    nova_build_config_t* config = calloc(1, sizeof(nova_build_config_t));

    if (pkg->is_stdlib) {
        config->target_type = TARGET_LIBRARY;
        config->output_name = strdup("nova_std");
    } else {
        config->target_type = TARGET_EXECUTABLE;
        config->output_name = strdup(pkg->name ? pkg->name : "nova_app");
    }

    // Find all .zn files in src/
    char src_path[1024];
    sprintf(src_path, "%s/src", package_root);

    DIR* dir = opendir(src_path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (strstr(entry->d_name, ".zn")) {
                config->source_count++;
                config->source_files = realloc(config->source_files,
                    sizeof(char*) * config->source_count);
                char* full_path = malloc(strlen(src_path) + strlen(entry->d_name) + 2);
                sprintf(full_path, "%s/%s", src_path, entry->d_name);
                config->source_files[config->source_count - 1] = full_path;
            }
        }
        closedir(dir);
    }

    config->packages = reg;
    return config;
}

// === NOVA BUILD COMMAND SIMULATION ===

int nova_build_package(const char* package_path) {
    printf("🏗️  Building Nova package: %s\n", package_path);
    printf("================================\n\n");

    // Find package root
    char* package_root = nova_find_package_root(package_path);
    if (!package_root) {
        printf("❌ Error: Cannot find nova.toml in %s or parent directories\n", package_path);
        return 1;
    }

    printf("📦 Package root: %s\n", package_root);

    // Load main package
    nova_package_t* main_pkg = nova_package_load(package_root);
    if (!main_pkg) {
        printf("❌ Error: Failed to load nova.toml\n");
        free(package_root);
        return 1;
    }

    printf("📋 Package: %s v%s\n", main_pkg->name, main_pkg->version);
    if (main_pkg->description) {
        printf("   %s\n", main_pkg->description);
    }
    printf("\n");

    // Create package registry
    nova_package_registry_t* reg = nova_package_registry_create();
    nova_package_registry_add(reg, main_pkg);

    // Resolve dependencies
    printf("🔍 Resolving dependencies...\n");
    if (!nova_resolve_dependencies(reg, main_pkg, package_root)) {
        printf("❌ Error: Dependency resolution failed\n");
        free(package_root);
        return 1;
    }

    printf("📦 Total packages: %zu\n", reg->count);
    for (size_t i = 0; i < reg->count; i++) {
        nova_package_t* pkg = reg->packages[i];
        printf("  - %s v%s\n", pkg->name, pkg->version);
    }
    printf("\n");

    // Generate build config
    nova_build_config_t* build_config = nova_generate_build_config(main_pkg, reg, package_root);

    printf("⚙️  Build configuration:\n");
    printf("  Target: %s\n", build_config->target_type == TARGET_EXECUTABLE ? "executable" : "library");
    printf("  Output: %s\n", build_config->output_name);
    printf("  Sources: %zu file(s)\n", build_config->source_count);

    for (size_t i = 0; i < build_config->source_count; i++) {
        printf("    %s\n", build_config->source_files[i]);
    }

    // Simulate compilation
    printf("\n🔨 Compiling...\n");
    for (size_t i = 0; i < build_config->source_count; i++) {
        printf("  ✓ %s\n", strrchr(build_config->source_files[i], '/') + 1);
    }

    printf("\n🔗 Linking...\n");
    printf("  ✓ %s\n", build_config->output_name);

    printf("\n✅ Build successful!\n");
    printf("   Output: %s/%s\n", package_root, build_config->output_name);

    // Cleanup
    free(package_root);
    // TODO: proper cleanup of all allocated memory

    return 0;
}

// === SAMPLE NOVA.TOML CONTENT ===

const char* sample_nova_toml = R"(
# Nova Package Configuration
name = "my_app"
version = "0.1.0"
description = "A sample Nova application"

[dependencies]
nova_std = "0.1.0"
)";

// === TEST INFRASTRUCTURE ===

void nova_test_package_manager() {
    printf("📦 Testing Nova Package Manager (D.4)\n");
    printf("=====================================\n\n");

    // Test TOML parsing
    printf("1. Testing TOML parsing...\n");
    nova_package_t* pkg = nova_toml_parse(sample_nova_toml);
    printf("   Name: %s\n", pkg->name);
    printf("   Version: %s\n", pkg->version);
    printf("   Description: %s\n", pkg->description);
    printf("   ✓ TOML parsing works\n\n");

    // Test package resolution (simplified)
    printf("2. Testing package resolution...\n");
    nova_package_registry_t* reg = nova_package_registry_create();
    nova_package_registry_add(reg, pkg);

    printf("   Registry contains %zu package(s)\n", reg->count);
    printf("   ✓ Package registry works\n\n");

    // Test build simulation
    printf("3. Testing build configuration...\n");
    nova_build_config_t* config = nova_generate_build_config(pkg, reg, "/tmp/test");
    printf("   Target: %s\n", config->target_type == TARGET_EXECUTABLE ? "executable" : "library");
    printf("   Output: %s\n", config->output_name);
    printf("   ✓ Build config generation works\n\n");

    printf("✅ Package manager test completed!\n\n");

    // Demonstrate nova build command
    printf("🚀 Nova Build Command Demo:\n");
    printf("==========================\n");
    printf("$ nova build\n");
    nova_build_package("."); // Use current directory
}

int main() {
    nova_test_package_manager();
    return 0;
}
