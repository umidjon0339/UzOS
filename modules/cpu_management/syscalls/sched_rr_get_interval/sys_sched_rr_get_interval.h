#ifndef SYS_SCHED_RR_GET_INTERVAL_H
#define SYS_SCHED_RR_GET_INTERVAL_H

/*
 * sys_sched_rr_get_interval.h - sched_rr_get_interval(): how long one turn lasts.
 */

#include <stdint.h>

#define SYS_SCHED_RR_GET_INTERVAL 32    // EAX = 32, EBX = pid (0 = me), ECX = &timespec

// A time span, as Linux's struct timespec (32-bit).
typedef struct {
    int32_t tv_sec;
    int32_t tv_nsec;
} timespec_t;

// The length of one turn of 'pid' into *interval: 100 ms for SCHED_RR,
// the normal slice for SCHED_OTHER, 0 for SCHED_FIFO (no end).
// 0 = done, -1 = no such process.
int sys_sched_rr_get_interval(uint32_t pid, timespec_t *interval);

#endif
