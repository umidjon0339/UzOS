/*
 * def_ptrace_restrict.c - DEFENCE ptrace_restrict: debugging needs rights.
 *
 * Stops:  ptrace_abuse (security/ptrace_abuse)
 * Called: sys_ptrace (PTRACE_ATTACH, PTRACE_TRACEME) and `strace -p`,
 *         before one process may read and change another's registers
 *         and memory.
 * Rules:  1. never a system process, never yourself
 *         2. your own process (or any, with CAP_PTRACE)
 *         3. it must be dumpable (prctl PR_SET_DUMPABLE) - unless CAP_PTRACE
 *         4. it is not traced already
 *         5. Yama ptrace_scope 1: only an ANCESTOR may attach, so a
 *            program cannot read your ssh-agent or browser just because
 *            the same user runs both - unless CAP_PTRACE
 * Why:    a debugger sees and can change every byte of the traced
 *         process. Without these rules, ptrace is a read/write window
 *         into anyone's secrets: the classic way to steal keys.
 *
 * Uses: process_get_by_pid (core/process), caps_has / CAP_PTRACE
 *       (security_management), audit_log, registry.
 */

#include "modules/process_management/security/ptrace_abuse/defence/ptrace_restrict/def_ptrace_restrict.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                            // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                                  // audit_log
#include "modules/security_management/core/caps/caps.h"                                                    // caps_has, CAP_PTRACE

static int owner_ok(int has_cap, uint32_t caller_uid, uint32_t target_uid, int target_is_system) {
    if (target_is_system) return 0;
    return has_cap || caller_uid == target_uid;
}

int def_ptrace_allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system) {
    if (owner_ok(caps_has(caller_uid, CAP_PTRACE), caller_uid, target_uid, target_is_system))
        return 1;
    audit_log("ptrace_restrict", "ptrace refused: not your process, or a system one");
    return 0;
}

// Is 'a' an ancestor of 't' (following ppid)?
static int is_ancestor(const process_t *a, const process_t *t) {
    for (int depth = 0; t && depth < MAX_PROCESSES; depth++) {
        if (t->ppid == a->pid)
            return 1;
        t = t->ppid ? process_get_by_pid(t->ppid) : 0;
    }
    return 0;
}

static const char *attach_refusal(int has_cap, const process_t *c, const process_t *t, int ancestor) {
    if (!t || t == c)
        return "ptrace refused: no such process, or yourself";
    if (!owner_ok(has_cap, c->uid, t->uid, t->is_system))
        return "ptrace refused: not your process, or a system one";
    if (t->tracer)
        return "ptrace refused: it is traced already";
    if (!has_cap && !t->dumpable)
        return "ptrace refused: the process is not dumpable";
    if (!has_cap && !ancestor)
        return "ptrace refused: only an ancestor may attach (Yama scope 1)";
    return 0;
}

int def_ptrace_attach_ok(const process_t *caller, const process_t *target) {
    const char *why = !caller ? "ptrace refused: no caller"
                    : attach_refusal(caps_has(caller->uid, CAP_PTRACE), caller, target,
                                     target ? is_ancestor(caller, target) : 0);
    if (!why)
        return 1;
    audit_log("ptrace_restrict", why);
    return 0;
}

static int selftest(void) {
    process_t me = {0}, kid = {0}, other = {0}, sys = {0};
    me.pid = 5;  me.uid = 1;
    kid.pid = 6; kid.uid = 1; kid.ppid = 5; kid.dumpable = 1;
    other.pid = 7; other.uid = 1; other.ppid = 2; other.dumpable = 1;
    sys.pid = 1; sys.is_system = 1; sys.dumpable = 1;

    if (attach_refusal(0, &me, &kid, 1)) return 0;          // my child: fine
    if (!attach_refusal(0, &me, &other, 0)) return 0;       // same user, not mine: Yama
    if (attach_refusal(1, &me, &other, 0)) return 0;        // ...unless CAP_PTRACE
    if (!attach_refusal(1, &me, &sys, 0)) return 0;         // never a system process
    if (!attach_refusal(0, &me, &me, 0)) return 0;          // not myself
    kid.dumpable = 0;
    if (!attach_refusal(0, &me, &kid, 1)) return 0;         // not dumpable
    kid.dumpable = 1; kid.tracer = 9;
    if (!attach_refusal(0, &me, &kid, 1)) return 0;         // traced already
    if (owner_ok(0, 1, 2, 0)) return 0;                     // another user's
    return 1;
}

static const defence_t defence = {
    "ptrace_restrict",
    "ptrace: own, dumpable, untraced, a descendant (Yama); never system",
    "ptrace_abuse",
    selftest
};

void def_ptrace_restrict_register(void) { registry_add_defence(&defence); }
