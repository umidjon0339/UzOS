#ifndef SYS_EXIT_H
#define SYS_EXIT_H

/*
 * sys_exit.h - exit(code): end the calling process.
 */

#define SYS_EXIT 1      // int 0x80 with EAX = 1, EBX = exit code (0 = success)

void sys_exit(int code) __attribute__((noreturn));

#endif
