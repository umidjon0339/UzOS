#ifndef PROC_SYSCALLS_H
#define PROC_SYSCALLS_H

#include <stdint.h>

void sys_sleep(uint32_t ms);
void sys_yield(void);
uint32_t sys_getpid(void);
void sys_exit(void);

#endif
