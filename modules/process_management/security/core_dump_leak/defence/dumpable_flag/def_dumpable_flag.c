/*
 * def_dumpable_flag.c - DEFENCE dumpable_flag: a process that changed its
 * user cannot be looked into by the new user.
 *
 * Stops:  core_dump_leak (security/core_dump_leak)
 * Called: sys_setuid() (user_management/syscalls/setuid), when the uid
 *         really changes.
 * Rule:   after a uid change the process is NOT dumpable (prctl can
 *         turn it back on, if the program knows its memory is clean).
 *         Every way into a process's memory respects the flag: ptrace
 *         (ptrace_restrict), process_vm_readv / writev (vm_access_check),
 *         pidfd_getfd (pidfd_handle), procinfo / pmap / /proc (hidepid).
 * Why:    a server starts as root, reads its private key, then drops to a
 *         normal user with setuid(). Its memory still holds the key, and
 *         it now belongs to that user - who could read it with a debugger,
 *         or (on Linux) in a core dump the crash leaves in a file they
 *         own. Linux clears "dumpable" on every credential change for
 *         exactly this. UzOS writes no core files; the flag guards all
 *         the other ways in.
 *
 * Uses: audit_log, registry (security_management).
 */

#include "modules/process_management/security/core_dump_leak/defence/dumpable_flag/def_dumpable_flag.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static int dumpable_after(int dumpable, uint32_t old_uid, uint32_t new_uid) {
    return old_uid == new_uid ? dumpable : 0;
}

int def_dumpable_on_setuid(process_t *p, uint32_t old_uid, uint32_t new_uid) {
    int now = dumpable_after(p->dumpable, old_uid, new_uid);
    if (p->dumpable && !now)
        audit_log("dumpable_flag", "uid changed: process made non-dumpable");
    p->dumpable = now;
    return now;
}

static int selftest(void) {
    if (dumpable_after(1, 0, 1) != 0) return 0;     // root -> user: no longer
    if (dumpable_after(1, 1, 1) != 1) return 0;     // "change" to itself: stays
    if (dumpable_after(0, 0, 0) != 0) return 0;     // stays off
    return 1;
}

static const defence_t defence = {
    "dumpable_flag",
    "setuid() makes a process non-dumpable: its old secrets stay hidden",
    "core_dump_leak",
    selftest
};

void def_dumpable_flag_register(void) { registry_add_defence(&defence); }
