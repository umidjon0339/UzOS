/*
 * def_timer_frequency.c - DEFENCE timer_frequency: only rates the PIT can do.
 *
 * Stops:  timer_divide_by_zero (security/timer_divide_by_zero)
 * Called: init_timer() (timer/), before any division or port write.
 * Rule:   PIT_MIN_HZ <= hz <= PIT_INPUT_HZ
 * Why:    init_timer() computes divisor = 1193180 / hz.
 *           hz = 0      -> division by zero: CPU exception, kernel halts
 *           hz < 19     -> divisor > 65535: cut to 16 bits, wrong rate
 *           hz > 1.19 M -> divisor 0, which the PIT reads as 65536
 *         A wrong tick rate breaks sleep(), uptime and scheduling.
 */

#include "def_timer_frequency.h"
#include "registry.h"
#include "audit.h"
#include "timer.h"

// The whole rule, as a pure function so the selftest can try it.
static int frequency_ok(uint32_t hz) {
    return hz >= PIT_MIN_HZ && hz <= PIT_INPUT_HZ;
}

int def_timer_frequency_ok(uint32_t hz) {
    if (frequency_ok(hz))
        return 1;
    audit_log("timer_frequency", "PIT frequency refused: divisor would not fit");
    return 0;
}

static int selftest(void) {
    if (frequency_ok(0)) return 0;                  // divide by zero
    if (frequency_ok(PIT_MIN_HZ - 1)) return 0;     // divisor too big
    if (!frequency_ok(PIT_MIN_HZ)) return 0;        // lowest legal rate
    if (!frequency_ok(TIMER_HZ)) return 0;          // what UzOS uses
    if (!frequency_ok(PIT_INPUT_HZ)) return 0;      // divisor 1
    if (frequency_ok(PIT_INPUT_HZ + 1)) return 0;   // divisor 0
    return 1;
}

static const defence_t defence = {
    "timer_frequency",
    "only accept PIT rates whose divisor fits in 16 bits (never 0)",
    "timer_divide_by_zero",
    selftest
};

void def_timer_frequency_register(void) {
    registry_add_defence(&defence);
}
