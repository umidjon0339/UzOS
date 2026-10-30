#ifndef SYS_RT_TGSIGQUEUEINFO_H
#define SYS_RT_TGSIGQUEUEINFO_H
/* sys_rt_tgsigqueueinfo.h - rt_tgsigqueueinfo(tgid, tid, sig, info): a valued signal for one thread. */
#include <stdint.h>
#include "modules/process_management/core/signal/signal.h"  // k_siginfo_t
#define SYS_RT_TGSIGQUEUEINFO 75    // EAX=75, EBX=tgid, ECX=tid, EDX=sig, ESI=info
int sys_rt_tgsigqueueinfo(uint32_t tgid, uint32_t tid, int sig, const k_siginfo_t *info);
#endif
