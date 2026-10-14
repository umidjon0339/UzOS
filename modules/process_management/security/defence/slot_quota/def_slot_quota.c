#include "proc_security.h"
#include "process.h"
#include "kheap.h"

void def_release_child_slot(process_t *child) {
    if (!child) return;
    if (child->stack_base) {
        kfree((void *)child->stack_base);
        child->stack_base = 0;
    }
    child->state = PROCESS_STATE_FREE;
}

/* ===================================================================
 * DEFENCE: slot quota
 * ===================================================================
 * Restores the INV_RESOURCE guarantee broken by the fork bomb.
 *
 * Two rules, both checked inside process_create() before a slot is
 * handed out:
 *
 *   1. per-user cap   - an unprivileged user may not hold more than
 *                       MAX_PROC_PER_UID processes at once.
 *   2. root reserve   - an unprivileged user may never take the last
 *                       RESERVED_FOR_ROOT slots.
 *
 * Rule 2 exists because a machine that is 100% full is a machine that
 * nobody can log into to fix. Keeping a little room for root turns a
 * permanent denial of service into a recoverable one.
 *
 * The policy is a pure function of three numbers, so it can be tested
 * without touching the real process table.
 * =================================================================== */

#define RESERVED_FOR_ROOT 5
#define MAX_PROC_PER_UID  8

static int count_free_slots(void) {
    process_t *t = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state == PROCESS_STATE_FREE) n++;
    return n;
}

static int count_owned_by(uint32_t uid) {
    process_t *t = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].state != PROCESS_STATE_FREE && t[i].uid == uid) n++;
    return n;
}

/* the whole policy, in one place */
static int quota_allows(int free_slots, int owned_by_uid, uint32_t uid) {
    if (uid == 0) return 1;                          /* root is not capped */
    if (owned_by_uid >= MAX_PROC_PER_UID) return 0;  /* rule 1             */
    return free_slots > RESERVED_FOR_ROOT;           /* rule 2             */
}

int def_slot_quota_check(uint32_t uid) {
    int free_slots = count_free_slots();
    int owned      = count_owned_by(uid);

    if (quota_allows(free_slots, owned, uid)) return 1;

    sec_log("slot_quota", "process creation refused: quota or root reserve");
    return 0;
}

/* selftest() feeds the policy crafted numbers and checks the verdict.
 * The boundary cases are the interesting ones. */
static int def_slot_quota_selftest(void) {
    /* unprivileged user under the cap with slots to spare: allowed */
    if (quota_allows(64, 0, 1000) != 1) return 0;

    /* right at the per-user cap: refused */
    if (quota_allows(64, MAX_PROC_PER_UID, 1000) != 0) return 0;

    /* one below the cap: still allowed */
    if (quota_allows(64, MAX_PROC_PER_UID - 1, 1000) != 1) return 0;

    /* table nearly full: the root reserve must hold */
    if (quota_allows(RESERVED_FOR_ROOT, 0, 1000) != 0) return 0;
    if (quota_allows(RESERVED_FOR_ROOT + 1, 0, 1000) != 1) return 0;

    /* root is never blocked, even on a completely full table */
    if (quota_allows(0, 999, 0) != 1) return 0;

    return 1;
}

static const defence_t def = {
    "slot_quota",
    "cap processes per user and keep a few slots reserved for root",
    "fork_bomb",
    def_slot_quota_selftest
};

void def_slot_quota_register(void) {
    proc_security_register_defence(&def);
}
