/**
 * @file cpu.c
 * @brief x86_64 CPU initialization
 */

#include "cpu.h"
#include <stdint.h>

void nova_hal_arch_init(void)
{
    /* Initialize GDT, IDT, etc. */
}

void nova_hal_arch_cpu_init(void)
{
    /* CPU-specific initialization */
}

void nova_hal_arch_enable_interrupts(void)
{
    __asm__ volatile("sti");
}

void nova_hal_arch_disable_interrupts(void)
{
    __asm__ volatile("cli");
}

void nova_hal_arch_halt(void)
{
    __asm__ volatile("hlt");
}

uint32_t nova_hal_arch_cpu_count(void)
{
    /* TODO: Detect CPU count */
    return 1;
}

uint32_t nova_hal_arch_cpu_id(void)
{
    /* TODO: Get current CPU ID */
    return 0;
}
