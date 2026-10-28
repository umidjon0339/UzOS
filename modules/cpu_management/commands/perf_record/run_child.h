#ifndef PERF_RECORD_RUN_CHILD_H
#define PERF_RECORD_RUN_CHILD_H

/*
 * run_child.h - Run a command line as a NEW process and wait for it.
 *
 * Normally the shell runs a command inside its own process (the shell
 * is kernel_main). This command must not change the shell, so it makes
 * a CHILD process that runs the command line, and waits for it, like
 * fork() + exec() + wait() in Unix.
 *
 * Every command that starts a child has its own copy of this file, so
 * each folder can be read on its own.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// Run 'line' ("ls -l") as a child process named after its first word.
// Returns the child's pid, or -1 if it could not be started (unknown
// command, no free slot).
int perf_record_run_child(const char *line);

#endif
