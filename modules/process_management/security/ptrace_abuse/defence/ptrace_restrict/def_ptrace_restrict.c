/*
 * def_ptrace_restrict.c - DEFENCE ptrace_restrict: debugging needs rights.
 *
 * Stops:  ptrace_abuse (security/ptrace_abuse)
 * Called: nothing yet - UzOS has no ptrace() syscall. This is the rule a
 *         debugging interface WOULD have to apply before it let one
 *         process read another's registers and memory.
 * Rule:   a system process is off limits; CAP_KILL (stands in for a
 *         debug capability here) may attach to any other; otherwise you
 *         may only debug a process you own.
 * Why:    a debugger sees and can change every byte of the traced
 *         process. Without an owner check, ptrace is a read/write window
 *         into anyone's secrets - the classic way one account steals
 *         another's keys. Linux restricts it (Yama ptrace_scope).
 */

#include "def_ptrace_restrict.h"
#include "registry.h"
#include "audit.h"
#include "caps.h"

static int allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system) {
    if (target_is_system) return 0;
    if (caps_has(caller_uid, CAP_KILL)) return 1;
    return caller_uid == target_uid;
}

int def_ptrace_allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system) {
    if (allowed(caller_uid, target_uid, target_is_system))
        return 1;
    audit_log("ptrace_restrict", "ptrace refused: not your process, or a system one");
    return 0;
}

static int selftest(void) {
    if (!allowed(1, 1, 0)) return 0;
    if (allowed(1, 2, 0)) return 0;
    if (allowed(1, 0, 1)) return 0;
    if (!allowed(0, 2, 0)) return 0;
    if (allowed(0, 3, 1)) return 0;
    return 1;
}

static const defence_t defence = {
    "ptrace_restrict",
    "debugging another process needs ownership; no ptrace into strangers",
    "ptrace_abuse",
    selftest
};

void def_ptrace_restrict_register(void) { registry_add_defence(&defence); }
