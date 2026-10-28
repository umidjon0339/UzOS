#ifndef DEF_CPU_TIME_LIMIT_H
#define DEF_CPU_TIME_LIMIT_H

#include <stdint.h>

// 1 if a process that has used 'used_sec' CPU seconds is OVER a soft
// limit of 'limit_sec' (RLIM_INFINITY = no limit, so never over). The
// kernel makes this exact check in the timer tick, through
// rlimit_check_cpu() (core/rlimit); over the limit, the process dies.
int def_cpu_over_limit(uint32_t used_sec, uint32_t limit_sec);

#endif
