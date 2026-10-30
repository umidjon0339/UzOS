#ifndef TIMEOUT_START_CHILD_H
#define TIMEOUT_START_CHILD_H

/*
 * start_child.h - Start a command line as a new process.
 *
 * The shell runs a command inside its own process. `timeout` must be
 * able to end it, so it runs it in a CHILD process (like fork() + exec()
 * in Unix); this returns at once and cmd_timeout.c watches the clock.
 *
 * Every command that starts a child has its own copy of this file, so
 * each folder can be read on its own.
 */

#include "modules/process_management/core/process/process.h"   // process_t

// Run 'line' ("sleep 30") as a child named after its first word, after
// setup(child). Returns the child's pid, or -1 (unknown command, no slot).
int timeout_start_child(const char *line, void (*setup)(process_t *child));

#endif
