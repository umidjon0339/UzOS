/*
 * sys_rt_sigaction.c - rt_sigaction(sig, act, oldact, sigsetsize).
 *
 * Read and/or change what signal 'sig' does for the caller:
 *   act->handler   SIG_DFL (the default), SIG_IGN, or a function
 *   act->flags     SA_SIGINFO (3 arguments), SA_ONSTACK, SA_RESTORER,
 *                  SA_NODEFER, SA_RESETHAND
 *   act->mask      signals also blocked while the handler runs
 * Setting SIG_IGN (or a default of "ignore") throws away a pending one,
 * as Linux does. Returns 0, or -1 (bad signal, refused).
 *
 * Uses: scheduler_get_current_process (core/scheduler), signal_*
 *       (core/signal), def_handler_ok (security/handler_hijack/defence/
 *       handler_bounds).
 */
#include "modules/process_management/syscalls/rt_sigaction/sys_rt_sigaction.h"                             // this file
#include "modules/process_management/core/scheduler/scheduler.h"                                           // scheduler_get_current_process
#include "modules/process_management/security/handler_hijack/defence/handler_bounds/def_handler_bounds.h"  // def_handler_ok

int sys_rt_sigaction(int sig, const k_sigaction_t *act, k_sigaction_t *oldact, uint32_t sigsetsize) {
    process_t *self = scheduler_get_current_process();
    if (!self || sig < 1 || sig >= SIGNAL_COUNT || (sigsetsize != 4 && sigsetsize != 8))
        return -1;

    k_sigaction_t old = { self->sig_handler[sig], self->sig_flags[sig],
                          self->sig_restorer[sig], self->sig_mask[sig] };

    if (act) {
        // DEFENCE handler_bounds: no catching SIGKILL/SIGSTOP, no kernel addresses.
        if (!def_handler_ok(sig, act))
            return -1;
        self->sig_handler[sig]  = act->handler;
        self->sig_flags[sig]    = act->flags;
        self->sig_restorer[sig] = act->restorer;
        self->sig_mask[sig]     = act->mask & ~SIG_UNBLOCKABLE;

        int ignoring = act->handler == SIG_IGN ||
                       (act->handler == SIG_DFL && signal_default_action(sig) == ACT_IGNORE);
        if (ignoring)
            signal_clear(self, sig);
    }
    if (oldact)
        *oldact = old;
    return 0;
}
