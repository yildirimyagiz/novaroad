/**
 * @file atomic.h
 * @brief Atomic operations (enhanced)
 */

#ifndef NOVA_ATOMIC_H
#define NOVA_ATOMIC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { volatile uint64_t value; } nova_atomic_u64_t;
typedef struct { volatile uint32_t value; } nova_atomic_u32_t;
typedef struct { volatile void *ptr; } nova_atomic_ptr_t;

/* Memory ordering */
typedef enum {
    NOVA_MEM_RELAXED,
    NOVA_MEM_ACQUIRE,
    NOVA_MEM_RELEASE,
    NOVA_MEM_ACQ_REL,
    NOVA_MEM_SEQ_CST,
} nova_mem_order_t;

/* ============================================================================
 * 64-bit Atomics
 * ========================================================================== */

uint64_t nova_atomic_load_u64(nova_atomic_u64_t *atomic);
void nova_atomic_store_u64(nova_atomic_u64_t *atomic, uint64_t value);
bool nova_atomic_cas_u64(nova_atomic_u64_t *atomic, uint64_t expected, uint64_t desired);
uint64_t nova_atomic_fetch_add_u64(nova_atomic_u64_t *atomic, uint64_t value);
uint64_t nova_atomic_fetch_sub_u64(nova_atomic_u64_t *atomic, uint64_t value);
uint64_t nova_atomic_exchange_u64(nova_atomic_u64_t *atomic, uint64_t value);

/* ============================================================================
 * 32-bit Atomics
 * ========================================================================== */

uint32_t nova_atomic_load_u32(nova_atomic_u32_t *atomic);
void nova_atomic_store_u32(nova_atomic_u32_t *atomic, uint32_t value);
bool nova_atomic_cas_u32(nova_atomic_u32_t *atomic, uint32_t expected, uint32_t desired);
uint32_t nova_atomic_fetch_add_u32(nova_atomic_u32_t *atomic, uint32_t value);

/* ============================================================================
 * Pointer Atomics
 * ========================================================================== */

void *nova_atomic_load_ptr(nova_atomic_ptr_t *atomic);
void nova_atomic_store_ptr(nova_atomic_ptr_t *atomic, void *ptr);
bool nova_atomic_cas_ptr(nova_atomic_ptr_t *atomic, void *expected, void *desired);

/* ============================================================================
 * Memory Fences
 * ========================================================================== */

void nova_atomic_fence(void);
void nova_atomic_compiler_fence(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_ATOMIC_H */
