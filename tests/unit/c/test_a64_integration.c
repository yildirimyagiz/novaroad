// A6.4 Integration Test
// Complete module loading pipeline test

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include our implementations
#include "module_registry.c"
#include "module_loader.c"
#include "import_graph.c"

// Main integration test
int test_a64_integration() {
    printf("🚀 A6.4 Module Loading Integration Test\n");
    printf("=======================================\n\n");

    // Create module loader (includes registry)
    nova_module_loader_t* loader = nova_module_loader_create(".");

    // Load main module
    printf("📂 Loading main module...\n");
    int main_unit = nova_module_loader_load(loader, "main");
    if (main_unit < 0) {
        printf("❌ Failed to load main module\n");
        nova_module_loader_destroy(loader);
        return 1;
    }

    // Parse main module dependencies
    printf("\n🔍 Parsing main module dependencies...\n");
    nova_module_loader_parse_dependencies(loader, main_unit);

    // Create import graph
    printf("\n🔗 Building import graph...\n");
    nova_import_graph_t* graph = nova_import_graph_create();
    nova_import_graph_build(graph, loader);

    // Print status
    printf("\n📊 Final Status:\n");
    nova_module_loader_print(loader);
    nova_import_graph_print(graph, loader);

    // Validation checks
    printf("\n✅ Validation:\n");

    // Check 1: Main module loaded
    int main_loaded = 0;
    for (size_t i = 0; i < loader->unit_count; i++) {
        if (strcmp(loader->units[i]->module_name, "main") == 0 && loader->units[i]->loaded) {
            main_loaded = 1;
            break;
        }
    }
    printf("  %s Main module loaded\n", main_loaded ? "✅" : "❌");

    // Check 2: Dependencies loaded
    int lexer_loaded = 0, parser_loaded = 0, semantic_loaded = 0;
    for (size_t i = 0; i < loader->unit_count; i++) {
        if (strcmp(loader->units[i]->module_name, "lexer") == 0 && loader->units[i]->loaded) lexer_loaded = 1;
        if (strcmp(loader->units[i]->module_name, "parser") == 0 && loader->units[i]->loaded) parser_loaded = 1;
        if (strcmp(loader->units[i]->module_name, "semantic") == 0 && loader->units[i]->loaded) semantic_loaded = 1;
    }
    printf("  %s Lexer module loaded\n", lexer_loaded ? "✅" : "❌");
    printf("  %s Parser module loaded\n", parser_loaded ? "✅" : "❌");
    printf("  %s Semantic module loaded\n", semantic_loaded ? "✅" : "❌");

    // Check 3: Import graph has edges
    printf("  %s Import graph has edges (%zu)\n", graph->edge_count > 0 ? "✅" : "❌", graph->edge_count);

    // Check 4: No self-imports
    int self_imports = nova_import_graph_check_self_imports(graph);
    printf("  %s No self-imports\n", self_imports == 0 ? "✅" : "❌");

    // Topological sort
    size_t sort_count;
    int* sorted = nova_import_graph_topological_sort(graph, loader, &sort_count);
    printf("  %s Topological sort computed (%zu modules)\n", sorted ? "✅" : "❌", sort_count);
    free(sorted);

    // Clean up
    nova_import_graph_destroy(graph);
    nova_module_loader_destroy(loader);

    printf("\n🏆 A6.4 Integration Test: %s\n",
           (main_loaded && lexer_loaded && parser_loaded && semantic_loaded) ? "PASSED" : "FAILED");

    return 0;
}

int main() {
    return test_a64_integration();
}
