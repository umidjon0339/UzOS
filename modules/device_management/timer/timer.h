#ifndef TIMER_H
#define TIMER_H

/*
 * timer.h - PIT (Programmable Interval Timer), IRQ 0.
 *
 * The PIT counts down from a "divisor" at 1,193,180 Hz and raises IRQ 0
 * each time it reaches zero. divisor = 1193180 / frequency, so at 100 Hz
 * the kernel gets a "tick" every 10 ms. Each tick runs the scheduler.
 */

#include <stdint.h>

#define TIMER_HZ     100            // the rate kernel_main asks for
#define PIT_INPUT_HZ 1193180u       // the PIT's own clock

extern volatile uint32_t timer_ticks;   // ticks since the timer started

// Start the PIT at 'frequency' ticks per second. Bad values are refused
// (timer_frequency) and the timer keeps its old setting.
void init_timer(uint32_t frequency);

uint32_t timer_frequency(void);         // current ticks per second (0 = not started)
uint32_t timer_uptime_ms(void);         // milliseconds since the timer started

// Sleep (hlt) until 'ticks' more ticks have passed. Interrupts must be on.
void timer_wait(uint32_t ticks);

#endif
