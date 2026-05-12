/**
 * @file cpu.c
 * @brief RISC-V CPU initialization
 */

#include "cpu.h"

void nova_hal_arch_init(void)
{
    /* RISC-V initialization */
}

void nova_hal_arch_cpu_init(void)
{
    /* CPU-specific initialization */
}

void nova_hal_arch_enable_interrupts(void)
{
    __asm__ volatile("csrs mstatus, %0" :: "r"(0x8));
}

void nova_hal_arch_disable_interrupts(void)
{
    __asm__ volatile("csrc mstatus, %0" :: "r"(0x8));
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
