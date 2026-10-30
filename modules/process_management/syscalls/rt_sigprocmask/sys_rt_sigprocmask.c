/*
 * sys_rt_sigprocmask.c - rt_sigprocmask(how, set, oldset, sigsetsize).
 *
 * A blocked signal stays pending until it is unblocked; then it is
 * delivered on the next return to ring 3. SIGKILL and SIGSTOP can never
 * be blocked (they are taken out of 'set' silently, as in Linux).
 * A sigset is 4 or 8 bytes (Linux uses 8); UzOS uses the low 32 bits.
 *
 * Uses: scheduler_get_current_process (core/scheduler), SIG_UNBLOCKABLE
 *       (core/signal), cpu_irq_save (arch/x86/cpu).
 */
#include "modules/process_management/syscalls/rt_sigprocmask/sys_rt_sigprocmask.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"                    // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                          // SIG_UNBLOCKABLE
#include "arch/x86/cpu/cpu.h"                                                       // cpu_irq_save, cpu_irq_restore

int sys_rt_sigprocmask(int how, const uint32_t *set, uint32_t *oldset, uint32_t sigsetsize) {
    process_t *self = scheduler_get_current_process();
    if (!self || (sigsetsize != 4 && sigsetsize != 8))
        return -1;
    if (set && how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK)
        return -1;

    uint32_t flags = cpu_irq_save();
    uint32_t old = self->sig_blocked;
    if (set) {
        uint32_t s = set[0] & ~SIG_UNBLOCKABLE;
        if (how == SIG_BLOCK)        self->sig_blocked |= s;
        else if (how == SIG_UNBLOCK) self->sig_blocked &= ~s;
        else                         self->sig_blocked = s;
    }
    cpu_irq_restore(flags);

    if (oldset) {
        oldset[0] = old;
        if (sigsetsize == 8)
            oldset[1] = 0;
    }
    return 0;
}
