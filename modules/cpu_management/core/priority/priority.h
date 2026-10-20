#ifndef PRIORITY_H
#define PRIORITY_H

/*
 * priority.h - Who gets the CPU next, and for how long.
 *
 * The scheduler (process_management) knows HOW to switch between two
 * processes. This file decides WHICH one and WHEN: the scheduling POLICY.
 *
 *   level 7  ███  runs first                  (only root may give it)
 *   level 4  ███  every new process starts here
 *   level 1  ███  runs when nothing above it is READY
 *   level 0  ███  the idle task only: "nobody needs the CPU"
 *
 * Rules, checked on every timer tick:
 *   1. the READY process with the highest EFFECTIVE priority runs
 *   2. equal priorities take turns (round robin)
 *   3. a turn (time slice) lasts CPU_SLICE_TICKS ticks, then rule 1 again
 *   4. if a better process becomes READY, it takes the CPU at once
 *
 * Strict priorities have one danger: a busy process at level 7 would
 * keep everyone below it off the CPU forever (STARVATION). So a process
 * that waits slowly gains priority: the defence aging. Its effective
 * priority is
 *
 *     base + waited_ticks / AGING_TICKS          (never above 7)
 */

#include <stdint.h>
#include "process.h"

#define PRIORITY_IDLE     0     // the idle task; never aged, never set by anyone
#define PRIORITY_MIN      1
#define PRIORITY_DEFAULT  4
#define PRIORITY_MAX      7

#define CPU_SLICE_TICKS   5     // one turn = 5 ticks = 50 ms at 100 Hz

// The priority 'p' really runs with right now: its base plus aging.
int priority_effective(const process_t *p);

// Rules 1 and 2: the process that should run after 'current' (which
// may be 'current' itself). Never NULL once the idle task exists.
process_t *priority_pick_next(process_t *current);

// Rules 3 and 4, called on every timer tick: count down the running
// process's slice and let the waiting ones age. 1 = switch now.
int priority_tick(process_t *current);

// The scheduler just gave 'p' the CPU: a full slice, no more waiting.
void priority_start_turn(process_t *p);

// Set process 'pid' to 'level', asked by 'caller' (NULL = the kernel,
// treated as root). The defence priority_bounds decides. 1 = done.
int priority_set(const process_t *caller, uint32_t pid, int level);

#endif
