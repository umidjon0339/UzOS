#ifndef CPU_ACCOUNT_H
#define CPU_ACCOUNT_H

/*
 * cpu_account.h - Where did the CPU's time go?
 *
 * The CPU itself keeps no record. But the timer interrupts it 100 times a
 * second, and each time we can look at WHO it interrupted and IN WHICH
 * RING (the saved CS):
 *
 *     the idle task      -> the CPU had nothing to do      "idle"
 *     a process, ring 3  -> a user program was computing   "user"
 *     a process, ring 0  -> kernel code was running        "kernel"
 *
 * Each tick is charged to that process. Counting ticks is "sampling":
 * a process that runs for 2 ms between ticks is never seen, but over
 * thousands of ticks the shares come out right. Linux's `top` works
 * the same way.
 *
 * Every second the counts become percentages (the "load").
 */

#include <stdint.h>
#include "isr.h"
#include "process.h"

// Shares of the last full second, in percent. busy = user + kernel.
typedef struct {
    uint32_t busy;
    uint32_t user;
    uint32_t kernel;
    uint32_t idle;
} cpu_load_t;

// Charge this tick to the process the timer interrupted. Called by the
// timer interrupt (device_management/timer) on every tick.
void cpu_account_tick(const registers_t *regs);

const cpu_load_t *cpu_load(void);

// CPU time used by 'p' since it started, in milliseconds.
uint32_t cpu_account_ms(const process_t *p);

// Since boot: time the CPU spent busy / idle, in milliseconds.
uint32_t cpu_busy_ms(void);
uint32_t cpu_idle_ms(void);

#endif
