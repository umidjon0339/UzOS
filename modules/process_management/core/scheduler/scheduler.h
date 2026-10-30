#ifndef SCHEDULER_H
#define SCHEDULER_H

/*
 * scheduler.h - The scheduler: hands the CPU from process to process.
 *
 * The timer interrupt calls scheduler_tick() 100 times a second. Each
 * tick it asks cpu_management/core/priority whether the running
 * process's turn is over; if so, the READY process with the highest
 * priority gets the CPU, and equal priorities take turns ("preemptive
 * priority scheduling with round robin").
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"  // process_t

// Set up the process table, the idle task, and turn the running
// kernel_main into a process. Called once from kernel_main.
void scheduler_init(void);

// From now on the timer may switch tasks.
void scheduler_start(void);

// Called by the timer interrupt: wake sleepers, reap zombies, switch.
void scheduler_tick(void);

// Give the CPU to the next READY process (if there is one).
void scheduler_yield(void);

process_t *scheduler_get_current_process(void);

// The idle task: runs only when nobody else wants the CPU, so its time
// is the CPU's idle time (cpu_management/core/accounting).
process_t *scheduler_idle_process(void);

// Build the first stack of a new process so that switch_task() can
// "resume" it: it will start running at 'first' with interrupts on.
// Returns the stack pointer to save in process_t.esp.
uint32_t scheduler_prepare_stack(uint32_t stack_top, void (*first)(void));

#endif
