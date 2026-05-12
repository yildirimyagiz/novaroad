/**
 * @file hal.c
 * @brief Hardware Abstraction Layer dispatcher
 */

#include "kernel/hal.h"
#include "hal.h"

/* Forward declarations for platform-specific functions */
extern void nova_hal_arch_init(void);
extern void nova_hal_arch_cpu_init(void);
extern void nova_hal_arch_enable_interrupts(void);
extern void nova_hal_arch_disable_interrupts(void);
extern void nova_hal_arch_halt(void);
extern uint32_t nova_hal_arch_cpu_count(void);
extern uint32_t nova_hal_arch_cpu_id(void);

int nova_hal_init(void)
{
    nova_hal_arch_init();
    return 0;
}

void nova_hal_cpu_init(void)
{
    nova_hal_arch_cpu_init();
}

void nova_hal_enable_interrupts(void)
{
    nova_hal_arch_enable_interrupts();
}

void nova_hal_disable_interrupts(void)
{
    nova_hal_arch_disable_interrupts();
}

void nova_hal_halt(void)
{
    nova_hal_arch_halt();
}

uint32_t nova_hal_cpu_count(void)
{
    return nova_hal_arch_cpu_count();
}

uint32_t nova_hal_cpu_id(void)
{
    return nova_hal_arch_cpu_id();
}
