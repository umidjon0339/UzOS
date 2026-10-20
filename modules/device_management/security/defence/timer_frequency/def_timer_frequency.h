#ifndef DEF_TIMER_FREQUENCY_H
#define DEF_TIMER_FREQUENCY_H

#include <stdint.h>

// Lowest rate whose divisor still fits 16 bits: ceil(1193180 / 65535) = 19 Hz.
#define PIT_MIN_HZ ((1193180u + 65534u) / 65535u)

// Can the PIT run at 'hz'? 1 = yes, 0 = refused (logged).
int def_timer_frequency_ok(uint32_t hz);

#endif
