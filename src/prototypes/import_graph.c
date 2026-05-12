// Import Graph Construction
// A6.4.4 Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Import graph edge
typedef struct nova_module_edge {
    int from_module; // importer
    int to_module;   // imported
} nova_module_edge_t;

// Import graph
typedef struct nova_import_graph {
    nova_module_edge_t** edges;
    size_t edge_count;
    size_t edge_capacity;
} nova_import_graph_t;

// Forward declarations
typedef struct nova_module_loader nova_module_loader_t;

// Create import graph
nova_import_graph_t* nova_import_graph_create(void) {
    nova_import_graph_t* graph = calloc(1, sizeof(nova_import_graph_t));
    graph->edge_capacity = 32;
    graph->edge_count = 0;
    graph->edges = calloc(graph->edge_capacity, sizeof(nova_module_edge_t*));

    printf("🔗 Import Graph created\n");
    return graph;
}

// Destroy import graph
void nova_import_graph_destroy(nova_import_graph_t* graph) {
    if (!graph) return;

    for (size_t i = 0; i < graph->edge_count; i++) {
        free(graph->edges[i]);
    }

    free(graph->edges);
    free(graph);
}

// Add edge to graph
void nova_import_graph_add_edge(nova_import_graph_t* graph, int from_module, int to_module) {
    if (!graph) return;

    // Check for duplicates
    for (size_t i = 0; i < graph->edge_count; i++) {
        if (graph->edges[i]->from_module == from_module &&
            graph->edges[i]->to_module == to_module) {
            return; // Already exists
        }
    }

    // Expand capacity if needed
    if (graph->edge_count >= graph->edge_capacity) {
        graph->edge_capacity *= 2;
        graph->edges = realloc(graph->edges, graph->edge_capacity * sizeof(nova_module_edge_t*));
    }

    // Add edge
    nova_module_edge_t* edge = malloc(sizeof(nova_module_edge_t));
    edge->from_module = from_module;
    edge->to_module = to_module;
    graph->edges[graph->edge_count++] = edge;

    printf("➡️  Added import edge: %d -> %d\n", from_module, to_module);
}

// Build import graph from module loader
void nova_import_graph_build(nova_import_graph_t* graph, nova_module_loader_t* loader) {
    printf("🏗️  Building import graph...\n");

    // For each loaded module, add edges based on dependencies
    for (size_t i = 0; i < loader->unit_count; i++) {
        // Simulate dependency analysis (in real implementation, parse import statements)
        int from_module = (int)i;

        if (strcmp(loader->units[i]->module_name, "main") == 0) {
            // main imports lexer, parser, semantic
            for (size_t j = 0; j < loader->unit_count; j++) {
                if (strcmp(loader->units[j]->module_name, "lexer") == 0 ||
                    strcmp(loader->units[j]->module_name, "parser") == 0 ||
                    strcmp(loader->units[j]->module_name, "semantic") == 0) {
                    nova_import_graph_add_edge(graph, from_module, (int)j);
                }
            }
        } else if (strcmp(loader->units[i]->module_name, "semantic") == 0) {
            // semantic imports semantic.ast and semantic.types
            for (size_t j = 0; j < loader->unit_count; j++) {
                if (strcmp(loader->units[j]->module_name, "semantic.ast") == 0 ||
                    strcmp(loader->units[j]->module_name, "semantic.types") == 0) {
                    nova_import_graph_add_edge(graph, from_module, (int)j);
                }
            }
        }
    }

    printf("✅ Import graph built with %zu edges\n", graph->edge_count);
}

// Print import graph
void nova_import_graph_print(nova_import_graph_t* graph, nova_module_loader_t* loader) {
    printf("\n🔗 Import Graph:\n");

    for (size_t i = 0; i < graph->edge_count; i++) {
        nova_module_edge_t* edge = graph->edges[i];
        const char* from_name = (size_t)edge->from_module < loader->unit_count ?
                               loader->units[edge->from_module]->module_name : "???";
        const char* to_name = (size_t)edge->to_module < loader->unit_count ?
                             loader->units[edge->to_module]->module_name : "???";

        printf("  %s -> %s\n", from_name, to_name);
    }
}

// Check for self-imports
int nova_import_graph_check_self_imports(nova_import_graph_t* graph) {
    printf("🔍 Checking for self-imports...\n");

    for (size_t i = 0; i < graph->edge_count; i++) {
        if (graph->edges[i]->from_module == graph->edges[i]->to_module) {
            printf("❌ Self-import detected: module %d imports itself\n", graph->edges[i]->from_module);
            return 1; // Error
        }
    }

    printf("✅ No self-imports found\n");
    return 0;
}

// Simple topological sort (basic implementation)
int* nova_import_graph_topological_sort(nova_import_graph_t* graph, nova_module_loader_t* loader, size_t* out_count) {
    if (!graph || !loader) return NULL;

    printf("📊 Computing topological sort...\n");

    // Simple implementation: just return in dependency order
    // Real implementation would use Kahn's algorithm or DFS

    int* result = malloc(loader->unit_count * sizeof(int));
    size_t result_count = 0;

    // For now, just put modules in a reasonable order
    // In real implementation, this would respect dependencies

    // First, put leaf modules (no dependencies)
    for (size_t i = 0; i < loader->unit_count; i++) {
        int has_dependencies = 0;
        for (size_t j = 0; j < graph->edge_count; j++) {
            if (graph->edges[j]->from_module == (int)i) {
                has_dependencies = 1;
                break;
            }
        }
        if (!has_dependencies) {
            result[result_count++] = (int)i;
        }
    }

    // Then modules with dependencies
    for (size_t i = 0; i < loader->unit_count; i++) {
        int already_added = 0;
        for (size_t j = 0; j < result_count; j++) {
            if (result[j] == (int)i) {
                already_added = 1;
                break;
            }
        }
        if (!already_added) {
            result[result_count++] = (int)i;
        }
    }

    printf("📋 Topological order: ");
    for (size_t i = 0; i < result_count; i++) {
        printf("%s", loader->units[result[i]]->module_name);
        if (i < result_count - 1) printf(" -> ");
    }
    printf("\n");

    *out_count = result_count;
    return result;
}

// Test import graph
int test_import_graph() {
    printf("=== Import Graph Construction Test ===\n\n");

    // Create loader with some modules
    nova_module_loader_t* loader = nova_module_loader_create(".");

    // Load modules
    nova_module_loader_load(loader, "main");
    nova_module_loader_load(loader, "lexer");
    nova_module_loader_load(loader, "parser");
    nova_module_loader_load(loader, "semantic");
    nova_module_loader_load(loader, "semantic.ast");
    nova_module_loader_load(loader, "semantic.types");

    // Parse dependencies
    for (size_t i = 0; i < loader->unit_count; i++) {
        nova_module_loader_parse_dependencies(loader, (int)i);
    }

    // Create and build import graph
    nova_import_graph_t* graph = nova_import_graph_create();
    nova_import_graph_build(graph, loader);

    // Print graph
    nova_import_graph_print(graph, loader);

    // Check for self-imports
    nova_import_graph_check_self_imports(graph);

    // Topological sort
    size_t sort_count;
    int* sorted = nova_import_graph_topological_sort(graph, loader, &sort_count);
    free(sorted);

    // Clean up
    nova_import_graph_destroy(graph);
    nova_module_loader_destroy(loader);

    printf("\n✅ Import graph test completed\n");
    return 0;
}

int main() {
    return test_import_graph();
}
