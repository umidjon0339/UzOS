#ifndef NOHUP_START_CHILD_H
#define NOHUP_START_CHILD_H

/*
 * start_child.h - Start a command line as a new process, do not wait.
 *
 * The shell runs a command inside its own process. `nohup` must run it
 * apart, in the background, so it makes a CHILD process that runs the
 * line (like fork() + exec() in Unix) and returns at once. A setup
 * function changes the child's settings before it may run.
 *
 * Every command that starts a child has its own copy of this file, so
 * each folder can be read on its own.
 */

#include "modules/process_management/core/process/process.h"   // process_t

// Run 'line' ("sleep 30") as a child named after its first word, after
// setup(child). Returns the child's pid, or -1 (unknown command, no slot).
int nohup_start_child(const char *line, void (*setup)(process_t *child));

#endif
