#ifndef SYS_RT_SIGTIMEDWAIT_H
#define SYS_RT_SIGTIMEDWAIT_H
/* sys_rt_sigtimedwait.h - rt_sigtimedwait(set, info, timeout, sigsetsize): take a signal without a handler. */
#include <stdint.h>
#include "modules/process_management/core/signal/signal.h"  // k_siginfo_t
#define SYS_RT_SIGTIMEDWAIT 73  // EAX=73, EBX=set, ECX=info (or 0), EDX=timeout (or 0), ESI=sigsetsize

// struct timespec: how long to wait at most.
typedef struct {
    uint32_t sec;
    uint32_t nsec;
} sigwait_timeout_t;

int sys_rt_sigtimedwait(const uint32_t *set, k_siginfo_t *info,
                        const sigwait_timeout_t *timeout, uint32_t sigsetsize);
#endif
