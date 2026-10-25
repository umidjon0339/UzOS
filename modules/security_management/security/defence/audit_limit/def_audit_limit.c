/*
 * def_audit_limit.c - DEFENCE audit_limit: a flood cannot wash the log.
 *
 * Stops:  log_flood (security/attack/log_flood)
 * Called: audit_log() (core/audit), for every event, before it is put
 *         into the ring.
 * Rules:  1. each defence may put AUDIT_BURST events into the ring per
 *            second (AUDIT_WINDOW_TICKS)
 *         2. after that, its events are still COUNTED, only not kept
 *         3. a new second starts a new allowance
 * Why:    the ring keeps the newest 32 events. Anyone who can make the
 *         kernel refuse something cheap, like close(-1), can push the
 *         one event that mattered out of it. With the limit, a flood
 *         fills at most 10 slots a second, and the counters (`audit
 *         sum`) still show every refusal. Linux does the same in its
 *         log (printk_ratelimit) and audit system (rate_limit).
 *
 * This defence cannot call audit_log() when it refuses: it is part of
 * the log. audit.c writes one note in its name for each flood instead.
 */

#include "def_audit_limit.h"
#include "registry.h"

// The whole rule, as a pure function so the selftest can try it.
// (now - window_start also works when the tick counter wraps.)
static int limit_allows(uint32_t now, uint32_t window_start, uint32_t stored) {
    if (now - window_start >= AUDIT_WINDOW_TICKS) return 1;    // rule 3
    return stored < AUDIT_BURST;                                // rules 1, 2
}

int def_audit_limit_allows(uint32_t now, uint32_t window_start, uint32_t stored) {
    return limit_allows(now, window_start, stored);
}

static int selftest(void) {
    if (!limit_allows(50, 0, 0)) return 0;                          // a quiet defence
    if (!limit_allows(50, 0, AUDIT_BURST - 1)) return 0;            // the last one kept
    if (limit_allows(50, 0, AUDIT_BURST)) return 0;                 // the flood begins
    if (limit_allows(50, 0, 1000)) return 0;                        // ...and goes on
    if (!limit_allows(AUDIT_WINDOW_TICKS, 0, 1000)) return 0;       // a new second
    if (limit_allows(5, 0xFFFFFFF0u, AUDIT_BURST)) return 0;        // tick wrapped: same second
    return 1;
}

static const defence_t defence = {
    "audit_limit",
    "at most 10 events per defence per second in the ring; the rest are counted",
    "log_flood",
    selftest
};

void def_audit_limit_register(void) {
    registry_add_defence(&defence);
}
