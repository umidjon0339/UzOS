#ifndef SYS_SETPRIORITY_H
#define SYS_SETPRIORITY_H

/*
 * sys_setpriority.h - setpriority(): change the nice value of processes.
 */

#include <stdint.h>

#define SYS_SETPRIORITY 21  // EAX = 21, EBX = which, ECX = who, EDX = nice; 0 / -1 back

// 'which' says what 'who' names (Linux values):
#define PRIO_PROCESS 0      // who = a pid (0 = me)
#define PRIO_PGRP    1      // who = a process group: UzOS has none, refused
#define PRIO_USER    2      // who = a uid (0 = my uid): all its processes

// Give every process 'which'/'who' names the nice value 'nice'
// (-20..19). 0 = done for all of them, -1 = refused for at least one.
int32_t sys_setpriority(int which, uint32_t who, int nice);

#endif
