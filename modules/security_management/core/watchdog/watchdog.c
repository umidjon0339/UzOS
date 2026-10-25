/*
 * watchdog.c - A deadline for live attacks, checked on every timer tick.
 *
 * Uses: timer_ticks (device_management/timer), audit_log (core/audit).
 */

#include "watchdog.h"
#include "audit.h"
#include "timer.h"

static uint32_t    deadline;
static const char *attack;
static int         active;
static int         fired;

void watchdog_arm(uint32_t ticks, const char *attack_id) {
    deadline = timer_ticks + ticks;
    attack   = attack_id;
    active   = 1;
    fired    = 0;
}

void watchdog_disarm(void) { active = 0; }

void watchdog_tick(void) {
    if (!active || timer_ticks < deadline)
        return;

    active = 0;
    fired  = 1;
    audit_log("watchdog", attack ? attack : "live attack");
}

int watchdog_fired(void) { return fired; }
const char *watchdog_attack(void) { return attack; }
