/*
 * sigqueue.c - Signals that carry a value (rt_sigqueueinfo).
 *
 * A plain signal is one bit: sending SIGUSR1 twice before it is delivered
 * leaves one SIGUSR1. rt_sigqueueinfo() also sends a VALUE, and every
 * value must arrive, so each one is kept here until it is delivered:
 *
 *   rt_sigqueueinfo(pid, SIGUSR1, {value 7})  -> entry {pid, USR1, 7}
 *   rt_sigqueueinfo(pid, SIGUSR1, {value 8})  -> entry {pid, USR1, 8}
 *   delivery: handler(USR1, info.value = 7), then handler(USR1, 8)
 *
 * The pool is shared by every process, so one user could fill it and
 * leave nothing for the others: sigqueue_limit caps what one user may
 * have queued at a time.
 *
 * Uses: cpu_irq_save (arch/x86/cpu), def_sigqueue_allowed
 *       (security/signal_flood/defence/sigqueue_limit), RLIMIT_SIGPENDING
 *       (cpu_management/core/rlimit).
 */

#include "modules/process_management/core/signal/signal.h"                                               // this part
#include "modules/process_management/core/scheduler/scheduler.h"                                         // scheduler_get_current_process
#include "modules/process_management/security/signal_flood/defence/sigqueue_limit/def_sigqueue_limit.h"  // def_sigqueue_allowed
#include "modules/cpu_management/core/rlimit/rlimit.h"                                                   // RLIMIT_SIGPENDING
#include "arch/x86/cpu/cpu.h"                                                                            // cpu_irq_save, cpu_irq_restore

static struct {
    int         used;
    uint32_t    target;         // pid it is for
    uint32_t    order;          // smaller = queued earlier
    k_siginfo_t info;
} pool[SIGQUEUE_SIZE];
static uint32_t next_order;

int sigqueue_count_user(uint32_t uid) {
    int n = 0;
    for (int i = 0; i < SIGQUEUE_SIZE; i++)
        n += pool[i].used && pool[i].info.uid == uid;
    return n;
}

int sigqueue_add(process_t *target, const k_siginfo_t *info) {
    process_t *sender = scheduler_get_current_process();
    uint32_t limit = sender ? sender->rlim_cur[RLIMIT_SIGPENDING] : RLIM_INFINITY;
    uint32_t flags = cpu_irq_save();

    // DEFENCE sigqueue_limit: one user may not take the whole pool.
    if (!def_sigqueue_allowed(info->uid, sigqueue_count_user(info->uid), limit)) {
        cpu_irq_restore(flags);
        return 0;
    }
    for (int i = 0; i < SIGQUEUE_SIZE; i++) {
        if (pool[i].used)
            continue;
        pool[i].used   = 1;
        pool[i].target = target->pid;
        pool[i].order  = next_order++;
        pool[i].info   = *info;
        cpu_irq_restore(flags);
        return 1;
    }
    cpu_irq_restore(flags);
    return 0;                   // the pool is full
}

int sigqueue_waiting(const process_t *p, int sig) {
    for (int i = 0; i < SIGQUEUE_SIZE; i++)
        if (pool[i].used && pool[i].target == p->pid && pool[i].info.signo == sig)
            return 1;
    return 0;
}

int sigqueue_take(process_t *p, int sig, k_siginfo_t *out) {
    uint32_t flags = cpu_irq_save();
    int oldest = -1;
    for (int i = 0; i < SIGQUEUE_SIZE; i++) {
        if (!pool[i].used || pool[i].target != p->pid || pool[i].info.signo != sig)
            continue;
        if (oldest < 0 || pool[i].order < pool[oldest].order)
            oldest = i;
    }
    if (oldest >= 0) {
        if (out)
            *out = pool[oldest].info;
        pool[oldest].used = 0;
    }
    cpu_irq_restore(flags);
    return oldest >= 0;
}

void sigqueue_release(process_t *p) {
    for (int i = 0; i < SIGQUEUE_SIZE; i++)
        if (pool[i].used && pool[i].target == p->pid)
            pool[i].used = 0;
}
