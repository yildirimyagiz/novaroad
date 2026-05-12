#include "formal/klee/nova_klee_harness.h"
#include <stdio.h>

int nova_klee_generate_harness(const char *kernel_id,
                                 const char *output_path) {
  printf("[Gödel/KLEE] Generating harness for '%s' at %s\n", kernel_id,
         output_path);
  // Logic to write a C file that calls the kernel with symbolic inputs
  return 0;
}
