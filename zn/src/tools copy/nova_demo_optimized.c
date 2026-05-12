#include "nova_arena.h"
#include "nova_incremental.h"
#include "nova_ir.h"
#include "nova_jit.h"
#include <stdio.h>
#include <string.h>

void demo_full_pipeline_optimization() {
  printf("═══════════════════════════════════════════════════════════════\n");
  printf("        NOVA FULL PIPELINE OPTIMIZATION DEMO\n");
  printf("═══════════════════════════════════════════════════════════════\n");

  // 1. Initialized Arena (The base of everything)
  Arena *global_arena = arena_create(2 * 1024 * 1024); // 2MB
  printf("[1] Arena Initialized: 2MB Pre-allocated\n");

  // 2. Incremental Context
  NovaIncrementalContext *inc = nova_incremental_create(".nova_cache");
  nova_incremental_load_manifest(inc);
  const char *mock_source = "fn main() { return 42; }";
  NovaHash src_hash = nova_hash_buffer(mock_source, strlen(mock_source));
  NovaHash config_hash = 12345; // Mock config hash

  if (nova_incremental_needs_compile(inc, "main.zn", src_hash, config_hash)) {
    printf("[2] Incremental: main.zn changed, starting compilation...\n");

    // 3. IR Generation (Arena-based)
    IRModule *mod = ir_module_create(global_arena);
    IRFunction *fn = ir_function_create(mod, "main", IR_TYPE_I32);
    IRBlock *entry = ir_block_create(fn);

    ir_block_append(entry, ir_make_const_int(0, 42, IR_TYPE_I32));
    ir_block_append(entry, ir_make_return(0));

    printf("[3] IR Modules Created: All allocations done in global arena\n");
    ir_print_function(fn);

    // 4. JIT Integration
    NovaJITContext *jit = nova_jit_create(NOVA_JIT_LLVM);
    printf("[4] JIT Context Created: Using internal arena for profiling\n");

    nova_jit_compile(jit, fn);

    nova_incremental_update_cache(inc, "main.zn", src_hash, config_hash,
                                    "main.o", NULL, NULL, 0);
    nova_incremental_save_manifest(inc);

    nova_jit_destroy(jit);
  } else {
    printf("[2] Incremental: main.zn unchanged, Skipping entire pipeline!\n");
  }

  // O(1) Cleanup
  nova_incremental_destroy(inc);
  arena_destroy(global_arena);
  printf("\n[5] O(1) Global Cleanup: All memory released instantly.\n");
  printf("═══════════════════════════════════════════════════════════════\n");
}

int main() {
  // Run twice to demonstrate incremental cache hit
  printf("\n>>> FIRST RUN (Full Compile) <<<\n");
  demo_full_pipeline_optimization();

  printf("\n>>> SECOND RUN (Incremental Hit) <<<\n");
  demo_full_pipeline_optimization();

  return 0;
}
