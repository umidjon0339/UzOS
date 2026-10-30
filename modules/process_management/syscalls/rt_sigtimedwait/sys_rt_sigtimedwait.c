/*
 * sys_rt_sigtimedwait.c - rt_sigtimedwait(set, info, timeout, sigsetsize).
 *
 * Wait until one of the signals in 'set' is pending, then TAKE it: no
 * handler runs, the signal number is returned and 'info' says who sent
 * it. The signals in 'set' are normally blocked first, so they wait for
 * this call instead of being delivered. timeout 0 = wait for ever.
 *
 *   returns  the signal number
 *            -1 when the time is up (EAGAIN), or another signal that is
 *            not blocked arrived (EINTR: it is delivered on the way out)
 *
 * Uses: scheduler_get_current_process (core/scheduler), sigqueue_take,
 *       signal_deliverable (core/signal), sys_sleep (syscalls/sleep),
 *       timer_ticks (device_management), cpu_irq_save, def_sleep_time_ticks
 *       (security/sleep_overflow/defence/sleep_bounds).
 */
#include "modules/process_management/syscalls/rt_sigtimedwait/sys_rt_sigtimedwait.h"                   // this file
#include "modules/process_management/core/scheduler/scheduler.h"                                       // scheduler_get_current_process
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                                       // sys_sleep
#include "modules/device_management/timer/timer.h"                                                     // timer_ticks
#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // def_sleep_time_ticks
#include "arch/x86/cpu/cpu.h"                                                                          // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                                                         // memset

#define POLL_MS 10

static int lowest(uint32_t mask) {
    for (int sig = 1; sig < SIGNAL_COUNT; sig++)
        if (mask & SIG_BIT(sig))
            return sig;
    return 0;
}

// Take one pending signal of 'want' off 'p'. 0 = none pending.
static int take(process_t *p, uint32_t want, k_siginfo_t *info) {
    uint32_t flags = cpu_irq_save();
    int sig = lowest(p->signals & want);
    if (sig) {
        k_siginfo_t got;
        if (!sigqueue_take(p, sig, &got)) {         // a plain signal: no value
            memset(&got, 0, sizeof(got));
            got.signo = sig;
            got.code  = p->sig_code[sig];
            got.pid   = p->sig_from[sig];
        }
        if (!sigqueue_waiting(p, sig))              // more queued: stays pending
            p->signals &= ~SIG_BIT(sig);
        if (info)
            *info = got;
    }
    cpu_irq_restore(flags);
    return sig;
}

int sys_rt_sigtimedwait(const uint32_t *set, k_siginfo_t *info,
                        const sigwait_timeout_t *timeout, uint32_t sigsetsize) {
    process_t *self = scheduler_get_current_process();
    if (!self || !set || (sigsetsize != 4 && sigsetsize != 8))
        return -1;

    uint32_t want = set[0] & ~SIG_UNBLOCKABLE;
    uint32_t end = 0;
    if (timeout) {
        uint32_t ticks;
        // DEFENCE sleep_bounds: a valid time, converted without overflow.
        if (!def_sleep_time_ticks((int32_t)timeout->sec, (int32_t)timeout->nsec, 1000000000u, &ticks))
            return -1;                              // EINVAL
        end = timer_ticks + ticks;
    }

    for (;;) {
        int sig = take(self, want, info);
        if (sig)
            return sig;
        if (signal_deliverable(self) & ~want)
            return -1;                              // EINTR: another signal came
        if (timeout && (int32_t)(timer_ticks - end) >= 0)
            return -1;                              // EAGAIN: time is up
        sys_sleep(POLL_MS);
    }
}
