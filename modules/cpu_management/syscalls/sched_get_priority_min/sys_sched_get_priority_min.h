#ifndef SYS_SCHED_GET_PRIORITY_MIN_H
#define SYS_SCHED_GET_PRIORITY_MIN_H

/*
 * sys_sched_get_priority_min.h - sched_get_priority_min(): lowest priority of a policy.
 */

#define SYS_SCHED_GET_PRIORITY_MIN 30   // EAX = 30, EBX = policy; the minimum back in EAX

// 1 for SCHED_FIFO / SCHED_RR, 0 for the others, -1 for an unknown policy.
int sys_sched_get_priority_min(int policy);

#endif
