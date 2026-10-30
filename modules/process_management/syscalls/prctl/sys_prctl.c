/*
 * sys_prctl.c - prctl(option, arg2): read or change one setting of the
 * calling process.
 *
 *   PR_SET_NAME / PR_GET_NAME    the name `ps` shows (name_guard checks it)
 *   PR_SET_PDEATHSIG / GET       a signal for us when our parent ends
 *   PR_SET_CHILD_SUBREAPER / GET orphans below us are re-parented to us
 *   PR_SET_DUMPABLE / GET        may others inspect us (procinfo, ptrace)
 *   PR_SET_NO_NEW_PRIVS / GET    exec can never raise our rights; cannot
 *                                be turned off again
 *
 * Returns 0 (or the value asked for), -1 for a bad option or argument.
 * The handler checks pointer arguments from ring 3 (user_ptr_bounds).
 *
 * Uses: scheduler_get_current_process (core/scheduler), strncpy
 *       (lib/string), def_name_ok (security/process_masquerade/defence/
 *       name_guard).
 */
#include "modules/process_management/syscalls/prctl/sys_prctl.h"     // this file
#include "modules/process_management/core/scheduler/scheduler.h"     // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"           // SIGNAL_COUNT
#include "modules/process_management/security/process_masquerade/defence/name_guard/def_name_guard.h"  // def_name_ok
#include "lib/string/string.h"                                       // strncpy

int sys_prctl(int option, uint32_t arg2) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return -1;

    switch (option) {
        case PR_SET_NAME: {
            char name[PR_NAME_LEN];
            strncpy(name, (const char *)arg2, PR_NAME_LEN - 1);
            name[PR_NAME_LEN - 1] = '\0';               // Linux cuts it at 15
            // DEFENCE name_guard: printable, and not a system process's name.
            if (!def_name_ok(self, name))
                return -1;
            strncpy(self->name, name, PROCESS_NAME_LEN - 1);
            self->name[PROCESS_NAME_LEN - 1] = '\0';
            return 0;
        }
        case PR_GET_NAME:
            strncpy((char *)arg2, self->name, PR_NAME_LEN - 1);
            ((char *)arg2)[PR_NAME_LEN - 1] = '\0';
            return 0;
        case PR_SET_PDEATHSIG:
            if (arg2 >= SIGNAL_COUNT)
                return -1;
            self->pdeathsig = (int)arg2;
            return 0;
        case PR_GET_PDEATHSIG:
            *(int *)arg2 = self->pdeathsig;
            return 0;
        case PR_SET_DUMPABLE:
            if (arg2 > 1)
                return -1;
            self->dumpable = (int)arg2;
            return 0;
        case PR_GET_DUMPABLE:
            return self->dumpable;
        case PR_SET_CHILD_SUBREAPER:
            self->child_subreaper = arg2 != 0;
            return 0;
        case PR_GET_CHILD_SUBREAPER:
            *(int *)arg2 = self->child_subreaper;
            return 0;
        case PR_SET_NO_NEW_PRIVS:
            if (arg2 != 1)
                return -1;                              // it can only be turned on
            self->no_new_privs = 1;
            return 0;
        case PR_GET_NO_NEW_PRIVS:
            return self->no_new_privs;
        default:
            return -1;                                  // EINVAL
    }
}
