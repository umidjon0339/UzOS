#ifndef SYS_RT_SIGQUEUEINFO_H
#define SYS_RT_SIGQUEUEINFO_H
/* sys_rt_sigqueueinfo.h - rt_sigqueueinfo(pid, sig, info): send a signal with a value. */
#include <stdint.h>
#include "modules/process_management/core/signal/signal.h"  // k_siginfo_t
#define SYS_RT_SIGQUEUEINFO 74  // EAX=74, EBX=pid, ECX=sig, EDX=info
int sys_rt_sigqueueinfo(uint32_t pid, int sig, const k_siginfo_t *info);
#endif
