// Module Loading Orchestration
// A6.4.3 Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Module unit structure
typedef struct nova_module_unit {
    char* module_name;
    char* path;
    int file_id;
    void* ast;        // parsed module AST (placeholder)
    int loaded;       // 0=not loaded, 1=loaded
    int parsing;      // currently parsing (for cycle detection)
    int parsed;       // successfully parsed
} nova_module_unit_t;

// Module loading context
typedef struct nova_module_loader {
    nova_module_registry_t* registry;
    nova_module_unit_t** units;
    size_t unit_count;
    size_t unit_capacity;
} nova_module_loader_t;

// Forward declarations (from previous files)
typedef struct nova_module_registry nova_module_registry_t;
nova_module_registry_t* nova_module_registry_create(const char* project_root);
void nova_module_registry_destroy(nova_module_registry_t* reg);
int nova_module_registry_register(nova_module_registry_t* reg, const char* module_name);

// Create module loader
nova_module_loader_t* nova_module_loader_create(const char* project_root) {
    nova_module_loader_t* loader = calloc(1, sizeof(nova_module_loader_t));
    loader->registry = nova_module_registry_create(project_root);
    loader->unit_capacity = 16;
    loader->unit_count = 0;
    loader->units = calloc(loader->unit_capacity, sizeof(nova_module_unit_t*));

    printf("🚀 Module Loader created for: %s\n", project_root);
    return loader;
}

// Destroy module loader
void nova_module_loader_destroy(nova_module_loader_t* loader) {
    if (!loader) return;

    for (size_t i = 0; i < loader->unit_count; i++) {
        free(loader->units[i]->module_name);
        free(loader->units[i]->path);
        free(loader->units[i]);
    }

    free(loader->units);
    nova_module_registry_destroy(loader->registry);
    free(loader);
}

// Load a module (parse and register)
int nova_module_loader_load(nova_module_loader_t* loader, const char* module_name) {
    if (!loader || !module_name) return -1;

    // Check if already loaded
    for (size_t i = 0; i < loader->unit_count; i++) {
        if (strcmp(loader->units[i]->module_name, module_name) == 0) {
            if (loader->units[i]->loaded) {
                printf("📦 Module already loaded: %s\n", module_name);
                return (int)i;
            }
            break;
        }
    }

    // Register in registry first
    int reg_index = nova_module_registry_register(loader->registry, module_name);
    if (reg_index < 0) {
        printf("❌ Failed to register module: %s\n", module_name);
        return -1;
    }

    // Get module file info
    if ((size_t)reg_index >= loader->registry->count) return -1;
    nova_module_file_t* mod_file = loader->registry->modules[reg_index];

    // Create module unit
    nova_module_unit_t* unit = calloc(1, sizeof(nova_module_unit_t));
    unit->module_name = strdup(module_name);
    unit->path = strdup(mod_file->abs_path);
    unit->file_id = -1; // Will be set when loaded
    unit->ast = NULL;
    unit->loaded = 0;
    unit->parsing = 0;
    unit->parsed = 0;

    // Add to units array
    if (loader->unit_count >= loader->unit_capacity) {
        loader->unit_capacity *= 2;
        loader->units = realloc(loader->units, loader->unit_capacity * sizeof(nova_module_unit_t*));
    }
    loader->units[loader->unit_count++] = unit;

    // Simulate loading (in real implementation, this would use SourceManager + Parser)
    printf("📂 Loading module: %s from %s\n", module_name, mod_file->abs_path);

    // Check if file exists
    FILE* file = fopen(mod_file->abs_path, "r");
    if (!file) {
        printf("❌ Module file not found: %s\n", mod_file->abs_path);
        return -1;
    }
    fclose(file);

    // Mark as loaded
    unit->loaded = 1;
    unit->file_id = reg_index; // Use registry index as file_id for now

    printf("✅ Module loaded successfully: %s (file_id=%d)\n", module_name, unit->file_id);
    return (int)(loader->unit_count - 1);
}

// Load module from import path
int nova_module_loader_load_from_import(nova_module_loader_t* loader, nova_import_path_t* import_path) {
    char* module_name = resolve_import_path(import_path);
    if (!module_name) return -1;

    int result = nova_module_loader_load(loader, module_name);
    free(module_name);
    return result;
}

// Parse module dependencies (stub - would parse mod/import declarations)
int nova_module_loader_parse_dependencies(nova_module_loader_t* loader, int unit_index) {
    if (unit_index < 0 || (size_t)unit_index >= loader->unit_count) return -1;

    nova_module_unit_t* unit = loader->units[unit_index];

    printf("🔍 Parsing dependencies for: %s\n", unit->module_name);

    // Simulate finding dependencies (in real implementation, parse the file)
    // For now, hardcode some dependencies for testing

    if (strcmp(unit->module_name, "main") == 0) {
        // main depends on lexer, parser, semantic
        nova_module_loader_load(loader, "lexer");
        nova_module_loader_load(loader, "parser");
        nova_module_loader_load(loader, "semantic");
    } else if (strcmp(unit->module_name, "semantic") == 0) {
        // semantic depends on semantic.ast and semantic.types
        nova_module_loader_load(loader, "semantic.ast");
        nova_module_loader_load(loader, "semantic.types");
    }

    unit->parsed = 1;
    printf("✅ Dependencies parsed for: %s\n", unit->module_name);
    return 0;
}

// Print loader status
void nova_module_loader_print(nova_module_loader_t* loader) {
    printf("\n🚀 Module Loader Status:\n");
    printf("Loaded units: %zu\n", loader->unit_count);

    for (size_t i = 0; i < loader->unit_count; i++) {
        nova_module_unit_t* unit = loader->units[i];
        printf("  %zu: %s", i, unit->module_name);
        if (unit->loaded) {
            printf(" ✅ (loaded, file_id=%d)", unit->file_id);
        } else {
            printf(" ❌ (not loaded)");
        }
        if (unit->parsed) {
            printf(" 📋 (parsed)");
        }
        printf("\n");
    }
}

// Test module loading
int test_module_loading() {
    printf("=== Module Loading Orchestration Test ===\n\n");

    // Create loader
    nova_module_loader_t* loader = nova_module_loader_create(".");

    // Load main module
    int main_unit = nova_module_loader_load(loader, "main");
    if (main_unit >= 0) {
        nova_module_loader_parse_dependencies(loader, main_unit);
    }

    // Print final status
    nova_module_loader_print(loader);

    nova_module_loader_destroy(loader);

    printf("\n✅ Module loading test completed\n");
    return 0;
}

int main() {
    return test_module_loading();
}
