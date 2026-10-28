#ifndef DEF_RT_THROTTLING_H
#define DEF_RT_THROTTLING_H

#include <stdint.h>

// Real-time (SCHED_FIFO/RR) tasks run before every normal task, so one
// that never sleeps could freeze the machine. The kernel gives all RT
// work at most RT_BUDGET_PERCENT of each second (priority_tick,
// core/priority); past that it lets normal tasks run.
#define RT_BUDGET_PERCENT 95

// 1 if 'rt_ticks' of real-time running in a second of 'hz' ticks is
// still inside the budget (so RT may keep the CPU this tick).
int def_rt_within_budget(uint32_t rt_ticks, uint32_t hz);

#endif
