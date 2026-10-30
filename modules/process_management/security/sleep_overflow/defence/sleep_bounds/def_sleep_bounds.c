/*
 * def_sleep_bounds.c - DEFENCE sleep_bounds: times from a program are
 * checked and converted without overflow.
 *
 * Stops:  sleep_overflow (security/sleep_overflow)
 * Called: sys_sleep / sys_sleep_interruptible (ms), nanosleep,
 *         clock_nanosleep, setitimer, rt_sigtimedwait (timespec /
 *         timeval) - every place a program's time becomes ticks.
 * Rules:  1. a negative time, or nsec >= 1e9 (usec >= 1e6), is refused
 *            (Linux: EINVAL)
 *         2. the conversion is done in 64 bits and rounded UP: 1 ns
 *            still waits a tick, never 0
 *         3. the result is capped at SLEEP_MAX_TICKS (2^31 - 1, ~248
 *            days): "deadline - now" always fits a signed 32-bit number
 * Why:    sec * 100 in 32 bits wraps: 42949673 s becomes 0.3 s, and
 *         "(ms + 9) / 10" with ms = 0xFFFFFFFF becomes 0. A program
 *         that asked to sleep for months returns at once, a timeout
 *         that should protect something expires immediately, and a
 *         wrapped deadline can look already passed - or never come.
 *
 * Uses: TIMER_HZ (device_management), audit_log, registry.
 */

#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // this file
#include "modules/device_management/timer/timer.h"                // TIMER_HZ
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static uint32_t cap(uint64_t ticks) {
    return ticks > SLEEP_MAX_TICKS ? SLEEP_MAX_TICKS : (uint32_t)ticks;
}

uint32_t def_sleep_ms_ticks(uint32_t ms) {
    uint64_t ticks = ((uint64_t)ms * TIMER_HZ + 999) / 1000;     // rounded up
    return cap(ticks);
}

static int convert(int32_t sec, int32_t frac, uint32_t frac_per_sec, uint32_t *ticks) {
    if (sec < 0 || frac < 0 || (uint32_t)frac >= frac_per_sec)
        return 0;
    uint64_t t = (uint64_t)sec * TIMER_HZ +
                 ((uint64_t)frac * TIMER_HZ + frac_per_sec - 1) / frac_per_sec;
    *ticks = cap(t);
    return 1;
}

int def_sleep_time_ticks(int32_t sec, int32_t frac, uint32_t frac_per_sec, uint32_t *ticks) {
    if (convert(sec, frac, frac_per_sec, ticks))
        return 1;
    audit_log("sleep_bounds", "time refused: negative, or a fraction of a second >= 1 s");
    return 0;
}

static int selftest(void) {
    uint32_t t;
    if (def_sleep_ms_ticks(1) != 1) return 0;                   // 1 ms: one tick, not 0
    if (def_sleep_ms_ticks(0xFFFFFFFFu) < 0x10000000u) return 0; // ~49 days, not 0
    if (!convert(42949673, 0, 1000000000, &t) || t != SLEEP_MAX_TICKS) return 0;  // capped, not 0.3 s
    if (!convert(0, 1, 1000000000, &t) || t != 1) return 0;     // 1 ns: one tick
    if (!convert(1, 500000, 1000000, &t) || t != TIMER_HZ + TIMER_HZ / 2) return 0;
    if (convert(0, 1000000000, 1000000000, &t)) return 0;       // nsec = 1 s: refused
    if (convert(-1, 0, 1000000000, &t)) return 0;               // negative: refused
    if (convert(0, -5, 1000000, &t)) return 0;
    return 1;
}

static const defence_t defence = {
    "sleep_bounds",
    "times become ticks in 64 bits, rounded up, capped; bad ones refused",
    "sleep_overflow",
    selftest
};

void def_sleep_bounds_register(void) { registry_add_defence(&defence); }
