#ifndef SYS_FORK_H
#define SYS_FORK_H
/*
 * sys_fork.h - fork(): duplicate the calling process.
 *
 * STUB. A real fork() copies the caller's whole address space into a new
 * process (the child returns 0, the parent gets the child's pid). UzOS
 * runs every process in ONE shared address space, so there is nothing
 * private to copy - fork() needs a per-process page directory first (see
 * process_management/README "Known limits"). Returns -1 until then; new
 * processes are made with process_create(name, entry, uid) instead.
 */
#define SYS_FORK 65         // EAX=65
int sys_fork(void);
#endif
