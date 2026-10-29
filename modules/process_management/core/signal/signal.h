#ifndef SIGNAL_H
#define SIGNAL_H

/*
 * signal.h - Tell a process that something happened.
 *
 * A signal is a small message to a process: one bit in process_t.signals.
 * Sending it does not stop anything by itself; the process looks at its
 * signals at the moments it chooses, and decides what to do.
 *
 *   Ctrl+C  -> the keyboard interrupt -> the shell -> signal_send(job, SIGINT)
 *   the job -> a sleep that may be interrupted ends early
 *           -> the command sees signal_interrupted() and stops
 *
 * Two kinds of sleep (as in Linux):
 *   sys_sleep()                 uninterruptible: always sleeps the full time
 *   sys_sleep_interruptible()   a signal wakes it at once
 */

#include "process.h"

#define SIGINT 2        // "interrupt", sent by Ctrl+C (the same number as Linux)
#define SIGKILL 9       // forced termination: cannot be caught or ignored
#define SIGTERM 15      // polite request to terminate (may be caught)
#define SIGHUP  1       // the controlling terminal went away

// Mark 'sig' pending on 'p', and wake it if it is in an interruptible
// sleep. Safe to call from an interrupt handler.
void signal_send(process_t *p, int sig);

// Is 'sig' pending on 'p'?
int signal_pending(const process_t *p, int sig);

// Forget 'sig': it has been dealt with.
void signal_clear(process_t *p, int sig);

// Has the running process been sent SIGINT? (For loops that should stop
// on Ctrl+C: `while (... && !signal_interrupted())`.)
int signal_interrupted(void);

#endif
