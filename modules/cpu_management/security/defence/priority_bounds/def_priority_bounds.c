/*
 * def_priority_bounds.c - DEFENCE priority_bounds: only sane, permitted priorities.
 *
 * Stops:  priority_out_of_range (security/attack/priority_out_of_range)
 * Called: priority_set() (core/priority), used by the setpriority
 *         syscall and the `nice` command, before anything is changed.
 * Rules:  1. PRIORITY_MIN <= level <= PRIORITY_MAX       (1..7)
 *         2. the target exists and is not a system process
 *         3. CAP_PRIORITY (root) may set any level on any other process
 *         4. a user only on its own processes, and only DOWN
 *            (level <= its current priority), like Unix `nice`
 * Why:    the level is a number from a user program.
 *           1000  ranks above 7, above the aging cap: nobody, not even
 *                 an aged shell, could ever take the CPU back
 *           -5    below the idle task (and 4 billion as unsigned)
 *         And it is a privilege: a user who raises itself to 7 beats
 *         everyone; a user who lowers the shell to 1 slows it for all.
 */

#include "def_priority_bounds.h"
#include "registry.h"
#include "audit.h"
#include "priority.h"
#include "caps.h"

// The whole rule, as a pure function so the selftest can try it.
static int change_allowed(int level, int target_exists, int target_is_system,
                          uint32_t caller_uid, uint32_t target_uid, int current) {
    if (level < PRIORITY_MIN || level > PRIORITY_MAX) return 0;    // rule 1
    if (!target_exists || target_is_system) return 0;               // rule 2
    if (caps_has(caller_uid, CAP_PRIORITY)) return 1;               // rule 3
    return caller_uid == target_uid && level <= current;            // rule 4
}

int def_priority_change_allowed(const process_t *caller, const process_t *target,
                                int level) {
    uint32_t caller_uid = caller ? caller->uid : 0;

    if (change_allowed(level, target != 0, target && target->is_system,
                       caller_uid, target ? target->uid : 0,
                       target ? target->priority : 0))
        return 1;

    if (level < PRIORITY_MIN || level > PRIORITY_MAX)
        audit_log("priority_bounds", "priority refused: not a level from 1 to 7");
    else
        audit_log("priority_bounds", "priority refused: system process, not yours, or a raise");
    return 0;
}

static int selftest(void) {
    if (change_allowed(1000, 1, 0, 0, 0, 4)) return 0;      // too high, even for root
    if (change_allowed(-5, 1, 0, 0, 0, 4)) return 0;        // negative
    if (change_allowed(PRIORITY_IDLE, 1, 0, 0, 0, 4)) return 0;  // nobody becomes "idle"
    if (change_allowed(4, 0, 0, 0, 0, 4)) return 0;         // no such process
    if (change_allowed(1, 1, 1, 0, 0, 4)) return 0;         // system process, even root
    if (change_allowed(7, 1, 0, 1, 1, 4)) return 0;         // user raising itself
    if (change_allowed(2, 1, 0, 1, 2, 4)) return 0;         // user vs someone else's
    if (!change_allowed(2, 1, 0, 1, 1, 4)) return 0;        // user lowering itself: fine
    if (!change_allowed(7, 1, 0, 0, 1, 4)) return 0;        // root raising a user process
    return 1;
}

static const defence_t defence = {
    "priority_bounds",
    "levels 1-7 only; users may only lower their own processes",
    "priority_out_of_range",
    selftest
};

void def_priority_bounds_register(void) {
    registry_add_defence(&defence);
}
