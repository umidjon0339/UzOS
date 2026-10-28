#ifndef TIME_RUN_CHILD_H
#define TIME_RUN_CHILD_H

/*
 * run_child.h - Run a command line as a NEW process and wait for it.
 *
 * Normally the shell runs a command inside its own process (the shell
 * is kernel_main). This command must not change the shell, so it makes
 * a CHILD process that runs the command line, and waits for it, like
 * fork() + exec() + wait() in Unix.
 * Afterwards it reports what the child cost (its usage record, kept by
 * cpu_management/core/task), like wait4() in Unix.
 *
 * Every command that starts a child has its own copy of this file, so
 * each folder can be read on its own.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t
#include "modules/cpu_management/core/task/cpu_task.h"         // cpu_usage_t

// What time_run_child() measured.
typedef struct {
    int         pid;            // the child's pid
    uint32_t    real_ms;        // wall-clock time from start to end
    uint64_t    real_cycles;    // the same, in TSC cycles (finer)
    cpu_usage_t usage;          // CPU it used (with its own children)
} time_result_t;

// Run 'line' ("ls -l") as a child process named after its first word.
// Returns the child's pid, or -1 if it could not be started (unknown
// command, no free slot).
int time_run_child(const char *line, time_result_t *result);

#endif
