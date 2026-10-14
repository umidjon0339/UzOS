#include "proc_security.h"

/* ===================================================================
 * DEFENCE: kill guard
 * ===================================================================
 * Restores INV_PRIVILEGE and INV_IDENTITY.
 *
 * Any process can call process_kill() with any number. Without a check,
 * "kill 5" from an unprivileged process kills whatever happens to be
 * PID 5. Two rules stop that:
 *
 *   1. some PIDs are protected      - PID 1 is the running system, and
 *                                     PID 0 means "no process".
 *   2. you may only kill your own   - unless your uid is 0 (root).
 *
 * The important detail is WHERE this lives. The original code had the
 * PID-1 check inside the shell command, so any other caller - a syscall,
 * another kernel module - walked straight past it. A guard belongs next
 * to the operation it guards, not next to the user interface.
 * =================================================================== */

#define SYSTEM_PID 1

/* pure policy, so the rules can be read and tested on their own */
static int kill_allows(uint32_t caller_pid, uint32_t caller_uid,
                       uint32_t target_pid, uint32_t target_uid) {

    if (target_pid == 0) return 0;                 /* no such process     */
    if (target_pid == SYSTEM_PID) return 0;        /* protected           */
    if (caller_pid == target_pid) return 1;        /* own life, deferred  */
    if (caller_uid != 0 && caller_uid != target_uid) return 0;  /* not yours */

    return 1;
}

int def_kill_allowed(uint32_t caller, uint32_t target) {
    process_t *victim = process_get_by_pid(target);
    if (!victim) {
        sec_log("kill_guard", "kill refused: no such process");
        return 0;
    }

    process_t *actor = process_get_by_pid(caller);
    uint32_t caller_uid = actor ? actor->uid : 0;

    if (!kill_allows(caller, caller_uid, target, victim->uid)) {
        sec_log("kill_guard", "kill refused: protected PID or not yours");
        return 0;
    }
    return 1;
}

int def_self_kill_policy(uint32_t caller, uint32_t target) {
    if (caller == target) return 0; // Self-kill must be deferred to zombie state
    return 1;
}

static int def_kill_guard_selftest(void) {
    /* an unprivileged user must not kill the system process */
    if (kill_allows(7, 1000, SYSTEM_PID, 0) != 0) return 0;

    /* nor a process that belongs to somebody else */
    if (kill_allows(7, 1000, 8, 1001) != 0) return 0;

    /* nor a process that does not exist */
    if (kill_allows(7, 1000, 0, 0) != 0) return 0;

    /* but may kill its own */
    if (kill_allows(7, 1000, 7, 1000) != 1) return 0;
    if (kill_allows(7, 1000, 9, 1000) != 1) return 0;

    /* root may kill any user process, but still not the system */
    if (kill_allows(7, 0, 9, 1000) != 1) return 0;
    if (kill_allows(7, 0, SYSTEM_PID, 0) != 0) return 0;

    return 1;
}

static const defence_t def = {
    "kill_guard",
    "protect the system PID and enforce kill ownership",
    "kill_init_bypass",
    def_kill_guard_selftest
};

void def_kill_guard_register(void) {
    proc_security_register_defence(&def);
}
