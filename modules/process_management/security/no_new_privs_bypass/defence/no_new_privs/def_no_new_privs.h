#ifndef DEF_NO_NEW_PRIVS_H
#define DEF_NO_NEW_PRIVS_H

#include "modules/process_management/core/process/process.h"   // process_t

// May 'p' install a seccomp filter? Only if it can never gain rights
// (prctl PR_SET_NO_NEW_PRIVS), or it holds CAP_SETUID (root): a filter
// that makes calls fail could trick a more privileged program into a
// half-done, unsafe state. sys_seccomp asks this.
// 1 = allowed, 0 = refused (logged).
int def_nnp_seccomp_ok(const process_t *p);

#endif
