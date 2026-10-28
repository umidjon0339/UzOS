#ifndef SYS_SCHED_YIELD_H
#define SYS_SCHED_YIELD_H

/*
 * sys_sched_yield.h - sched_yield(): "I am done for now, let someone else run."
 */

#define SYS_SCHED_YIELD 4   // int 0x80 with EAX = 4; 0 back in EAX

// Give the rest of this turn away. Always succeeds.
int sys_sched_yield(void);

#endif
