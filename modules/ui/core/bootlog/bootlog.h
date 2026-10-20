#ifndef BOOTLOG_H
#define BOOTLOG_H

/*
 * bootlog.h - Linux-style boot messages, one line per step.
 *
 *     boot_begin("Memory manager");         [ .... ] Memory manager
 *     ...the step runs...
 *     boot_ok("127 MB, paging on");         [  OK  ] Memory manager  127 MB, paging on
 *
 * The line is shown BEFORE the step runs, so if the step hangs or
 * crashes you can still see which one it was. When it finishes, '\r'
 * goes back to the start of the line and the result is written over it.
 */

void boot_begin(const char *what);
void boot_ok(const char *detail);
void boot_skip(const char *reason);         // could not be done, but the OS runs on
void boot_fail(const char *reason) __attribute__((noreturn));  // stops the OS

#endif
