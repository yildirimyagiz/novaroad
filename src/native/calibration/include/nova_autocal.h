#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * 🦅 Nova Sovereign Calibration — Benchmark & Autotuning Context
 * Used by the bench suite to measure kernel performance.
 */
typedef struct AutocalContext AutocalContext;

typedef struct {
    const char *name;            // Name of the workload
    void (*setup)(void *ctx);    // (Optional) Setup before benchmark
    void (*execute)(void *ctx);  // Main kernel workload
    void (*teardown)(void *ctx); // (Optional) Teardown after benchmark
    void *context;               // External context passed to functions
    int iterations;              // Number of iterations for averaging
} AutocalWorkload;

/* Benchmark Engine Control */
AutocalContext *autocal_create(void);
void autocal_destroy(AutocalContext *ctx);

/* Measure execution time with averaging and warmup */
double autocal_measure_time(AutocalWorkload workload);

/* Main platform-wide calibration entry point */
void nova_autocal_run(void);
