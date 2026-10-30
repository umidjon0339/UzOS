#ifndef SYS_RT_SIGRETURN_H
#define SYS_RT_SIGRETURN_H
/* sys_rt_sigreturn.h - rt_sigreturn(): a signal handler is done; carry on where the signal came. */
#include <stdint.h>
#include "arch/x86/isr/isr.h"  // registers_t
#define SYS_RT_SIGRETURN 76     // EAX=76 (the trampoline in every signal frame calls it)
uint32_t sys_rt_sigreturn(registers_t *regs);
#endif
