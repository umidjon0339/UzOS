#ifndef CMD_STRACE_H
#define CMD_STRACE_H

/*
 * cmd_strace.h - `strace command [args]` | `strace -p pid` | `strace -d pid`
 *
 * Show every syscall a program makes, with its arguments and answer.
 */

void cmd_strace(const char *args);

#endif
