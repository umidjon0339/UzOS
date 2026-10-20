/*
 * signal.c - Pending signals, one bit each.
 *
 * signal_send() is called from the keyboard interrupt while the target
 * may be halfway into going to sleep. Interrupts stay off while the bit
 * and the state change together, so the wake-up can never be missed
 * (sys_sleep_interruptible does the same on its side).
 *
 * Uses: the current process (core/scheduler), cpu_irq_save (arch/x86/cpu).
 */

#include "signal.h"
#include "scheduler.h"
#include "cpu.h"

#define MAX_SIGNAL 31           // one bit each in a 32-bit word

void signal_send(process_t *p, int sig) {
    if (!p || sig < 1 || sig > MAX_SIGNAL)
        return;

    uint32_t flags = cpu_irq_save();
    p->signals |= 1u << sig;
    if (p->state == PROCESS_STATE_SLEEPING && p->sleep_interruptible) {
        p->sleep_ticks = 0;
        p->state = PROCESS_STATE_READY;     // wake it: it may run next tick
    }
    cpu_irq_restore(flags);
}

int signal_pending(const process_t *p, int sig) {
    if (!p || sig < 1 || sig > MAX_SIGNAL)
        return 0;
    return (p->signals >> sig) & 1;
}

void signal_clear(process_t *p, int sig) {
    if (!p || sig < 1 || sig > MAX_SIGNAL)
        return;
    uint32_t flags = cpu_irq_save();
    p->signals &= ~(1u << sig);
    cpu_irq_restore(flags);
}

int signal_interrupted(void) {
    return signal_pending(scheduler_get_current_process(), SIGINT);
}
