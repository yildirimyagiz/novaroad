/**
 * @file panic.c
 * @brief Kernel panic handler
 */

#include "panic.h"
#include "kernel/hal.h"
#include <stdarg.h>

void nova_panic(const char *msg)
{
    /* TODO: Print panic message */
    (void)msg;
    
    nova_hal_disable_interrupts();
    nova_hal_halt();
    
    while (1) {
        /* Halt forever */
    }
}
