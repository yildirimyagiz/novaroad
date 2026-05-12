/**
 * 🕐 zenith_autocal_timer.h - High-Resolution Timer Interface
 */

#ifndef ZENITH_AUTOCAL_TIMER_H
#define ZENITH_AUTOCAL_TIMER_H

void zenith_autocal_timer_start(void);
double zenith_autocal_timer_stop_ms(void);
double zenith_timer_get_sec(void);  // Get current time in seconds

#endif // ZENITH_AUTOCAL_TIMER_H
