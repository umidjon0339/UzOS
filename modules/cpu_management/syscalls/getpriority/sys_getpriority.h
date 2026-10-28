#ifndef SYS_GETPRIORITY_H
#define SYS_GETPRIORITY_H

/*
 * sys_getpriority.h - getpriority(): read a nice value.
 */

#include <stdint.h>
#include "modules/cpu_management/syscalls/setpriority/sys_setpriority.h"   // PRIO_*

#define SYS_GETPRIORITY 22  // EAX = 22, EBX = which, ECX = who; 20 - nice back in EAX

// 20 - nice of the process(es) 'which'/'who' names: 1 (nice 19) .. 40
// (nice -20). For PRIO_USER the lowest nice of the user's processes.
// -1 if nothing matches.
int32_t sys_getpriority(int which, uint32_t who);

#endif
