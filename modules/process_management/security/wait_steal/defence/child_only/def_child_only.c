/*
 * def_child_only.c - DEFENCE child_only: only a parent collects its child.
 *
 * Stops:  wait_steal (security/wait_steal)
 * Called: sys_wait4, sys_waitpid, sys_waitid, when the caller names a
 *         pid; exitlog_take() (core/exitlog) also hands a record only to
 *         the parent it names.
 * Rule:   a process may wait for, and collect the exit status of, its
 *         own children only - not even root may take another parent's.
 * Why:    an exit status tells what a program did (which test failed,
 *         whether a login was right) and collecting it is a one-time
 *         event: whoever takes it, the real parent never sees it. A
 *         stranger who could wait for your child would steal both.
 *
 * Uses: audit_log, registry (security_management).
 */

#include "modules/process_management/security/wait_steal/defence/child_only/def_child_only.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static int allowed(uint32_t caller_pid, uint32_t parent_pid) {
    return caller_pid != 0 && caller_pid == parent_pid;
}

int def_wait_allowed(const process_t *caller, uint32_t pid, uint32_t parent_pid) {
    (void)pid;
    if (caller && allowed(caller->pid, parent_pid))
        return 1;
    audit_log("child_only", "wait refused: not the caller's child");
    return 0;
}

static int selftest(void) {
    if (!allowed(5, 5)) return 0;       // my child
    if (allowed(5, 2)) return 0;        // the shell's child
    if (allowed(2, 5)) return 0;        // ...and the other way round
    if (allowed(0, 0)) return 0;        // nobody
    return 1;
}

static const defence_t defence = {
    "child_only",
    "only a process's parent may wait for it and take its exit status",
    "wait_steal",
    selftest
};

void def_child_only_register(void) { registry_add_defence(&defence); }
