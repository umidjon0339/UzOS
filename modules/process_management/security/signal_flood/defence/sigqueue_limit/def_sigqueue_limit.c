/*
 * def_sigqueue_limit.c - DEFENCE sigqueue_limit: a fair share of queued signals.
 *
 * Stops:  signal_flood (security/signal_flood)
 * Called: sigqueue_add() (core/signal/sigqueue.c), before a queued
 *         signal (rt_sigqueueinfo) takes an entry of the shared pool.
 * Rule:   a user may have at most min(RLIMIT_SIGPENDING, 16) queued
 *         signals waiting at a time; root too.
 * Why:    a queued signal carries a value, so each one needs kernel
 *         memory until it is delivered. The pool has 64 entries for
 *         everyone. A target that blocks the signal never takes them,
 *         so without a cap one user fills the pool and nobody else can
 *         queue a signal any more. Linux has the same limit
 *         (RLIMIT_SIGPENDING, `ulimit -i`).
 *
 * Uses: audit_log (security_management core/audit), registry (core/registry).
 */

#include "modules/process_management/security/signal_flood/defence/sigqueue_limit/def_sigqueue_limit.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                          // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                                // audit_log

static int allowed(int queued, uint32_t rlimit) {
    uint32_t limit = rlimit < SIGQUEUE_USER_MAX ? rlimit : SIGQUEUE_USER_MAX;
    return queued >= 0 && (uint32_t)queued < limit;
}

int def_sigqueue_allowed(uint32_t uid, int queued, uint32_t rlimit) {
    (void)uid;                          // the same rule for every user
    if (allowed(queued, rlimit))
        return 1;
    audit_log("sigqueue_limit", "queued signal refused: this user's share of the pool is used up");
    return 0;
}

static int selftest(void) {
    if (!allowed(0, 16)) return 0;              // the first one: fine
    if (!allowed(15, 16)) return 0;             // the 16th: fine
    if (allowed(16, 16)) return 0;              // the 17th: refused
    if (allowed(3, 3)) return 0;                // a lower rlimit counts
    if (allowed(16, 0xFFFFFFFFu)) return 0;     // "unlimited" is still 16
    if (allowed(-1, 16)) return 0;              // nonsense in: refuse
    return 1;
}

static const defence_t defence = {
    "sigqueue_limit",
    "a user may have at most 16 queued signals waiting (RLIMIT_SIGPENDING)",
    "signal_flood",
    selftest
};

void def_sigqueue_limit_register(void) { registry_add_defence(&defence); }
