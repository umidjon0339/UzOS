#ifndef POLICY_H
#define POLICY_H

/*
 * policy.h - Scheduling policies: the RULE a process is scheduled by.
 *
 * Priority (core/priority) says how important a process is. The POLICY
 * says which kind of rule applies to it at all. The numbers are Linux's:
 *
 *   policy        kind        priority        a turn lasts       ps CLS
 *   SCHED_OTHER   normal      nice -20..19    50 ms, then next   TS
 *   SCHED_BATCH   normal      nice -20..19    50 ms; never       B
 *                                             pushes anyone off
 *   SCHED_IDLE    normal      (none)          only when nothing  IDL
 *                                             else wants the CPU
 *   SCHED_FIFO    real-time   1..99           until it sleeps    FF
 *                                             or ends
 *   SCHED_RR      real-time   1..99           100 ms, then the   RR
 *                                             next at its level
 *
 * Real-time processes always run before normal ones. A busy FIFO
 * process would therefore freeze everything, the shell included. So
 * real-time processes get at most 95% of each second; the last 5% go
 * to the normal ones ("RT throttling", as in Linux:
 * /proc/sys/kernel/sched_rt_runtime_us = 950000).
 *
 * SCHED_DEADLINE (6) exists in Linux but not in UzOS: it is refused.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define SCHED_OTHER     0
#define SCHED_FIFO      1
#define SCHED_RR        2
#define SCHED_BATCH     3
#define SCHED_IDLE      5
#define SCHED_DEADLINE  6

// OR'd into the policy: "my children start as SCHED_OTHER, nice 0".
#define SCHED_RESET_ON_FORK 0x40000000

#define RT_PRIORITY_MIN 1
#define RT_PRIORITY_MAX 99
#define RR_SLICE_TICKS  10      // one SCHED_RR turn: 100 ms at 100 Hz
#define RT_RUNTIME_PERCENT 95   // RT throttling: real-time gets 95% of a second

// The argument of sched_setparam() / sched_getparam() (as in Linux).
typedef struct {
    int32_t sched_priority;     // 1..99 for FIFO/RR, 0 for the others
} sched_param_t;

// The argument of sched_setattr() / sched_getattr() (Linux's struct
// sched_attr, first version: 48 bytes).
#define SCHED_ATTR_SIZE 48
#define SCHED_FLAG_RESET_ON_FORK 0x01
typedef struct {
    uint32_t size;              // sizeof(sched_attr_t): tells the version
    uint32_t sched_policy;
    uint64_t sched_flags;       // SCHED_FLAG_RESET_ON_FORK
    int32_t  sched_nice;        // SCHED_OTHER / SCHED_BATCH
    uint32_t sched_priority;    // SCHED_FIFO / SCHED_RR
    uint64_t sched_runtime;     // SCHED_DEADLINE only (not in UzOS)
    uint64_t sched_deadline;
    uint64_t sched_period;
} sched_attr_t;

// 1 if UzOS knows 'policy' (DEADLINE is not supported).
int policy_valid(int policy);

// 1 for SCHED_FIFO and SCHED_RR.
int policy_is_realtime(int policy);

// The priority range of a policy: 1..99 real-time, 0..0 the others.
// -1 if the policy is not valid.
int policy_priority_min(int policy);
int policy_priority_max(int policy);

// "SCHED_FIFO" and the short ps/chrt form "FF". "?" if unknown.
const char *policy_name(int policy);
const char *policy_short(int policy);

// Find a policy by its name ("fifo", "SCHED_FIFO", "rr", ...): -1 if none.
int policy_find(const char *name);

// How long one turn of 'p' lasts, in ticks. 0 = no limit (SCHED_FIFO).
uint32_t policy_slice_ticks(const process_t *p);

// Set 'pid' to 'policy' with real-time priority 'rt_priority', asked by
// 'caller' (NULL = the kernel, treated as root). The defence
// priority_bounds decides. reset_on_fork: 1 = set it, 0 = clear it.
// 1 = done, 0 = refused.
int policy_set(const process_t *caller, uint32_t pid, int policy, int rt_priority,
               int reset_on_fork);

#endif
