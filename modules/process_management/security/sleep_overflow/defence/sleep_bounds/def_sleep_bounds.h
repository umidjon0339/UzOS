#ifndef DEF_SLEEP_BOUNDS_H
#define DEF_SLEEP_BOUNDS_H

#include <stdint.h>

// The longest wait in ticks: half the tick counter, so "deadline - now"
// stays a positive int32 and every comparison of times stays right.
#define SLEEP_MAX_TICKS 0x7FFFFFFFu

// Milliseconds -> timer ticks, rounded up (never 0 for ms > 0), capped,
// without the overflow of "(ms + 9) / 10". Every sleep uses it.
uint32_t def_sleep_ms_ticks(uint32_t ms);

// A time from a program (timespec: frac in nanoseconds, frac_per_sec =
// 1000000000; timeval: microseconds, 1000000) -> ticks, rounded up and
// capped at SLEEP_MAX_TICKS. 1 = fine, 0 = refused (logged): a negative
// time, or a fraction of a second that is not below one second.
int def_sleep_time_ticks(int32_t sec, int32_t frac, uint32_t frac_per_sec, uint32_t *ticks);

#endif
