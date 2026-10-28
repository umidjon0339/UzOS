#ifndef PRIORITY_H
#define PRIORITY_H

/*
 * priority.h - Who gets the CPU next, and for how long.
 *
 * The scheduler (process_management) knows HOW to switch between two
 * processes. This file decides WHICH one and WHEN: the scheduling POLICY.
 *
 * A user sets a NICE value, as in Unix: -20 = "give me the most CPU",
 * 19 = "I am nice to the others". The scheduler turns it into one of
 * seven LEVELS:
 *
 *   nice -20 .. -16   level 7  ███  runs first        (only root may lower nice)
 *   nice -15 .. -10   level 6
 *   nice  -9 .. -4    level 5
 *   nice  -3 ..  1    level 4  ███  nice 0: every new process
 *   nice   2 ..  7    level 3
 *   nice   8 .. 13    level 2
 *   nice  14 .. 19    level 1  ███  runs when nothing above is READY
 *                     level 0  ███  the idle task only
 *
 * Rules, checked on every timer tick:
 *   1. the READY process with the highest RANK runs (see priority_rank)
 *   2. equal ranks take turns (round robin)
 *   3. a turn (time slice) lasts CPU_SLICE_TICKS ticks, then rule 1 again
 *      (other lengths for some policies and power profiles: core/policy)
 *   4. if a better process becomes READY, it takes the CPU at once
 *
 * Strict priorities have one danger: a busy process at level 7 would
 * keep everyone below it off the CPU forever (STARVATION). So a process
 * that waits slowly gains priority: the defence aging. Its effective
 * level is
 *
 *     level + waited_ticks / AGING_TICKS          (never above 7)
 *
 * Real-time processes (SCHED_FIFO / SCHED_RR, core/policy) rank above
 * every normal one, but get at most 95% of each second (RT throttling).
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define PRIORITY_IDLE     0     // the idle task; never aged, never set by anyone
#define PRIORITY_MIN      1
#define PRIORITY_DEFAULT  4
#define PRIORITY_MAX      7

#define NICE_MIN         -20
#define NICE_MAX          19
#define NICE_DEFAULT      0

#define CPU_SLICE_TICKS   5     // one turn = 5 ticks = 50 ms at 100 Hz

// The level (1-7) a nice value (-20..19) gives.
int priority_level_of_nice(int nice);

// The usual nice value for a level (1 -> 19, 4 -> 0, 7 -> -20).
int priority_nice_of_level(int level);

// The level 'p' really runs with right now: its level plus aging.
int priority_effective(const process_t *p);

// The one number the scheduler compares (bigger runs first):
//   real-time        100 + rt_priority        101 .. 199
//   normal           effective level * 10      10 .. 70
//   SCHED_IDLE       5
//   the idle task    0
int priority_rank(const process_t *p);

// Rules 1 and 2: the process that should run after 'current' (which
// may be 'current' itself). Never NULL once the idle task exists.
process_t *priority_pick_next(process_t *current);

// Rules 3 and 4, called on every timer tick: count down the running
// process's slice and let the waiting ones age. 1 = switch now.
int priority_tick(process_t *current);

// The scheduler just gave 'p' the CPU: a full slice, no more waiting.
void priority_start_turn(process_t *p);

// Set the nice value of process 'pid', asked by 'caller' (NULL = the
// kernel, treated as root). The defence priority_bounds decides.
// 1 = done, 0 = refused.
int priority_set_nice(const process_t *caller, uint32_t pid, int nice);

// 1 while real-time processes have used up their 95% of this second.
int priority_rt_throttled(void);

// How many times RT throttling has stepped in since boot.
uint32_t priority_rt_throttle_count(void);

#endif
