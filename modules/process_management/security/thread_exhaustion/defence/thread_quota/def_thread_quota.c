/*
 * def_thread_quota.c - DEFENCE thread_quota: one program cannot be every process.
 *
 * Stops:  thread_exhaustion (security/thread_exhaustion)
 * Called: sys_clone() and sys_clone3() (cpu_management/syscalls), for
 *         CLONE_THREAD, before the new thread takes a process slot.
 * Rule:   a thread group (one program) has at most 4 live threads,
 *         whoever runs it - root too.
 * Why:    every thread is a process slot (64 in all) and a kernel stack.
 *         slot_quota caps a USER at 8 processes, but root is exempt -
 *         and one buggy or malicious program run as root would spin
 *         threads until the table is full. A cap per program limits
 *         the damage of one program, whoever runs it. (Linux:
 *         threads-max, RLIMIT_NPROC, and cgroup pids.max.)
 *
 * Uses: process_get_table (core/process), audit_log, registry.
 */

#include "modules/process_management/security/thread_exhaustion/defence/thread_quota/def_thread_quota.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static int allowed(int threads) {
    return threads < THREADS_PER_GROUP;
}

int def_thread_quota_ok(const process_t *parent) {
    uint32_t group = parent->tgid ? parent->tgid : parent->pid;
    process_t *table = process_get_table();
    int threads = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].state != PROCESS_STATE_ZOMBIE &&
            (table[i].tgid ? table[i].tgid : table[i].pid) == group)
            threads++;
    if (allowed(threads))
        return 1;
    audit_log("thread_quota", "clone refused: this program already has 4 threads");
    return 0;
}

static int selftest(void) {
    if (!allowed(1)) return 0;          // the main thread wants a second
    if (!allowed(3)) return 0;          // the 4th
    if (allowed(4)) return 0;           // the 5th: refused
    return 1;
}

static const defence_t defence = {
    "thread_quota",
    "a program (thread group) has at most 4 threads, root too",
    "thread_exhaustion",
    selftest
};

void def_thread_quota_register(void) { registry_add_defence(&defence); }
