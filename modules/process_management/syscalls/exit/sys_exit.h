#ifndef SYS_EXIT_H
#define SYS_EXIT_H

/*
 * sys_exit.h - exit(): end the calling process.
 */

#define SYS_EXIT 1      // int 0x80 with EAX = 1

void sys_exit(void) __attribute__((noreturn));

#endif
