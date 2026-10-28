#ifndef NUMACTL_RUN_CHILD_H
#define NUMACTL_RUN_CHILD_H

/*
 * run_child.h - Run a command line as a NEW process and wait for it.
 *
 * Normally the shell runs a command inside its own process (the shell
 * is kernel_main). This command must not change the shell, so it makes
 * a CHILD process that runs the command line, and waits for it, like
 * fork() + exec() + wait() in Unix.
 * Before the child may run, a setup function changes ITS settings
 * (here: give it the memory policy / CPUs).
 *
 * Every command that starts a child has its own copy of this file, so
 * each folder can be read on its own.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// Change the child's settings before it runs. Return 0 to refuse: the
// child then never runs and numactl_run_child() returns -1.
typedef int (*numactl_setup_t)(process_t *child, void *arg);

// Run 'line' ("ls -l") as a child process named after its first word.
// Returns the child's pid, or -1 if it could not be started (unknown
// command, no free slot, setup refused).
int numactl_run_child(const char *line, numactl_setup_t setup, void *arg);

#endif
