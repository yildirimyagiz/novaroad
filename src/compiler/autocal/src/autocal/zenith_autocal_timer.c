/**
 * 🕐 zenith_autocal_timer.c - High-Resolution Timer for Auto-calibration
 */

#include "../../include/zenith_autocal_timer.h"
#include <stdio.h>
#include <time.h>

static struct timespec g_timer_start;

void zenith_autocal_timer_start(void)
{
    clock_gettime(CLOCK_MONOTONIC, &g_timer_start);
}

double zenith_autocal_timer_stop_ms(void)
{
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed =
        (end.tv_sec - g_timer_start.tv_sec) * 1000.0 + (end.tv_nsec - g_timer_start.tv_nsec) / 1e6;
    return elapsed;
}

double zenith_timer_get_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
