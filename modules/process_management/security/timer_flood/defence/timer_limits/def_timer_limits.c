/*
 * def_timer_limits.c - DEFENCE timer_limits: a fair number of periodic timers.
 *
 * Stops:  timer_flood (security/timer_flood)
 * Called: sys_setitimer() (syscalls/setitimer), before it arms a timer
 *         with an interval.
 * Rule:   a user may have at most 8 periodic interval timers armed in
 *         all their processes together (CAP_RESOURCE: no limit). The
 *         interval itself is never below one tick (sleep_bounds rounds
 *         up), so one timer fires at most 100 times a second.
 * Why:    every firing is work for the KERNEL: count down, send a
 *         signal, wake a process, deliver it. A user who arms timers in
 *         all their processes at the fastest rate makes the timer tick
 *         spend its time on them: everyone else's programs slow down.
 *         Linux bounds the same thing with RLIMIT_SIGPENDING and the
 *         POSIX timer limits.
 *
 * Uses: process_get_table (core/process), caps_has / CAP_RESOURCE
 *       (security_management), audit_log, registry.
 */

#include "modules/process_management/security/timer_flood/defence/timer_limits/def_timer_limits.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_RESOURCE

static int allowed(int armed, int unlimited) {
    return unlimited || armed < TIMER_USER_MAX;
}

// Periodic timers armed by 'uid', not counting 'skip_which' of 'skip'
// (the one setitimer is about to replace).
static int armed_by(uint32_t uid, const process_t *skip, int skip_which) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        const process_t *q = &table[i];
        if (q->state == PROCESS_STATE_FREE || q->state == PROCESS_STATE_ZOMBIE || q->uid != uid)
            continue;
        for (int t = 0; t < ITIMERS; t++)
            if (q->itimer_value[t] && q->itimer_interval[t] && !(q == skip && t == skip_which))
                n++;
    }
    return n;
}

int def_itimer_allowed(const process_t *p, int which) {
    if (allowed(armed_by(p->uid, p, which), caps_has(p->uid, CAP_RESOURCE)))
        return 1;
    audit_log("timer_limits", "periodic timer refused: this user already has 8 armed");
    return 0;
}

static int selftest(void) {
    if (!allowed(0, 0)) return 0;           // the first
    if (!allowed(7, 0)) return 0;           // the 8th
    if (allowed(8, 0)) return 0;            // the 9th: refused
    if (!allowed(50, 1)) return 0;          // CAP_RESOURCE
    return 1;
}

static const defence_t defence = {
    "timer_limits",
    "a user may have at most 8 periodic interval timers armed",
    "timer_flood",
    selftest
};

void def_timer_limits_register(void) { registry_add_defence(&defence); }
