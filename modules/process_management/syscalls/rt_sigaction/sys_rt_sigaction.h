#ifndef SYS_RT_SIGACTION_H
#define SYS_RT_SIGACTION_H
/* sys_rt_sigaction.h - rt_sigaction(sig, act, oldact, sigsetsize): what a signal does. */
#include <stdint.h>
#include "modules/process_management/core/signal/signal.h"  // k_sigaction_t
#define SYS_RT_SIGACTION 69     // EAX=69, EBX=sig, ECX=act (or 0), EDX=oldact (or 0), ESI=sigsetsize (4 or 8)
int sys_rt_sigaction(int sig, const k_sigaction_t *act, k_sigaction_t *oldact, uint32_t sigsetsize);
#endif
