#ifndef DEF_CPU_WATCHDOG_H
#define DEF_CPU_WATCHDOG_H

#include <stdint.h>

// 1 if a process that has spent 'used_sec' CPU seconds making no useful
// progress should be stopped, given a soft RLIMIT_CPU of 'limit_sec'
// (RLIM_INFINITY = no limit). UzOS has no way to tell "useful" work
// from a spin, so the watchdog IS the CPU-time limit: the timer tick
// (rlimit_check_cpu, core/rlimit) kills whatever runs too long.
int def_watchdog_over(uint32_t used_sec, uint32_t limit_sec);

#endif
