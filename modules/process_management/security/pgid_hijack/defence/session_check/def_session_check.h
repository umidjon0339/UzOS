#ifndef DEF_SESSION_CHECK_H
#define DEF_SESSION_CHECK_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// setpgid(target, pgid) by 'caller': 'target' is the caller or its
// child, in the caller's session, not a session leader; 'pgid' is the
// target's own pid (a new group) or a group that exists in the SAME
// session. 1 = allowed, 0 = refused (logged).
int def_setpgid_ok(const process_t *caller, const process_t *target, uint32_t pgid);

// setsid() by 'caller': not already a process group leader.
// 1 = allowed, 0 = refused (logged).
int def_setsid_ok(const process_t *caller);

#endif
