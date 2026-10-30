/*
 * def_session_check.c - DEFENCE session_check: process groups stay
 * inside their session.
 *
 * Stops:  pgid_hijack (security/pgid_hijack)
 * Called: sys_setpgid() and sys_setsid() (syscalls/), before a process
 *         changes group or session.
 * Rules (POSIX setpgid / setsid):
 *   1. you may move yourself or your own child, nobody else
 *   2. only within your session; never a session leader
 *   3. into a NEW group (pgid = the target's pid), or a group that
 *      already exists in that same session
 *   4. setsid() only if you are not a group leader already (a leader
 *      leaving would split its group across two sessions)
 * Why:    a process group is what gets a terminal's signals (Ctrl+C,
 *         Ctrl+Z, SIGHUP at logout) and what kill(-pgid) hits. Joining
 *         another session's group lets a process receive or dodge
 *         those signals as part of someone else's login; pulling a
 *         stranger into your group lets you kill(-pgid) it.
 *
 * Uses: process_get_table (core/process), audit_log, registry.
 */

#include "modules/process_management/security/pgid_hijack/defence/session_check/def_session_check.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

// The whole setpgid rule, on facts the caller looked up.
static const char *refusal(uint32_t caller_pid, uint32_t caller_sid,
                           uint32_t target_pid, uint32_t target_ppid, uint32_t target_sid,
                           uint32_t pgid, int group_in_session) {
    if (target_pid != caller_pid && target_ppid != caller_pid)
        return "setpgid: not yourself or your child";
    if (target_sid != caller_sid)
        return "setpgid: the process is in another session";
    if (target_sid == target_pid)
        return "setpgid: a session leader cannot change group";
    if (pgid != target_pid && !group_in_session)
        return "setpgid: no such group in this session";
    return 0;
}

static int group_exists(uint32_t pgid, uint32_t sid) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].state != PROCESS_STATE_ZOMBIE &&
            table[i].pgid == pgid && table[i].sid == sid)
            return 1;
    return 0;
}

int def_setpgid_ok(const process_t *caller, const process_t *target, uint32_t pgid) {
    const char *why = !caller || !target ? "setpgid: no such process"
                    : refusal(caller->pid, caller->sid, target->pid, target->ppid,
                              target->sid, pgid, group_exists(pgid, target->sid));
    if (!why)
        return 1;
    audit_log("session_check", why);
    return 0;
}

int def_setsid_ok(const process_t *caller) {
    if (caller && caller->pgid != caller->pid)
        return 1;
    audit_log("session_check", "setsid: a group leader cannot start a new session");
    return 0;
}

static int selftest(void) {
    //            caller  sid  target ppid tsid  pgid group?
    if (refusal(5, 2, 5, 2, 2, 5, 0)) return 0;     // myself, a new group
    if (refusal(5, 2, 7, 5, 2, 5, 1)) return 0;     // my child, into my group
    if (!refusal(5, 2, 7, 2, 2, 7, 0)) return 0;    // someone else's child
    if (!refusal(5, 2, 5, 2, 2, 1, 0)) return 0;    // a group of another session
    if (!refusal(5, 2, 7, 5, 9, 7, 0)) return 0;    // my child, but in another session
    if (!refusal(5, 5, 5, 0, 5, 9, 1)) return 0;    // a session leader
    return 1;
}

static const defence_t defence = {
    "session_check",
    "setpgid/setsid: groups stay in their session (POSIX rules)",
    "pgid_hijack",
    selftest
};

void def_session_check_register(void) { registry_add_defence(&defence); }
