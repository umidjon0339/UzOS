#ifndef SYS_EXIT_GROUP_H
#define SYS_EXIT_GROUP_H

/*
 * sys_exit_group.h - exit_group(): end the whole process, every thread.
 */

#define SYS_EXIT_GROUP 51   // EAX = 51, EBX = status

// End all threads of the caller's group, then the caller. Never returns.
// 'status' is not kept: UzOS has no wait() that could collect it.
void sys_exit_group(int status) __attribute__((noreturn));

#endif
