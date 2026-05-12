#include "formal/klee/nova_klee_bridge.h"
#include <stdio.h>

#ifdef NOVA_ENABLE_KLEE
#include <klee/klee.h>
#endif

void nova_klee_make_symbolic(void *addr, size_t nbytes, const char *name) {
#ifdef NOVA_ENABLE_KLEE
  klee_make_symbolic(addr, nbytes, name);
#else
  printf("[Gödel/KLEE] Stub: Making %p (%zu bytes) symbolic as '%s'\n", addr,
         nbytes, name);
#endif
}

void nova_klee_assume(int condition) {
#ifdef NOVA_ENABLE_KLEE
  klee_assume(condition);
#endif
}

void nova_klee_abort(const char *reason) {
#ifdef NOVA_ENABLE_KLEE
  klee_report_error(__FILE__, __LINE__, reason, "nova.abort");
#else
  fprintf(stderr, "[Gödel/KLEE] Abort: %s\n", reason);
#endif
}
