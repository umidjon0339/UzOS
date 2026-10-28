#ifndef CPU_TASK_H
#define CPU_TASK_H

/*
 * cpu_task.h - A process's CPU settings at its birth and its death.
 *
 * BIRTH. A new process copies its parent's CPU settings, as fork()
 * does in Linux. So `nice -n 10 spawn` gives the worker nice 10, and
 * `ulimit -t 2` in the shell limits every program started after it:
 *
 *   copied from the parent           set fresh
 *   nice, policy, rt priority        ppid = the parent's pid
 *   CPU affinity mask                tgid = its own pid (a new group)
 *   resource limits                  start tick, all counters 0
 *   memory policy
 *
 * With SCHED_RESET_ON_FORK set on the parent, the child starts as a
 * plain SCHED_OTHER process with nice 0 instead.
 * A process with no parent (idle, kernel_main) gets the defaults.
 *
 * DEATH. When the reaper frees a finished process, what it used is
 * added to its parent ("children" time, like Linux's getrusage) and
 * kept in a small list of finished processes. `time` and `perf stat`
 * read that record after their command ended: it is how a parent learns
 * what its child cost (wait4() in Linux).
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define CPU_TASK_RECORDS 16     // finished processes remembered

// What a finished process used, its own children included.
typedef struct {
    uint32_t pid;
    char     name[16];
    uint32_t user_ticks;        // ring 3 ticks
    uint32_t kernel_ticks;      // ring 0 ticks
    uint32_t nvcsw, nivcsw;     // voluntary / involuntary context switches
    uint64_t cycles;            // exact TSC cycles on the CPU
} cpu_usage_t;

// Called by process_create() for every new process, after its pid is
// set and before it may run. 'parent' NULL = no parent.
void cpu_task_fork(process_t *child, const process_t *parent);

// Called by the reaper right before it frees 'p'.
void cpu_task_exit(process_t *p);

// The record of finished process 'pid'. 1 = found.
int cpu_task_usage(uint32_t pid, cpu_usage_t *out);

// Processes created since boot (/proc/stat "processes").
uint32_t cpu_task_forks(void);

#endif
