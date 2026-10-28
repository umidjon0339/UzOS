/*
 * def_aging.c - DEFENCE aging: whoever waits long enough gets a turn.
 *
 * Stops:  cpu_hog (security/attack/cpu_hog)
 * Called: priority_effective() (core/priority), for every candidate on
 *         every scheduling decision; priority_pick_next() reports rescues.
 * Rule:   effective = base + waited_ticks / AGING_TICKS, at most
 *         PRIORITY_MAX. The idle task (level 0) is never raised.
 *         waited_ticks goes back to 0 when the process gets the CPU.
 * Why:    with strict priorities, one busy loop at level 7 keeps every
 *         lower process off the CPU forever, the shell included: the
 *         OS looks frozen. With aging, a process at level 1 climbs one
 *         level every 100 ms; after 600 ms it ties the hog and the
 *         round robin gives it a turn. Nobody waits forever.
 */

#include "modules/cpu_management/security/starvation/defence/aging/def_aging.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/security_management/core/audit/audit.h"
#include "modules/cpu_management/core/priority/priority.h"
#include "modules/device_management/timer/timer.h"

#define NOTE_EVERY_TICKS 100        // log a rescue at most once a second

// The whole rule, as a pure function so the selftest can try it.
static int aged(int base, uint32_t waited) {
    if (base <= PRIORITY_IDLE)
        return base;                            // idle stays last
    uint32_t level = (uint32_t)base + waited / AGING_TICKS;
    return level > PRIORITY_MAX ? PRIORITY_MAX : (int)level;
}

int def_aging_effective(int base, uint32_t waited) {
    return aged(base, waited);
}

void def_aging_note_rescue(void) {
    static uint32_t last;           // tick of the last note (0 = never)

    if (last && timer_ticks - last < NOTE_EVERY_TICKS)
        return;
    last = timer_ticks ? timer_ticks : 1;
    audit_log("aging", "a starving process got the CPU");
}

static int selftest(void) {
    if (aged(PRIORITY_MIN, 0) != PRIORITY_MIN) return 0;            // no wait, no change
    if (aged(PRIORITY_MIN, AGING_TICKS) != PRIORITY_MIN + 1) return 0;
    if (aged(PRIORITY_MIN, 6 * AGING_TICKS) != PRIORITY_MAX) return 0;   // ties the hog
    if (aged(PRIORITY_MIN, 0xFFFFFFFF) != PRIORITY_MAX) return 0;   // never above the top
    if (aged(PRIORITY_MAX, 1000) != PRIORITY_MAX) return 0;
    if (aged(PRIORITY_IDLE, 100000) != PRIORITY_IDLE) return 0;     // idle stays idle
    return 1;
}

static const defence_t defence = {
    "aging",
    "a READY process rises one level per 100 ms of waiting: no starvation",
    "starvation",
    selftest
};

void def_aging_register(void) {
    registry_add_defence(&defence);
}
