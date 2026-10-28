#ifndef SYS_PERF_EVENT_OPEN_H
#define SYS_PERF_EVENT_OPEN_H

/*
 * sys_perf_event_open.h - perf_event_open(): start counting an event.
 */

#include <stdint.h>
#include "modules/cpu_management/syscalls/perf_event_open/perf_counter.h"   // PERF_TYPE_*, PERF_COUNT_*

// The argument of perf_event_open(): the start of Linux's struct
// perf_event_attr (UzOS reads only these fields).
typedef struct {
    uint32_t type;              // PERF_TYPE_*
    uint32_t size;              // sizeof the struct the caller uses
    uint64_t config;            // which event of that type
} perf_event_attr_t;

#define SYS_PERF_EVENT_OPEN 41  // EAX = 41, EBX = &attr, ECX = pid, EDX = cpu, ESI = group_fd, EDI = flags

// Count attr's event for 'pid' (0 = me, -1 = every process: needs
// root) on 'cpu' (-1 = any, or 0). group_fd must be -1, flags 0.
// Returns a file descriptor: read 8 bytes from it to get the count.
// -1 if the event is not supported, or the request is refused.
int sys_perf_event_open(const perf_event_attr_t *attr, int pid, int cpu,
                        int group_fd, uint32_t flags);

#endif
