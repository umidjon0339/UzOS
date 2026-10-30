/*
 * def_hidepid.c - DEFENCE hidepid: other users' process details stay private.
 *
 * Stops:  proc_info_leak (security/proc_info_leak)
 * Called: procinfo and pwdx (commands/), /proc/<pid>/status
 *         (filesystem_management core/procfs), before they show the
 *         details of a process.
 * Rule:   the details of a process are visible to its owner (if it is
 *         dumpable, prctl PR_SET_DUMPABLE) and to CAP_PTRACE (root).
 *         That it exists (pid, name, owner) stays visible to all, as
 *         `ps` shows.
 * Why:    a process's working directory, open files and signal masks
 *         tell a stranger what its user is doing: which files they
 *         edit, where their secrets are. Linux has the same rule as the
 *         /proc mount option hidepid=invisible (and ptrace_may_access).
 *
 * Uses: caps_has / CAP_PTRACE (security_management core/caps),
 *       audit_log, registry.
 */

#include "modules/process_management/security/proc_info_leak/defence/hidepid/def_hidepid.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_PTRACE

static int visible(int viewer_has_cap, uint32_t viewer_uid, uint32_t owner, int dumpable) {
    if (viewer_has_cap)
        return 1;                           // root / CAP_PTRACE: everything
    return viewer_uid == owner && dumpable; // your own, unless it said "not even me"
}

int def_hidepid_visible(uint32_t viewer_uid, const process_t *target) {
    if (!target)
        return 0;
    if (visible(caps_has(viewer_uid, CAP_PTRACE), viewer_uid, target->uid, target->dumpable))
        return 1;
    audit_log("hidepid", "details of another user's process hidden");
    return 0;
}

static int selftest(void) {
    if (!visible(0, 1, 1, 1)) return 0;     // my own process
    if (visible(0, 1, 0, 1)) return 0;      // root's process: hidden from user 1
    if (visible(0, 1, 2, 1)) return 0;      // user 2's: hidden
    if (visible(0, 1, 1, 0)) return 0;      // my own, but not dumpable
    if (!visible(1, 0, 2, 0)) return 0;     // CAP_PTRACE sees all
    return 1;
}

static const defence_t defence = {
    "hidepid",
    "process details (cwd, files, signals) only for the owner and root",
    "proc_info_leak",
    selftest
};

void def_hidepid_register(void) { registry_add_defence(&defence); }
