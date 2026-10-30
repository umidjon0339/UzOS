/*
 * sys_setuid.c - setuid(): change the caller's uid.
 *
 * Root uses this to give up its rights: a program started by root can
 * switch to a normal user before it does risky work. Nobody may use it
 * to GET more rights; the defence setuid_guard decides.
 *
 * Uses: the current process (process_management), def_setuid_allowed
 *       (security/defence/setuid_guard), def_dumpable_on_setuid
 *       (process_management defence dumpable_flag).
 */

#include "sys_setuid.h"
#include "scheduler.h"
#include "def_setuid_guard.h"
#include "modules/process_management/security/core_dump_leak/defence/dumpable_flag/def_dumpable_flag.h"  // def_dumpable_on_setuid

int32_t sys_setuid(uint32_t uid) {
    process_t *self = scheduler_get_current_process();

    // DEFENCE setuid_guard: only root may change its uid.
    if (!self || !def_setuid_allowed(self->uid, uid))
        return -1;

    // DEFENCE dumpable_flag (process_management): what was read as the
    // old user must not be readable by the new one.
    def_dumpable_on_setuid(self, self->uid, uid);
    self->uid = uid;
    return 0;
}
