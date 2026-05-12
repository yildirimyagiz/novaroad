/**
 * @file allocator.c
 * @brief Physical frame allocator
 */

#include "allocator.h"
#include "kernel/memory.h"
#include <stddef.h>

#define MAX_FRAMES 65536
#define FRAME_FREE 0
#define FRAME_USED 1

static struct {
    uint8_t frames[MAX_FRAMES];
    size_t total_frames;
    size_t free_frames;
} frame_allocator;

void nova_frame_allocator_init(void)
{
    frame_allocator.total_frames = MAX_FRAMES;
    frame_allocator.free_frames = MAX_FRAMES;
    
    for (size_t i = 0; i < MAX_FRAMES; i++) {
        frame_allocator.frames[i] = FRAME_FREE;
    }
}

nova_paddr_t nova_frame_alloc(void)
{
    for (size_t i = 0; i < frame_allocator.total_frames; i++) {
        if (frame_allocator.frames[i] == FRAME_FREE) {
            frame_allocator.frames[i] = FRAME_USED;
            frame_allocator.free_frames--;
            return i * NOVA_PAGE_SIZE;
        }
    }
    return 0; /* Out of memory */
}

void nova_frame_free(nova_paddr_t paddr)
{
    size_t frame = paddr / NOVA_PAGE_SIZE;
    if (frame < frame_allocator.total_frames) {
        frame_allocator.frames[frame] = FRAME_FREE;
        frame_allocator.free_frames++;
    }
}
