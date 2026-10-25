/*
 * def_slot_quota.c - DEFENCE slot_quota: one user cannot fill the table.
 *
 * Stops:  fork_bomb (security/attack/fork_bomb)
 * Called: process_create() (core/process), before a slot is handed out.
 * Rules:  1. a normal user may own at most MAX_PER_USER processes
 *         2. the last RESERVED_FOR_ROOT free slots are for root only
 *         Both rules skip whoever holds CAP_RESOURCE (root).
 * Why:    with both rules, a runaway user hits its own cap first, and
 *         root can always still start a process to clean up.
 */

#include "def_slot_quota.h"
#include "registry.h"
#include "audit.h"
#include "process.h"
#include "caps.h"

#define MAX_PER_USER      8
#define RESERVED_FOR_ROOT 5

static int count_owned_by(uint32_t uid) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].uid == uid)
            n++;
    return n;
}

// The whole rule, as a pure function so the selftest can try it.
static int quota_allows(int free_slots, int owned, uint32_t uid) {
    if (caps_has(uid, CAP_RESOURCE)) return 1;  // root is never capped
    if (owned >= MAX_PER_USER) return 0;        // rule 1
    return free_slots > RESERVED_FOR_ROOT;      // rule 2
}

int def_slot_quota_check(uint32_t uid) {
    if (quota_allows(process_count_free_slots(), count_owned_by(uid), uid))
        return 1;

    audit_log("slot_quota", "process creation refused: user quota or root reserve");
    return 0;
}

static int selftest(void) {
    if (quota_allows(64, 0, 1000) != 1) return 0;                       // normal case
    if (quota_allows(64, MAX_PER_USER - 1, 1000) != 1) return 0;        // just under cap
    if (quota_allows(64, MAX_PER_USER, 1000) != 0) return 0;            // at the cap
    if (quota_allows(RESERVED_FOR_ROOT + 1, 0, 1000) != 1) return 0;    // reserve not yet hit
    if (quota_allows(RESERVED_FOR_ROOT, 0, 1000) != 0) return 0;        // reserve holds
    if (quota_allows(0, 999, 0) != 1) return 0;                         // root always allowed
    return 1;
}

static const defence_t defence = {
    "slot_quota",
    "cap processes per user and keep the last slots for root",
    "fork_bomb",
    selftest
};

void def_slot_quota_register(void) {
    registry_add_defence(&defence);
}
