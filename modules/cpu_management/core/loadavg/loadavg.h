#ifndef LOADAVG_H
#define LOADAVG_H

/*
 * loadavg.h - The load average: how many processes want the CPU.
 *
 * Every 5 seconds the kernel counts the processes that are RUNNING or
 * READY (the idle task does not count) and blends that number into
 * three running averages, as Linux does:
 *
 *     load = load * e + count * (1 - e)
 *
 * with a different 'e' for each: the 1-minute average follows changes
 * quickly, the 15-minute one slowly. A load of 1.00 on one CPU means
 * "always exactly one process wanted the CPU": fully used, nobody had
 * to wait. 2.00 means half the time someone was waiting.
 *
 * The numbers are fixed point (no floating point in the kernel):
 * 2048 = 1.00. uptime, top, htop and /proc/loadavg show them.
 */

#include <stdint.h>

#define LOADAVG_SHIFT 11                    // fixed point: 1 << 11 = 2048 = 1.00
#define LOADAVG_ONE   (1 << LOADAVG_SHIFT)
#define LOADAVG_SAMPLE_SECONDS 5

// Called on every timer tick (core/accounting): every 5 s, sample.
void loadavg_tick(void);

// The three averages (1, 5, 15 minutes) in hundredths: 42 = 0.42.
void loadavg_get(uint32_t hundredths[3]);

// "0.42": one average as text.
void loadavg_format(uint32_t hundredths, char *out, int size);

// Processes RUNNING or READY right now (the idle task not counted).
uint32_t loadavg_runnable(void);

#endif
