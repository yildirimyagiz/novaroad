/**
 * @file paging.c
 * @brief Page table management
 */

#include "paging.h"
#include "kernel/memory.h"

int nova_map_page(nova_vaddr_t vaddr, nova_paddr_t paddr, uint32_t flags)
{
    /* TODO: Map virtual page to physical frame */
    (void)vaddr; (void)paddr; (void)flags;
    return 0;
}

void nova_unmap_page(nova_vaddr_t vaddr)
{
    /* TODO: Unmap virtual page */
    (void)vaddr;
}
