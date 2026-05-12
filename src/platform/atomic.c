/**
 * @file atomic.c
 * @brief Atomic operations implementation
 */

#include "platform/atomic.h"
#include <stdatomic.h>

uint64_t nova_atomic_load_u64(nova_atomic_u64_t *atomic)
{
    return atomic_load_explicit((_Atomic uint64_t *)&atomic->value, memory_order_acquire);
}

void nova_atomic_store_u64(nova_atomic_u64_t *atomic, uint64_t value)
{
    atomic_store_explicit((_Atomic uint64_t *)&atomic->value, value, memory_order_release);
}

bool nova_atomic_cas_u64(nova_atomic_u64_t *atomic, uint64_t expected, uint64_t desired)
{
    return atomic_compare_exchange_strong_explicit(
        (_Atomic uint64_t *)&atomic->value,
        &expected,
        desired,
        memory_order_acq_rel,
        memory_order_acquire
    );
}

uint64_t nova_atomic_fetch_add_u64(nova_atomic_u64_t *atomic, uint64_t value)
{
    return atomic_fetch_add_explicit((_Atomic uint64_t *)&atomic->value, value, memory_order_acq_rel);
}

void nova_atomic_fence(void)
{
    atomic_thread_fence(memory_order_seq_cst);
}
