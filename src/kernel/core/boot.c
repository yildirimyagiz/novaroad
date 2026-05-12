/**
 * @file boot.c
 * @brief Kernel entry point and early initialization
 */

#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/hal.h"
#include "boot.h"
#include <stdint.h>

extern void nova_main(void);

/**
 * Kernel entry point (called from assembly)
 */
void nova_kernel_entry(void)
{
    /* Initialize HAL first */
    nova_hal_init();
    
    /* Initialize memory management */
    nova_memory_init();
    
    /* Jump to kernel main */
    nova_main();
}

/**
 * Kernel main function
 */
void nova_main(void)
{
    /* TODO: Initialize remaining subsystems */
    
    /* Enter main loop */
    nova_kernel_run();
}

int nova_kernel_init(void)
{
    return 0;
}

void nova_kernel_shutdown(void)
{
    nova_hal_halt();
}

void nova_kernel_run(void)
{
    /* Kernel main loop */
    while (1) {
        nova_hal_halt();
    }
}

const char *nova_kernel_version(void)
{
    return "Nova Kernel 0.1.0";
}
