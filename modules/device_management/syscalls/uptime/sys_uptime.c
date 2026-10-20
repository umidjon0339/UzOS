/*
 * sys_uptime.c - uptime(): read the system clock.
 *
 * A program cannot read the PIT itself (ports are kernel-only), so it
 * asks the kernel. Programs use this to measure time or animate things.
 *
 * Uses: timer_uptime_ms (timer/).
 */

#include "sys_uptime.h"
#include "timer.h"

uint32_t sys_uptime(void) {
    return timer_uptime_ms();
}
