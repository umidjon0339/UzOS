#ifndef SYS_SCHED_GET_PRIORITY_MAX_H
#define SYS_SCHED_GET_PRIORITY_MAX_H

/*
 * sys_sched_get_priority_max.h - sched_get_priority_max(): highest priority of a policy.
 */

#define SYS_SCHED_GET_PRIORITY_MAX 31   // EAX = 31, EBX = policy; the maximum back in EAX

// 99 for SCHED_FIFO / SCHED_RR, 0 for the others, -1 for an unknown policy.
int sys_sched_get_priority_max(int policy);

#endif
