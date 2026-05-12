/**
 * @file optimizer.c
 * @brief Optimization pass manager
 */

#include "compiler/ir.h"
#include "optimizer.h"
#include <stdio.h>

/* Forward declarations */
extern int nova_opt_const_fold(nova_ir_module_t *module);
extern int nova_opt_const_prop(nova_ir_module_t *module);
extern int nova_opt_dce(nova_ir_module_t *module);
extern int nova_opt_inline(nova_ir_module_t *module);
extern int nova_opt_loop_unroll(nova_ir_module_t *module);
extern int nova_opt_loop_licm(nova_ir_module_t *module);

typedef int (*opt_pass_t)(nova_ir_module_t *);

typedef struct {
    const char *name;
    opt_pass_t func;
    int level;  /* O1, O2, O3 */
} opt_pass_entry_t;

static opt_pass_entry_t opt_passes[] = {
    {"Constant Folding", nova_opt_const_fold, 1},
    {"Constant Propagation", nova_opt_const_prop, 1},
    {"Dead Code Elimination", nova_opt_dce, 1},
    {"Function Inlining", nova_opt_inline, 2},
    {"Loop Unrolling", nova_opt_loop_unroll, 2},
    {"Loop Invariant Code Motion", nova_opt_loop_licm, 3},
    {NULL, NULL, 0}
};

/* Run optimization passes at specified level */
int nova_optimize(nova_ir_module_t *module, int opt_level)
{
    if (!module) return -1;
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Nova Optimizer: Running -O%d passes                      ║\n", opt_level);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    int total_passes = 0;
    
    for (int i = 0; opt_passes[i].name != NULL; i++) {
        if (opt_passes[i].level <= opt_level) {
            printf("Running: %s\n", opt_passes[i].name);
            opt_passes[i].func(module);
            total_passes++;
        }
    }
    
    printf("\n✅ Optimization complete: %d passes run\n\n", total_passes);
    
    return 0;
}
