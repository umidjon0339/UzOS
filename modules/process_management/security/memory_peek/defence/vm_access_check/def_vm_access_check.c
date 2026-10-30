/*
 * def_vm_access_check.c - DEFENCE vm_access_check: another process's
 * memory, only with rights, and only its user part.
 *
 * Stops:  memory_peek (security/memory_peek)
 * Called: sys_process_vm_readv / sys_process_vm_writev, and sys_ptrace
 *         PEEKDATA / POKEDATA, before a single byte is copied.
 * Rules:  1. never a system process
 *         2. ptrace rights: the caller itself, its own user's DUMPABLE
 *            process, or CAP_PTRACE
 *         3. the range is mapped user memory (user_ptr_bounds): the
 *            kernel's code, heap and stacks are never "remote memory"
 * Why:    process_vm_readv is a fast debugger read: one call, any amount
 *         of another process's memory. Without rule 2 any user reads
 *         any other's passwords and keys; without rule 3 a "remote"
 *         address inside the kernel would read the kernel itself.
 *         Linux checks PTRACE_MODE_ATTACH_REALCREDS the same way.
 *
 * Uses: caps_has / CAP_PTRACE (security_management), def_user_range_ok
 *       (memory_management defence user_ptr_bounds), audit_log, registry.
 */

#include "modules/process_management/security/memory_peek/defence/vm_access_check/def_vm_access_check.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_PTRACE
#include "modules/memory_management/security/defence/user_ptr_bounds/def_user_ptr_bounds.h"  // def_user_range_ok

static const char *refusal(int has_cap, uint32_t caller_pid, uint32_t caller_uid,
                           uint32_t target_pid, uint32_t target_uid, int dumpable, int is_system) {
    if (is_system)
        return "remote memory refused: a system process";
    if (caller_pid != target_pid && !has_cap && (caller_uid != target_uid || !dumpable))
        return "remote memory refused: no ptrace rights over that process";
    return 0;
}

int def_vm_access_ok(const process_t *caller, const process_t *target, uint32_t addr, uint32_t len) {
    const char *why = !caller || !target ? "remote memory refused: no such process"
                    : refusal(caps_has(caller->uid, CAP_PTRACE), caller->pid, caller->uid,
                              target->pid, target->uid, target->dumpable, target->is_system);
    if (!why && !def_user_range_ok(addr, len))         // logs its own refusal
        return 0;
    if (!why)
        return 1;
    audit_log("vm_access_check", why);
    return 0;
}

static int selftest(void) {
    if (refusal(0, 5, 1, 5, 1, 0, 0)) return 0;     // myself, even not dumpable
    if (refusal(0, 5, 1, 6, 1, 1, 0)) return 0;     // my own dumpable process
    if (!refusal(0, 5, 1, 6, 1, 0, 0)) return 0;    // ...not dumpable: refused
    if (!refusal(0, 5, 1, 2, 0, 1, 0)) return 0;    // root's: refused
    if (refusal(1, 2, 0, 6, 1, 0, 0)) return 0;     // CAP_PTRACE
    if (!refusal(1, 2, 0, 1, 0, 1, 1)) return 0;    // a system process: never
    return 1;
}

static const defence_t defence = {
    "vm_access_check",
    "remote memory: ptrace rights, and the target's user pages only",
    "memory_peek",
    selftest
};

void def_vm_access_check_register(void) { registry_add_defence(&defence); }
