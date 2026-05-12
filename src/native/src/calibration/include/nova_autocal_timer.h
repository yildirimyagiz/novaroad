// 🦅 Novaign Calibration — High-Precision Timer
// Nanosecond resolution timing for microkernel benchmarking.

#pragma once

#include <stdint.h>

/// Returns the current absolute time in nanoseconds.
uint64_t nova_now_ns(void);

/// Returns the current absolute time in milliseconds.
double nova_now_ms(void);
