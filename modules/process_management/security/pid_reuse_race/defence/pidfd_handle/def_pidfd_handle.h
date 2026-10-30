#ifndef DEF_PIDFD_HANDLE_H
#define DEF_PIDFD_HANDLE_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// pidfd_open: a handle is made only for a process that is alive (not a
// zombie, not a system process). 1 = allowed, 0 = refused (logged).
int def_pidfd_open_ok(const process_t *target);

// pidfd_getfd: may 'caller_uid' copy a file descriptor out of 'target'?
// Only with ptrace rights: its own dumpable process, or CAP_PTRACE; never
// a system process. 1 = allowed, 0 = refused (logged).
int def_pidfd_getfd_ok(uint32_t caller_uid, const process_t *target);

#endif
