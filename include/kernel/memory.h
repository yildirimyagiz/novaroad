/**
 * @file memory.h
 * @brief Kernel memory management
 */

#ifndef NOVA_MEMORY_H
#define NOVA_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOVA_PAGE_SIZE 4096

typedef uint64_t nova_paddr_t;
typedef uint64_t nova_vaddr_t;

/* Page flags */
#define NOVA_PAGE_PRESENT   (1 << 0)
#define NOVA_PAGE_WRITABLE  (1 << 1)
#define NOVA_PAGE_USER      (1 << 2)
#define NOVA_PAGE_NOCACHE   (1 << 3)

/* ============================================================================
 * Memory Initialization
 * ========================================================================== */

/**
 * Initialize memory management
 * @param mem_size Total physical memory
 * @return 0 on success
 */
int nova_memory_init(size_t mem_size);

/* ============================================================================
 * Physical Memory
 * ========================================================================== */

/**
 * Allocate physical frame
 * @return Physical address or 0 on error
 */
nova_paddr_t nova_frame_alloc(void);

/**
 * Free physical frame
 * @param paddr Physical address
 */
void nova_frame_free(nova_paddr_t paddr);

/**
 * Allocate contiguous frames
 * @param count Number of frames
 * @return Physical address or 0 on error
 */
nova_paddr_t nova_frame_alloc_contiguous(size_t count);

/* ============================================================================
 * Virtual Memory
 * ========================================================================== */

/**
 * Map virtual to physical page
 * @param vaddr Virtual address
 * @param paddr Physical address
 * @param flags Page flags
 * @return 0 on success, -1 on error
 */
int nova_map_page(nova_vaddr_t vaddr, nova_paddr_t paddr, uint32_t flags);

/**
 * Unmap virtual page
 * @param vaddr Virtual address
 */
void nova_unmap_page(nova_vaddr_t vaddr);

/**
 * Translate virtual to physical
 * @param vaddr Virtual address
 * @param paddr Output: physical address
 * @return true if mapped, false otherwise
 */
bool nova_virt_to_phys(nova_vaddr_t vaddr, nova_paddr_t *paddr);

/* ============================================================================
 * Kernel Heap
 * ========================================================================== */

/**
 * Kernel heap allocation
 * @param size Size in bytes
 * @return Pointer or NULL on error
 */
void *nova_kmalloc(size_t size);

/**
 * Kernel heap free
 * @param ptr Pointer to free
 */
void nova_kfree(void *ptr);

/**
 * Get memory statistics
 * @param total_mem Output: total memory
 * @param free_mem Output: free memory
 * @param used_mem Output: used memory
 */
void nova_memory_stats(size_t *total_mem, size_t *free_mem, size_t *used_mem);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_MEMORY_H */
