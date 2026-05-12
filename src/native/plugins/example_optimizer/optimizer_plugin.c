/**
 * Example Nova Plugin: Custom Optimizer
 * Demonstrates how to create a plugin that extends Nova's optimization pipeline
 */

#include "plugin/plugin.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// Plugin Metadata
// ============================================================================

NOVA_PLUGIN_DEFINE(
    "example_optimizer",
    "1.0.0",
    "Nova Team",
    "Example optimizer plugin demonstrating custom optimization passes"
);

// ============================================================================
// Plugin State
// ============================================================================

typedef struct {
    int optimization_level;
    bool aggressive_inlining;
    size_t optimizations_applied;
} OptimizerState;

static OptimizerState g_state = {
    .optimization_level = 2,
    .aggressive_inlining = false,
    .optimizations_applied = 0
};

// ============================================================================
// Optimization Interface
// ============================================================================

/**
 * Perform constant folding optimization
 */
void* optimize_constant_folding(void* ir_module) {
    printf("[ExampleOptimizer] Running constant folding pass\n");
    g_state.optimizations_applied++;
    
    // In a real plugin, this would traverse IR and fold constants
    // For now, just return the module unchanged
    return ir_module;
}

/**
 * Perform dead code elimination
 */
void* optimize_dead_code_elimination(void* ir_module) {
    printf("[ExampleOptimizer] Running dead code elimination pass\n");
    g_state.optimizations_applied++;
    
    // Placeholder for actual DCE implementation
    return ir_module;
}

/**
 * Perform function inlining
 */
void* optimize_inline_functions(void* ir_module) {
    if (g_state.aggressive_inlining) {
        printf("[ExampleOptimizer] Running aggressive inlining pass\n");
        g_state.optimizations_applied++;
    } else {
        printf("[ExampleOptimizer] Skipping inlining (not enabled)\n");
    }
    
    return ir_module;
}

/**
 * Run full optimization pipeline
 */
void* run_optimization_pipeline(void* ir_module) {
    printf("[ExampleOptimizer] Starting optimization pipeline (level %d)\n", 
           g_state.optimization_level);
    
    void* result = ir_module;
    
    if (g_state.optimization_level >= 1) {
        result = optimize_constant_folding(result);
    }
    
    if (g_state.optimization_level >= 2) {
        result = optimize_dead_code_elimination(result);
    }
    
    if (g_state.optimization_level >= 3) {
        g_state.aggressive_inlining = true;
        result = optimize_inline_functions(result);
    }
    
    printf("[ExampleOptimizer] Pipeline complete. Applied %zu optimizations\n",
           g_state.optimizations_applied);
    
    return result;
}

// ============================================================================
// Configuration API
// ============================================================================

void set_optimization_level(int level) {
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    
    g_state.optimization_level = level;
    printf("[ExampleOptimizer] Optimization level set to %d\n", level);
}

void enable_aggressive_inlining(bool enable) {
    g_state.aggressive_inlining = enable;
    printf("[ExampleOptimizer] Aggressive inlining %s\n", 
           enable ? "enabled" : "disabled");
}

size_t get_optimizations_count(void) {
    return g_state.optimizations_applied;
}

void reset_stats(void) {
    g_state.optimizations_applied = 0;
    printf("[ExampleOptimizer] Statistics reset\n");
}

// ============================================================================
// Plugin Lifecycle
// ============================================================================

int plugin_init(void) {
    printf("=== Example Optimizer Plugin ===\n");
    printf("Version: 1.0.0\n");
    printf("Initializing...\n");
    
    g_state.optimization_level = 2;
    g_state.aggressive_inlining = false;
    g_state.optimizations_applied = 0;
    
    printf("Initialization complete!\n\n");
    return 0;
}

void plugin_cleanup(void) {
    printf("\n=== Example Optimizer Plugin Shutdown ===\n");
    printf("Total optimizations applied: %zu\n", g_state.optimizations_applied);
    printf("Cleanup complete.\n");
}

NOVA_PLUGIN_INIT(plugin_init);
NOVA_PLUGIN_CLEANUP(plugin_cleanup);

// ============================================================================
// Exported Symbols (for Nova to call)
// ============================================================================

// Export the main optimization function
extern void* nova_plugin_optimize(void* ir_module) {
    return run_optimization_pipeline(ir_module);
}

// Export configuration functions
extern void nova_plugin_set_opt_level(int level) {
    set_optimization_level(level);
}

extern void nova_plugin_enable_inlining(bool enable) {
    enable_aggressive_inlining(enable);
}

extern size_t nova_plugin_get_stats(void) {
    return get_optimizations_count();
}
