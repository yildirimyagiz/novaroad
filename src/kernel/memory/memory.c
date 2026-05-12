/**
 * @file memory.c
 * @brief Memory subsystem initialization
 */

#include "kernel/memory.h"
#include "allocator.h"
#include "heap.h"
#include "vmm.h"

int nova_memory_init(void)
{
    /* Initialize frame allocator */
    nova_frame_allocator_init();
    
    /* Initialize kernel heap */
    nova_heap_init();
    
    /* Initialize virtual memory manager */
    nova_vmm_init();
    
    return 0;
}
