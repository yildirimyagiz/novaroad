#include "include/compiler/ast.h"
#include "src/compiler/diagnostics.c"
#include "src/compiler/sourcemgr.c"
#include <stdio.h>

int main() {
  // Create diagnostics
  nova_diag_collector_t *dc = nova_diag_collector_create();

  // Create dummy sourcemgr
  nova_sourcemgr_t *sm = nova_sourcemgr_create();

  // Add some test diagnostics
  nova_span_t span1 = {0, 10, 15, 2, 5};
  nova_diag_error(dc, span1, "Test syntax error: missing expression");

  nova_span_t span2 = {0, 20, 25, 3, 10};
  nova_diag_warning(dc, span2, "Test warning: unused variable");

  // Print diagnostics
  nova_diag_print(dc, sm);

  // Clean up
  nova_diag_collector_destroy(dc);
  nova_sourcemgr_destroy(sm);

  printf("Diagnostics test completed!\n");
  return 0;
}
