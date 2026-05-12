#ifndef NOVA_KLEE_BRIDGE_H
#define NOVA_KLEE_BRIDGE_H

#include <stddef.h>

/**
 * Nova-KLEE Bridge
 * Wraps KLEE intrinsics for symbolic execution of Nova workloads.
 */

// Marks a memory region as symbolic for KLEE
void nova_klee_make_symbolic(void *addr, size_t nbytes, const char *name);

// Forces a branch for KLEE to explore both paths
void nova_klee_assume(int condition);

// Reports an error to KLEE
void nova_klee_abort(const char *reason);

#endif // NOVA_KLEE_BRIDGE_H
