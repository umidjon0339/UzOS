#ifndef NET_TICK_H
#define NET_TICK_H

/*
 * net_tick.h - The network stack's clock.
 *
 * The timer interrupt calls net_tick() on every tick. Every
 * NET_TICK_MS it runs the stack's timers: ARP retries and old entries,
 * (later) IP fragments that never completed, TCP retransmissions.
 */

#include <stdint.h>

#define NET_TICK_MS 100         // how often the timers run

// Called by the timer interrupt (device_management/timer), every tick.
void net_tick(void);

// Milliseconds since boot, for timestamps in the stack.
uint32_t net_now_ms(void);

#endif
