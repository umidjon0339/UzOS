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
 *                           (or "nice": a user program with nice > 0)
 *     a process, ring 0  -> kernel code was running        "system"
 *
 * Each tick is charged to that process. Counting ticks is "sampling":
 * a process that runs for 2 ms between ticks is never seen, but over
 * thousands of ticks the shares come out right. Linux's `top` works
 * the same way.
 *
 * Every second the counts become percentages (the "load") and go into
 * a 60-second history (sar, btop).
 *
 * Next to the sampled ticks there is one EXACT clock: on every task
 * switch the scheduler calls cpu_account_switch(), which reads the
 * cycle counter (TSC) and adds the cycles the old process just used.
 * That is the "task clock" of perf, and it counts context switches.
 */

#include <stdint.h>
#include "arch/x86/isr/isr.h"                                    // registers_t
#include "modules/process_management/core/process/process.h"    // process_t

#define CPU_HISTORY_SECONDS 60

// Shares of the last full second, in percent. busy = user + nice + kernel.
typedef struct {
    uint32_t busy;
    uint32_t user;
    uint32_t nice;
    uint32_t kernel;
    uint32_t idle;
} cpu_load_t;

// Ticks since boot, by kind (the columns of /proc/stat's "cpu" line).
typedef struct {
    uint32_t user;          // ring 3, nice <= 0
    uint32_t nice;          // ring 3, nice > 0
    uint32_t system;        // ring 0 (the kernel and kernel processes)
    uint32_t idle;          // the idle task
} cpu_ticks_t;

// One second of history.
typedef struct {
    uint32_t uptime_s;      // which second (since boot)
    cpu_load_t load;        // the shares in that second
    uint32_t interrupts;    // interrupts in that second
    uint32_t switches;      // context switches in that second
    uint32_t runnable;      // processes READY or RUNNING at its end
} cpu_second_t;

// Charge this tick to the process the timer interrupted. Called by the
// timer interrupt (device_management/timer) on every tick.
void cpu_account_tick(const registers_t *regs);

// The scheduler hands the CPU from 'prev' to 'next' (process_management
// core/scheduler, right before the switch).
void cpu_account_switch(process_t *prev, process_t *next);

const cpu_load_t *cpu_load(void);

// CPU time used by 'p' since it started, in milliseconds (sampled).
uint32_t cpu_account_ms(const process_t *p);

// Exact TSC cycles 'p' has had the CPU, the current stretch included.
uint64_t cpu_account_cycles(const process_t *p);

// Since boot: time the CPU spent busy / idle, in milliseconds.
uint32_t cpu_busy_ms(void);
uint32_t cpu_idle_ms(void);

// Since boot: ticks by kind, context switches, interrupts (all IRQ lines).
void cpu_account_totals(cpu_ticks_t *out);
uint32_t cpu_account_switches(void);
uint32_t cpu_account_interrupts(void);

// The history: index 0 = the newest full second. NULL past the end.
int cpu_history_count(void);
const cpu_second_t *cpu_history(int index);

#endif
