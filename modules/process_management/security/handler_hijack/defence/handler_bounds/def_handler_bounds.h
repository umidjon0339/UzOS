#ifndef DEF_HANDLER_BOUNDS_H
#define DEF_HANDLER_BOUNDS_H

#include "modules/process_management/core/signal/signal.h"  // k_sigaction_t

// May a process set 'act' as its action for 'sig'? SIGKILL and SIGSTOP
// keep their default (no handler, no SIG_IGN); a handler and a restorer
// must be user-space addresses. rt_sigaction asks this before it
// changes anything. 1 = allowed, 0 = refused (logged).
int def_handler_ok(int sig, const k_sigaction_t *act);

#endif
