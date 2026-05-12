// 🦅 Nova Sovereign Calibration — High-Precision Timer Implementation
// Platform-specific low-overhead timing.

#include "../../include/nova_autocal_timer.h"

#ifdef __APPLE__
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

uint64_t nova_now_ns(void)
{
#ifdef __APPLE__
    static mach_timebase_info_data_t tb;
    if (tb.denom == 0)
        mach_timebase_info(&tb);
    return mach_absolute_time() * tb.numer / tb.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

double nova_now_ms(void)
{
    return (double) nova_now_ns() / 1000000.0;
}
