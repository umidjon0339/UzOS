/*
 * cpu_task.c - Inherit CPU settings at birth; collect usage at death.
 *
 * Uses: priority_level_of_nice (core/priority), SCHED_* (core/policy),
 *       affinity_online_mask (core/affinity), rlimit_defaults
 *       (core/rlimit), numa_task_fork (core/numa), tsc_now (core/tsc), timer_ticks
 *       (device_management), the process table (process_management).
 */

#include "modules/cpu_management/core/task/cpu_task.h"          // this part
#include "modules/cpu_management/core/priority/priority.h"      // priority_level_of_nice
#include "modules/cpu_management/core/policy/policy.h"          // SCHED_OTHER
#include "modules/cpu_management/core/affinity/affinity.h"      // affinity_online_mask
#include "modules/cpu_management/core/rlimit/rlimit.h"          // rlimit_defaults
#include "modules/cpu_management/core/numa/numa.h"              // numa_task_fork
#include "modules/cpu_management/core/tsc/tsc.h"                // tsc_now
#include "modules/device_management/timer/timer.h"              // timer_ticks
#include "lib/string/string.h"                                  // strncpy, memset

static cpu_usage_t records[CPU_TASK_RECORDS];
static int next_record;
static uint32_t forks;

void cpu_task_fork(process_t *child, const process_t *parent) {
    forks++;
    child->ppid       = parent ? parent->pid : 0;
    child->tgid       = child->pid;         // clone(CLONE_THREAD) changes it
    child->start_tick = timer_ticks;
    child->cycles_since = tsc_now();     // its exact CPU clock starts now

    // Scheduling: the parent's, unless it asked for a reset.
    if (parent && !parent->reset_on_fork) {
        child->nice        = parent->nice;
        child->policy      = parent->policy;
        child->rt_priority = parent->rt_priority;
        child->reset_on_fork = 0;
    } else {
        child->nice        = NICE_DEFAULT;
        child->policy      = SCHED_OTHER;
        child->rt_priority = 0;
    }
    child->priority = priority_level_of_nice(child->nice);
    child->cpu_mask = parent ? parent->cpu_mask : affinity_online_mask();

    // Limits: the parent's table, or the defaults.
    if (parent) {
        memcpy(child->rlim_cur, parent->rlim_cur, sizeof(child->rlim_cur));
        memcpy(child->rlim_max, parent->rlim_max, sizeof(child->rlim_max));
    } else {
        rlimit_defaults(child);
    }

    numa_task_fork(child, parent);
}

// Where the totals of a finished process go: its parent if alive, else
// the parent's own record (the reaper may free the parent first).
static void give_to_parent(uint32_t ppid, const cpu_usage_t *u) {
    process_t *parent = ppid ? process_get_by_pid(ppid) : 0;
    if (parent) {
        parent->child_user_ticks   += u->user_ticks;
        parent->child_kernel_ticks += u->kernel_ticks;
        parent->child_nvcsw  += u->nvcsw;
        parent->child_nivcsw += u->nivcsw;
        parent->child_cycles += u->cycles;
        return;
    }
    for (int i = 0; i < CPU_TASK_RECORDS; i++) {
        cpu_usage_t *r = &records[i];
        if (ppid && r->pid == ppid) {
            r->user_ticks   += u->user_ticks;
            r->kernel_ticks += u->kernel_ticks;
            r->nvcsw  += u->nvcsw;
            r->nivcsw += u->nivcsw;
            r->cycles += u->cycles;
            return;
        }
    }
}

void cpu_task_exit(process_t *p) {
    cpu_usage_t *u = &records[next_record];
    next_record = (next_record + 1) % CPU_TASK_RECORDS;

    memset(u, 0, sizeof(*u));
    u->pid = p->pid;
    strncpy(u->name, p->name, sizeof(u->name) - 1);
    u->user_ticks   = p->user_ticks   + p->child_user_ticks;
    u->kernel_ticks = p->kernel_ticks + p->child_kernel_ticks;
    u->nvcsw  = p->nvcsw  + p->child_nvcsw;
    u->nivcsw = p->nivcsw + p->child_nivcsw;
    u->cycles = p->cycles + p->child_cycles;

    give_to_parent(p->ppid, u);
}

int cpu_task_usage(uint32_t pid, cpu_usage_t *out) {
    for (int i = 0; i < CPU_TASK_RECORDS; i++) {
        if (records[i].pid == pid && pid) {
            *out = records[i];
            return 1;
        }
    }
    return 0;
}

uint32_t cpu_task_forks(void) {
    return forks;
}
