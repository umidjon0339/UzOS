/*
 * def_kill_guard.c - DEFENCE kill_guard: you may only kill what you own.
 *
 * Stops:  kill_system (security/kill_system)
 * Called: process_kill() (core/process), before anything is changed.
 * Rules:  1. the target must exist
 *         2. system processes (idle, kernel_main) can never be killed
 *         3. CAP_KILL (root) may kill any other process; a user only its own
 * Why:    the guard sits inside process_kill(), not in the `kill`
 *         command, so every caller passes through it.
 */

#include "modules/process_management/security/kill_system/defence/kill_guard/def_kill_guard.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                       // audit_log
#include "modules/security_management/core/caps/caps.h"                                         // caps_has

// The whole rule, as a pure function so the selftest can try it.
static int kill_allows(int target_exists, int target_is_system,
                       uint32_t caller_uid, uint32_t target_uid) {
    if (!target_exists) return 0;               // rule 1
    if (target_is_system) return 0;             // rule 2
    if (caps_has(caller_uid, CAP_KILL)) return 1;   // rule 3: root
    return caller_uid == target_uid;            // rule 3: own processes only
}

int def_kill_allowed(const process_t *caller, const process_t *target) {
    uint32_t caller_uid = caller ? caller->uid : 0;

    if (kill_allows(target != 0, target && target->is_system,
                    caller_uid, target ? target->uid : 0))
        return 1;

    audit_log("kill_guard", target ? "kill refused: system process or not yours"
                                 : "kill refused: no such process");
    return 0;
}

static int selftest(void) {
    if (kill_allows(0, 0, 0, 0) != 0) return 0;         // no such process
    if (kill_allows(1, 1, 0, 0) != 0) return 0;         // root vs system process
    if (kill_allows(1, 1, 1000, 0) != 0) return 0;      // user vs system process
    if (kill_allows(1, 0, 1000, 1001) != 0) return 0;   // user vs someone else's
    if (kill_allows(1, 0, 1000, 1000) != 1) return 0;   // user vs own
    if (kill_allows(1, 0, 0, 1000) != 1) return 0;      // root vs user process
    return 1;
}

static const defence_t defence = {
    "kill_guard",
    "never kill system processes; users may only kill their own",
    "kill_system",
    selftest
};

void def_kill_guard_register(void) {
    registry_add_defence(&defence);
}
