/**
 * @file cpu.c
 * @brief aarch64 CPU initialization
 */

#include "cpu.h"

void nova_hal_arch_init(void)
{
    /* ARM64 initialization */
}

void nova_hal_arch_cpu_init(void)
{
    /* CPU-specific initialization */
}

void nova_hal_arch_enable_interrupts(void)
{
    __asm__ volatile("msr daifclr, #2");
}

void nova_hal_arch_disable_interrupts(void)
{
    __asm__ volatile("msr daifset, #2");
}

void nova_hal_arch_halt(void)
{
    __asm__ volatile("wfi");
}

uint32_t nova_hal_arch_cpu_count(void)
{
    return 1;
}

uint32_t nova_hal_arch_cpu_id(void)
{
    return 0;
}
