/**
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                    EVRENSEL OPTİMİZASYON MOTORU (Header)                      ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 */

#ifndef NOVA_UNIVERSAL_OPTIMIZER_H
#define NOVA_UNIVERSAL_OPTIMIZER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ═══════════════════════════════════════════════════════════════════════════════
// TYPES
// ═══════════════════════════════════════════════════════════════════════════════

typedef struct {
    double tesla_factor;
    double phi_factor;
    double pi_factor;
} NovaUniversalOptimizer;

// ═══════════════════════════════════════════════════════════════════════════════
// TESLA 3-6-9 FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

int nova_tesla_digital_root(int64_t n);
bool nova_is_tesla_harmonic(int64_t n);
int64_t nova_round_to_tesla(int64_t n);

// ═══════════════════════════════════════════════════════════════════════════════
// GOLDEN RATIO FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

void nova_golden_partition(double total, double *larger, double *smaller);
size_t nova_fibonacci_hash(uint64_t key, size_t table_size);

// ═══════════════════════════════════════════════════════════════════════════════
// PI-BASED FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════

size_t nova_pi_aligned_size(size_t requested);

// ═══════════════════════════════════════════════════════════════════════════════
// UNIVERSAL OPTIMIZER
// ═══════════════════════════════════════════════════════════════════════════════

NovaUniversalOptimizer nova_universal_optimizer_new(void);
double nova_universal_optimize(NovaUniversalOptimizer *opt, double value);
size_t nova_optimize_buffer_size(NovaUniversalOptimizer *opt, size_t requested);
size_t nova_optimize_thread_count(NovaUniversalOptimizer *opt, size_t cpu_count);

// ═══════════════════════════════════════════════════════════════════════════════
// CACHE & FREQUENCY
// ═══════════════════════════════════════════════════════════════════════════════

size_t nova_optimize_cache_line_size(size_t requested);
double nova_harmonic_frequency(int64_t value);
double nova_schumann_timing(size_t operations);

#endif // NOVA_UNIVERSAL_OPTIMIZER_H
