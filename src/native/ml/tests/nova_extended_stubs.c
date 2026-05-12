/**
 * nova_extended_stubs.c
 * Stub implementations for symbols required by extended ML modules
 * but not yet linked (solver, profiler, contracts, formal).
 */
#include "nova_solver.h"
#include "nova_profiler_v2.h"
#include "nova_kernel_contracts.h"
#include "formal/nova_formal.h"
#include "nova_obligation.h"
#include <stdlib.h>
#include <string.h>

/* ── SolverSession is opaque — define it here ─────────────────── */
struct SolverSession { int dummy; };

/* ── Solver stubs ─────────────────────────────────────────────── */
SolverConfig solver_config_default(void) {
    SolverConfig c;
    memset(&c, 0, sizeof(c));
    return c;
}
SolverSession *solver_session_create(const SolverConfig *config) {
    (void)config;
    return (SolverSession *)calloc(1, sizeof(SolverSession));
}
void solver_session_destroy(SolverSession *s) { free(s); }

/* ── Profiler V2 stubs ────────────────────────────────────────── */
bool nova_profiler_is_hot(NovaProfilerV2 *p, const char *path_name) {
    (void)p; (void)path_name;
    return false;
}

/* ── Kernel contract stubs ────────────────────────────────────── */
NovaObligation nova_contract_matmul(const NovaTensor *A,
                                     const NovaTensor *B,
                                     const NovaTensor *Bias,
                                     NovaTensor *Out) {
    (void)A; (void)B; (void)Bias; (void)Out;
    NovaObligation ob;
    memset(&ob, 0, sizeof(ob));
    ob.op_name = "matmul";
    ob.all_satisfied = true;
    return ob;
}

NovaObligation nova_contract_add(const NovaTensor *A,
                                  const NovaTensor *B,
                                  NovaTensor *Out) {
    (void)A; (void)B; (void)Out;
    NovaObligation ob;
    memset(&ob, 0, sizeof(ob));
    ob.op_name = "add";
    ob.all_satisfied = true;
    return ob;
}

/* ── Formal check stub ────────────────────────────────────────── */
bool nova_formal_check_invariant(const char *name, void *data) {
    (void)name; (void)data;
    return true;
}

/* ── Memory allocation stubs ──────────────────────────────────── */
void *nova_malloc(size_t size) {
    return malloc(size);
}

void nova_free(void *ptr) {
    free(ptr);
}

void *nova_malloc_aligned(size_t size, size_t alignment) {
    (void)alignment;
    /* Simple aligned allocation stub — real implementation in nova_allocator.c */
    return malloc(size);
}
