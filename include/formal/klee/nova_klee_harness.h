#ifndef NOVA_KLEE_HARNESS_H
#define NOVA_KLEE_HARNESS_H

/**
 * Harness generator for KLEE.
 * Produces symbolic entry points for Nova kernels and ops.
 */

int nova_klee_generate_harness(const char *kernel_id,
                                 const char *output_path);

#endif // NOVA_KLEE_HARNESS_H
