#ifndef TSC_H
#define TSC_H

/*
 * tsc.h - The CPU's cycle counter, and how fast the CPU really runs.
 *
 * The Time Stamp Counter goes up by one every CPU clock cycle. The CPU
 * does not tell us its clock speed, so we MEASURE it against a clock we
 * do know, the PIT (100 ticks per second):
 *
 *     wait for a tick edge          start = rdtsc
 *     wait 10 more ticks (100 ms)   end   = rdtsc
 *     MHz = (end - start) * 10 / 1,000,000
 *
 * Afterwards the kernel can time things far more finely than the PIT:
 * one tick is 10 ms, one cycle is well under a nanosecond.
 *
 * Ring 3 does not get this clock: see the defence tsc_restrict.
 */

#include <stdint.h>

#define TSC_CALIBRATE_TICKS 10      // measure over 10 PIT ticks = 100 ms

// Measure the clock speed and switch on tsc_restrict. Needs the timer
// running and interrupts on. Called once from kernel_main.
void tsc_init(void);

// 1 if the CPU has a TSC (cpuid feature "tsc").
int tsc_available(void);

// Measured clock speed in MHz; 0 if there is no TSC.
uint32_t tsc_mhz(void);

// Cycles since power on; 0 if there is no TSC.
uint64_t tsc_now(void);

// How long 'cycles' took, in microseconds / nanoseconds.
uint32_t tsc_cycles_to_us(uint64_t cycles);
uint32_t tsc_cycles_to_ns(uint64_t cycles);

#endif
