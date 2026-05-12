// Module Registry + Path Resolution
// A6.4.1 + A6.4.2 Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Module file structure
typedef struct nova_module_file {
    char* module_name;   // "lexer" / "semantic.parser"
    char* abs_path;      // resolved .nv path
    int file_id;         // SourceManager file id (-1 if not loaded)
} nova_module_file_t;

// Module registry
typedef struct nova_module_registry {
    nova_module_file_t** modules;
    size_t count;
    size_t capacity;
    char* project_root;  // Absolute path to project root
} nova_module_registry_t;

// Import path structure
typedef enum nova_import_kind {
    NOVA_IMPORT_SIMPLE,
    NOVA_IMPORT_SELECTIVE,
    NOVA_IMPORT_WILDCARD,
    NOVA_IMPORT_ALIAS
} nova_import_kind_t;

typedef struct nova_import_path {
    char** segments;   // ["semantic", "ast"]
    int segment_count;
} nova_import_path_t;

// Create module registry
nova_module_registry_t* nova_module_registry_create(const char* project_root) {
    nova_module_registry_t* reg = calloc(1, sizeof(nova_module_registry_t));
    reg->capacity = 16;
    reg->count = 0;
    reg->modules = calloc(reg->capacity, sizeof(nova_module_file_t*));
    reg->project_root = realpath(project_root, NULL);

    printf("📁 Module Registry created for: %s\n", reg->project_root);
    return reg;
}

// Destroy module registry
void nova_module_registry_destroy(nova_module_registry_t* reg) {
    if (!reg) return;

    for (size_t i = 0; i < reg->count; i++) {
        free(reg->modules[i]->module_name);
        free(reg->modules[i]->abs_path);
        free(reg->modules[i]);
    }

    free(reg->modules);
    free(reg->project_root);
    free(reg);
}

// Resolve module path to file system
char* resolve_module_path(nova_module_registry_t* reg, const char* module_name) {
    if (!reg || !module_name) return NULL;

    // Convert module name to file path: "semantic.ast" -> "src/semantic/ast.nv"
    char* path = calloc(strlen(reg->project_root) + strlen(module_name) + 20, 1);

    // Start with project root
    strcpy(path, reg->project_root);

    // Add src/ if not already there
    if (strcmp(path + strlen(path) - 4, "/src") != 0) {
        strcat(path, "/src");
    }

    // Convert dots to path separators
    const char* src = module_name;
    char* dst = path + strlen(path);
    *dst++ = '/';

    while (*src) {
        if (*src == '.') {
            *dst++ = '/';
        } else {
            *dst++ = *src;
        }
        src++;
    }

    // Add .nv extension
    strcpy(dst, ".nv");

    // Check if file exists
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        return path;
    }

    free(path);
    return NULL;
}

// Register a module (without loading)
int nova_module_registry_register(nova_module_registry_t* reg, const char* module_name) {
    if (!reg || !module_name) return -1;

    // Check if already registered
    for (size_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->modules[i]->module_name, module_name) == 0) {
            return (int)i; // Return existing index
        }
    }

    // Resolve path
    char* abs_path = resolve_module_path(reg, module_name);
    if (!abs_path) {
        printf("❌ Cannot resolve module path: %s\n", module_name);
        return -1;
    }

    // Expand capacity if needed
    if (reg->count >= reg->capacity) {
        reg->capacity *= 2;
        reg->modules = realloc(reg->modules, reg->capacity * sizeof(nova_module_file_t*));
    }

    // Create module file entry
    nova_module_file_t* mod = calloc(1, sizeof(nova_module_file_t));
    mod->module_name = strdup(module_name);
    mod->abs_path = abs_path;
    mod->file_id = -1; // Not loaded yet

    reg->modules[reg->count++] = mod;

    printf("📝 Registered module: %s -> %s\n", module_name, abs_path);
    return (int)(reg->count - 1);
}

// Parse import path: "semantic::ast" -> segments ["semantic", "ast"]
nova_import_path_t* parse_import_path(const char* import_str) {
    nova_import_path_t* path = calloc(1, sizeof(nova_import_path_t));

    // Simple parsing: split by ::
    char* copy = strdup(import_str);
    char* token = strtok(copy, "::");
    int count = 0;

    while (token) {
        path->segments = realloc(path->segments, (count + 1) * sizeof(char*));
        path->segments[count] = strdup(token);
        count++;
        token = strtok(NULL, "::");
    }

    path->segment_count = count;
    free(copy);

    return path;
}

// Resolve import path to module name
char* resolve_import_path(nova_import_path_t* path) {
    if (!path || path->segment_count == 0) return NULL;

    // Join segments with dots: ["semantic", "ast"] -> "semantic.ast"
    size_t total_len = 0;
    for (int i = 0; i < path->segment_count; i++) {
        total_len += strlen(path->segments[i]) + 1; // +1 for dot
    }

    char* result = calloc(total_len, 1);
    for (int i = 0; i < path->segment_count; i++) {
        if (i > 0) strcat(result, ".");
        strcat(result, path->segments[i]);
    }

    return result;
}

// Free import path
void nova_import_path_free(nova_import_path_t* path) {
    if (!path) return;

    for (int i = 0; i < path->segment_count; i++) {
        free(path->segments[i]);
    }
    free(path->segments);
    free(path);
}

// Print registry status
void nova_module_registry_print(nova_module_registry_t* reg) {
    printf("\n📊 Module Registry Status:\n");
    printf("Project root: %s\n", reg->project_root);
    printf("Registered modules: %zu\n", reg->count);

    for (size_t i = 0; i < reg->count; i++) {
        nova_module_file_t* mod = reg->modules[i];
        printf("  %zu: %s -> %s", i, mod->module_name, mod->abs_path);
        if (mod->file_id >= 0) {
            printf(" (loaded, file_id=%d)", mod->file_id);
        } else {
            printf(" (not loaded)");
        }
        printf("\n");
    }
}

// Test module registry
int test_module_registry() {
    printf("=== Module Registry + Path Resolution Test ===\n\n");

    // Create registry
    nova_module_registry_t* reg = nova_module_registry_create(".");

    // Register some modules
    nova_module_registry_register(reg, "lexer");
    nova_module_registry_register(reg, "parser");
    nova_module_registry_register(reg, "semantic");
    nova_module_registry_register(reg, "semantic.ast");
    nova_module_registry_register(reg, "semantic.types");

    // Test import path parsing
    nova_import_path_t* path1 = parse_import_path("semantic::ast");
    if (path1) {
        printf("\n📍 Import path parsing:\n");
        printf("  Input: semantic::ast\n");
        printf("  Segments: ");
        for (int i = 0; i < path1->segment_count; i++) {
            printf("%s", path1->segments[i]);
            if (i < path1->segment_count - 1) printf(" -> ");
        }
        printf("\n");

        char* resolved = resolve_import_path(path1);
        printf("  Resolved: %s\n", resolved);

        // Try to register the resolved module
        nova_module_registry_register(reg, resolved);
        free(resolved);

        nova_import_path_free(path1);
    }

    // Print final status
    nova_module_registry_print(reg);

    nova_module_registry_destroy(reg);

    printf("\n✅ Module registry test completed\n");
    return 0;
}

int main() {
    return test_module_registry();
}
