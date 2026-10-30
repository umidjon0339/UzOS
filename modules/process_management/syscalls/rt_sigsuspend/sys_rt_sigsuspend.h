#ifndef SYS_RT_SIGSUSPEND_H
#define SYS_RT_SIGSUSPEND_H
/* sys_rt_sigsuspend.h - rt_sigsuspend(mask, sigsetsize): wait for a signal with another mask. */
#include <stdint.h>
#define SYS_RT_SIGSUSPEND 72    // EAX=72, EBX=mask, ECX=sigsetsize
int sys_rt_sigsuspend(const uint32_t *mask, uint32_t sigsetsize);
#endif
