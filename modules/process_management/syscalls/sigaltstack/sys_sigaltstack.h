#ifndef SYS_SIGALTSTACK_H
#define SYS_SIGALTSTACK_H
/* sys_sigaltstack.h - sigaltstack(ss, old_ss): a second stack for signal handlers. */
#include <stdint.h>
#include "modules/process_management/core/signal/signal.h"  // k_stack_t
#define SYS_SIGALTSTACK 77      // EAX=77, EBX=ss (or 0), ECX=old_ss (or 0)
int sys_sigaltstack(const k_stack_t *ss, k_stack_t *old_ss, uint32_t user_esp);
#endif
