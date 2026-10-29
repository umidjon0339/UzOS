#ifndef SYS_VFORK_H
#define SYS_VFORK_H
/*
 * sys_vfork.h - vfork(): like fork(), but the child borrows the parent's
 * address space and the parent is suspended until the child exits or
 * execs. STUB for the same reason as fork(): UzOS has one shared address
 * space, so there is no separate space to borrow or restore. Returns -1.
 */
#define SYS_VFORK 66        // EAX=66
int sys_vfork(void);
#endif
