#ifndef SYS_YIELD_H
#define SYS_YIELD_H

/*
 * sys_yield.h - yield(): "I am done for now, let someone else run."
 */

#define SYS_YIELD 4     // int 0x80 with EAX = 4

void sys_yield(void);

#endif
