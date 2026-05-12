/**
 * @file hal.h
 * @brief Hardware Abstraction Layer
 */

#ifndef NOVA_HAL_H
#define NOVA_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * HAL Initialization
 * ========================================================================== */

/**
 * Initialize HAL for current platform
 * @return 0 on success
 */
int nova_hal_init(void);

/* ============================================================================
 * CPU Operations
 * ========================================================================== */

/**
 * Initialize CPU
 */
void nova_hal_cpu_init(void);

/**
 * Enable interrupts
 */
void nova_hal_enable_interrupts(void);

/**
 * Disable interrupts
 */
void nova_hal_disable_interrupts(void);

/**
 * Halt CPU (wait for interrupt)
 */
void nova_hal_halt(void);

/**
 * Get CPU count
 * @return Number of CPUs
 */
uint32_t nova_hal_cpu_count(void);

/**
 * Get current CPU ID
 * @return CPU ID
 */
uint32_t nova_hal_cpu_id(void);

/* ============================================================================
 * Timers
 * ========================================================================== */

/**
 * Get timestamp counter
 * @return CPU cycle count
 */
uint64_t nova_hal_rdtsc(void);

/**
 * Initialize timer (PIT/HPET/etc)
 * @param frequency_hz Timer frequency
 * @return 0 on success
 */
int nova_hal_timer_init(uint32_t frequency_hz);

/**
 * Get timer ticks
 * @return Number of timer ticks since boot
 */
uint64_t nova_hal_timer_ticks(void);

/* ============================================================================
 * I/O Operations
 * ========================================================================== */

/**
 * Output byte to port (x86 only)
 * @param port I/O port
 * @param value Value to write
 */
void nova_hal_outb(uint16_t port, uint8_t value);

/**
 * Input byte from port (x86 only)
 * @param port I/O port
 * @return Read value
 */
uint8_t nova_hal_inb(uint16_t port);

/**
 * Memory-mapped I/O write
 * @param addr MMIO address
 * @param value Value to write
 */
void nova_hal_mmio_write32(uintptr_t addr, uint32_t value);

/**
 * Memory-mapped I/O read
 * @param addr MMIO address
 * @return Read value
 */
uint32_t nova_hal_mmio_read32(uintptr_t addr);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_HAL_H */
