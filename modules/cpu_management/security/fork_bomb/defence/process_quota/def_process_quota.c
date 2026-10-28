/*
 * def_process_quota.c - DEFENCE process_quota: a user gets only so many
 *                       processes (RLIMIT_NPROC / ulimit -u).
 *
 * Stops:  fork_bomb (security/fork_bomb)
 * Called: process_create (process_management), through
 *         rlimit_nproc_ok() (core/rlimit), on every new process.
 * Rule:   a non-root user may own at most RLIMIT_NPROC processes at
 *         once; the (count+1)-th fork is refused. Root (CAP_RESOURCE)
 *         keeps a reserve so an admin can always log in and clean up.
 * Why:    `while (1) fork();` doubles processes until the table is full
 *         and the machine cannot start anything, root included. A
 *         per-user cap means a bomb fills only that user's quota.
 */

#include "modules/cpu_management/security/fork_bomb/defence/process_quota/def_process_quota.h"
#include "modules/security_management/core/registry/registry.h"

int def_nproc_quota_ok(int is_root, uint32_t count, uint32_t limit) {
    if (is_root)
        return 1;                       // root is never fenced out
    return count < limit;               // room for one more?
}

static int selftest(void) {
    if (!def_nproc_quota_ok(0, 0, 4)) return 0;     // first process: fine
    if (!def_nproc_quota_ok(0, 3, 4)) return 0;     // 3 of 4 used: one more ok
    if (def_nproc_quota_ok(0, 4, 4)) return 0;      // at the cap: refused
    if (def_nproc_quota_ok(0, 9, 4)) return 0;      // already over: refused
    if (!def_nproc_quota_ok(1, 999, 4)) return 0;   // root: always allowed
    return 1;
}

static const defence_t defence = {
    "process_quota",
    "RLIMIT_NPROC (ulimit -u): a user's N-th extra fork is refused",
    "cpu_fork_bomb",
    selftest
};

void def_process_quota_register(void) { registry_add_defence(&defence); }
