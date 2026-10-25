/*
 * def_setuid_guard.c - DEFENCE setuid_guard: nobody makes themselves root.
 *
 * Stops:  setuid_escalation (security/attack/setuid_escalation)
 * Called: sys_setuid() (syscalls/setuid), before the uid is changed.
 * Rules:  1. the wanted uid belongs to a real user
 *         2. CAP_SETUID (root) may become any user
 *         3. anyone else may only "become" itself (nothing changes)
 * Why:    the uid is the ONE number every other check trusts: kill_guard,
 *         priority_bounds, file permissions. If a program could set its
 *         own uid to 0, every one of those checks would wave it through.
 */

#include "def_setuid_guard.h"
#include "registry.h"
#include "audit.h"
#include "users.h"
#include "caps.h"

// The whole rule, as a pure function so the selftest can try it.
static int setuid_allows(uint32_t current, uint32_t wanted, int wanted_exists) {
    if (!wanted_exists) return 0;               // rule 1
    if (caps_has(current, CAP_SETUID)) return 1;    // rule 2
    return wanted == current;                   // rule 3
}

int def_setuid_allowed(uint32_t current, uint32_t wanted) {
    if (setuid_allows(current, wanted, users_find_uid(wanted) != 0))
        return 1;
    audit_log("setuid_guard", "setuid refused: only root may change its uid");
    return 0;
}

static int selftest(void) {
    if (setuid_allows(UID_GUEST, UID_ROOT, 1)) return 0;    // guest -> root
    if (setuid_allows(1000, 1001, 1)) return 0;             // user -> another user
    if (setuid_allows(UID_ROOT, 4242, 0)) return 0;         // nobody has that uid
    if (!setuid_allows(UID_ROOT, UID_GUEST, 1)) return 0;   // root gives up rights
    if (!setuid_allows(UID_GUEST, UID_GUEST, 1)) return 0;  // stay yourself
    return 1;
}

static const defence_t defence = {
    "setuid_guard",
    "only root may change its uid; nobody can make itself root",
    "setuid_escalation",
    selftest
};

void def_setuid_guard_register(void) {
    registry_add_defence(&defence);
}
