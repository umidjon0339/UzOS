#ifndef SYS_RT_SIGPROCMASK_H
#define SYS_RT_SIGPROCMASK_H
/* sys_rt_sigprocmask.h - rt_sigprocmask(how, set, oldset, sigsetsize): block / unblock signals. */
#include <stdint.h>
#define SYS_RT_SIGPROCMASK 70   // EAX=70, EBX=how, ECX=set (or 0), EDX=oldset (or 0), ESI=sigsetsize
#define SIG_BLOCK   0           // add 'set' to the blocked signals
#define SIG_UNBLOCK 1           // take 'set' out of them
#define SIG_SETMASK 2           // the blocked signals become 'set'
int sys_rt_sigprocmask(int how, const uint32_t *set, uint32_t *oldset, uint32_t sigsetsize);
#endif
