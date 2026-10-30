/*
 * def_pidfd_handle.c - DEFENCE pidfd_handle: act on a process by handle,
 * and only as far as you may.
 *
 * Stops:  pid_reuse_race (security/pid_reuse_race)
 * Called: sys_pidfd_open (open), sys_pidfd_getfd (getfd). Every use of a
 *         handle also goes through pidfd_target() (core/pidfd), which
 *         asks pcb_generation whether the handle's process still lives.
 * Rules:  1. a handle is made only for a live process (no zombie, no
 *            system process)
 *         2. a handle never moves: once its process ends, every use
 *            fails (ESRCH) instead of reaching the next process
 *         3. pidfd_getfd copies another process's open file: it needs
 *            ptrace rights (your own dumpable process, or CAP_PTRACE)
 * Why:    `pgrep worker` then `kill 1234` is a race: between the two the
 *         process may end and, where PIDs wrap (Linux, pid_max), 1234
 *         may now be someone else. A handle closes the gap. Copying a
 *         file descriptor is like reading the other process's memory,
 *         so it is guarded the same way (Linux: PTRACE_MODE_ATTACH).
 *
 * Uses: caps_has / CAP_PTRACE (security_management core/caps), audit_log,
 *       registry.
 */

#include "modules/process_management/security/pid_reuse_race/defence/pidfd_handle/def_pidfd_handle.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_PTRACE

static int open_ok(int alive, int is_system) {
    return alive && !is_system;
}

static int getfd_ok(int has_cap, uint32_t caller, uint32_t owner, int dumpable, int is_system) {
    if (is_system)
        return 0;
    return has_cap || (caller == owner && dumpable);
}

int def_pidfd_open_ok(const process_t *target) {
    int alive = target && target->state != PROCESS_STATE_ZOMBIE &&
                target->state != PROCESS_STATE_FREE;
    if (open_ok(alive, target ? target->is_system : 0))
        return 1;
    audit_log("pidfd_handle", "pidfd_open refused: no such live process, or a system one");
    return 0;
}

int def_pidfd_getfd_ok(uint32_t caller_uid, const process_t *target) {
    if (target && getfd_ok(caps_has(caller_uid, CAP_PTRACE), caller_uid, target->uid,
                           target->dumpable, target->is_system))
        return 1;
    audit_log("pidfd_handle", "pidfd_getfd refused: no ptrace rights over that process");
    return 0;
}

static int selftest(void) {
    if (!open_ok(1, 0)) return 0;               // a live process
    if (open_ok(0, 0)) return 0;                // a zombie / gone
    if (open_ok(1, 1)) return 0;                // a system process
    if (!getfd_ok(0, 1, 1, 1, 0)) return 0;     // my own
    if (getfd_ok(0, 1, 0, 1, 0)) return 0;      // root's, as user 1
    if (getfd_ok(0, 1, 1, 0, 0)) return 0;      // my own, not dumpable
    if (!getfd_ok(1, 0, 1, 0, 0)) return 0;     // CAP_PTRACE
    if (getfd_ok(1, 0, 0, 1, 1)) return 0;      // never a system process
    return 1;
}

static const defence_t defence = {
    "pidfd_handle",
    "pidfds name one live process; getfd needs ptrace rights",
    "pid_reuse_race",
    selftest
};

void def_pidfd_handle_register(void) { registry_add_defence(&defence); }
