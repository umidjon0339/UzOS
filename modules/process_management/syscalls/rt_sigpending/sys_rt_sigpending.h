#ifndef SYS_RT_SIGPENDING_H
#define SYS_RT_SIGPENDING_H
/* sys_rt_sigpending.h - rt_sigpending(set, sigsetsize): which signals wait. */
#include <stdint.h>
#define SYS_RT_SIGPENDING 71    // EAX=71, EBX=set, ECX=sigsetsize
int sys_rt_sigpending(uint32_t *set, uint32_t sigsetsize);
#endif
