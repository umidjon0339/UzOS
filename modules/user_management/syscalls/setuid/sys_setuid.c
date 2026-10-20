/*
 * sys_setuid.c - setuid(): change the caller's uid.
 *
 * Root uses this to give up its rights: a program started by root can
 * switch to a normal user before it does risky work. Nobody may use it
 * to GET more rights; the defence setuid_guard decides.
 *
 * Uses: the current process (process_management), def_setuid_allowed
 *       (security/defence/setuid_guard).
 */

#include "sys_setuid.h"
#include "scheduler.h"
#include "def_setuid_guard.h"

int32_t sys_setuid(uint32_t uid) {
    process_t *self = scheduler_get_current_process();

    // DEFENCE setuid_guard: only root may change its uid.
    if (!self || !def_setuid_allowed(self->uid, uid))
        return -1;

    self->uid = uid;
    return 0;
}
