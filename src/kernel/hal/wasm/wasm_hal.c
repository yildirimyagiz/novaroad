/**
 * @file wasm_hal.c
 * @brief WebAssembly HAL stub
 */

#include "wasm_hal.h"

void nova_hal_arch_init(void) {}
void nova_hal_arch_cpu_init(void) {}
void nova_hal_arch_enable_interrupts(void) {}
void nova_hal_arch_disable_interrupts(void) {}
void nova_hal_arch_halt(void) {}
uint32_t nova_hal_arch_cpu_count(void) { return 1; }
uint32_t nova_hal_arch_cpu_id(void) { return 0; }
