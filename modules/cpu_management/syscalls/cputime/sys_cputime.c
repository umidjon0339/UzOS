/*
 * sys_cputime.c - cputime(): CPU time used, not wall-clock time.
 *
 * uptime() says how long the SYSTEM has run. cputime() says how long a
 * PROCESS actually had the CPU. A program that sleeps 10 s uses almost
 * no CPU time; a busy loop uses all of it. (Like clock() / times() in C.)
 *
 * Uses: cpu_account_ms (core/accounting), process_get_by_pid and the
 *       current process (process_management).
 */

#include "modules/cpu_management/syscalls/cputime/sys_cputime.h"
#include "modules/cpu_management/core/accounting/cpu_account.h"
#include "modules/process_management/core/scheduler/scheduler.h"

int32_t sys_cputime(uint32_t pid) {
    process_t *p = pid ? process_get_by_pid(pid) : scheduler_get_current_process();
    if (!p)
        return -1;
    return (int32_t)cpu_account_ms(p);
}
