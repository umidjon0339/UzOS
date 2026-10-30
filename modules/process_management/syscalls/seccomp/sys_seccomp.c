/*
 * sys_seccomp.c - seccomp(op, flags, args): put the caller in a sandbox.
 *
 *   SECCOMP_SET_MODE_STRICT   from now on only read, write, exit and
 *                             rt_sigreturn; any other syscall ends it
 *   SECCOMP_SET_MODE_FILTER   from now on only the syscalls in the list;
 *                             others get its action (SIGSYS or -1)
 *   SECCOMP_GET_ACTION_AVAIL  is this action supported? (0 = yes)
 *
 * There is no way out: a new filter is ANDed with the old one, strict
 * stays strict, and every child inherits it (process_create). A filter
 * needs no_new_privs first, or root. The syscall handler enforces it
 * before every call (seccomp_filter). Returns 0 or -1.
 *
 * Uses: scheduler_get_current_process (core/scheduler),
 *       def_nnp_seccomp_ok (security/no_new_privs_bypass/defence/no_new_privs),
 *       SECCOMP_* (security/syscall_abuse/defence/seccomp_filter).
 */
#include "modules/process_management/syscalls/seccomp/sys_seccomp.h"     // this file
#include "modules/process_management/core/scheduler/scheduler.h"         // scheduler_get_current_process
#include "modules/process_management/security/syscall_abuse/defence/seccomp_filter/def_seccomp_filter.h"     // SECCOMP_MODE_*, SECCOMP_RET_*
#include "modules/process_management/security/no_new_privs_bypass/defence/no_new_privs/def_no_new_privs.h"  // def_nnp_seccomp_ok

static int action_known(uint32_t a) {
    return a == SECCOMP_RET_KILL_THREAD || a == SECCOMP_RET_KILL_PROCESS ||
           (a & 0xFFFF0000u) == SECCOMP_RET_ERRNO || a == SECCOMP_RET_ALLOW;
}

// Of two actions, the stricter one: killing beats failing beats allowing.
static uint32_t stricter(uint32_t a, uint32_t b) {
    int ka = a == SECCOMP_RET_KILL_THREAD || a == SECCOMP_RET_KILL_PROCESS;
    int kb = b == SECCOMP_RET_KILL_THREAD || b == SECCOMP_RET_KILL_PROCESS;
    if (ka || kb)
        return SECCOMP_RET_KILL_PROCESS;
    return a == SECCOMP_RET_ALLOW ? b : a;
}

int sys_seccomp(uint32_t op, uint32_t flags, void *args) {
    process_t *self = scheduler_get_current_process();
    if (!self || flags != 0)
        return -1;

    switch (op) {
        case SECCOMP_SET_MODE_STRICT:
            if (args)
                return -1;
            self->seccomp_mode = SECCOMP_MODE_STRICT;   // the strictest: stays
            return 0;

        case SECCOMP_SET_MODE_FILTER: {
            const seccomp_list_t *list = args;
            if (!list || !action_known(list->action))
                return -1;
            if (self->seccomp_mode == SECCOMP_MODE_STRICT)
                return 0;                               // already stricter than any list
            // DEFENCE no_new_privs: a filter only with the promise (or root).
            if (!def_nnp_seccomp_ok(self))
                return -1;
            if (self->seccomp_mode != SECCOMP_MODE_FILTER) {    // the first filter
                for (int i = 0; i < 4; i++)
                    self->seccomp_allow[i] = 0xFFFFFFFFu;
                self->seccomp_action = SECCOMP_RET_ALLOW;
            }
            for (int i = 0; i < 4; i++)                 // only ever stricter
                self->seccomp_allow[i] &= list->allow[i];
            self->seccomp_action = stricter(self->seccomp_action, list->action);
            self->seccomp_mode = SECCOMP_MODE_FILTER;
            return 0;
        }

        case SECCOMP_GET_ACTION_AVAIL:
            return args && action_known(*(const uint32_t *)args) ? 0 : -1;

        default:
            return -1;
    }
}
