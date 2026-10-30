#ifndef SYS_FORK_H
#define SYS_FORK_H
/*
 * sys_fork.h - fork(): a child with a COPY of my memory. Not in UzOS: -1.
 *
 * fork() gives the child its own copy of every user page, so parent and
 * child can change their memory independently. That needs one page
 * directory per process (CR3 switched on every task switch, user pages
 * copied - or shared copy-on-write - kernel pages kept the same in all).
 * UzOS has ONE address space, so there is nothing to copy into. It
 * returns -1 instead of pretending. To start a program, use
 * vfork() + execve(): the child borrows the memory until it execs.
 */
#define SYS_FORK 65         // EAX=65
int sys_fork(void);
#endif
