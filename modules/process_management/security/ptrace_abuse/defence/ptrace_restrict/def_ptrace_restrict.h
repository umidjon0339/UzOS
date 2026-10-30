#ifndef DEF_PTRACE_RESTRICT_H
#define DEF_PTRACE_RESTRICT_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// The owner rule alone: may 'caller_uid' debug a process of 'target_uid'?
// Your own, or anyone's with CAP_PTRACE; never a system process.
int def_ptrace_allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system);

// May 'caller' attach to 'target' (ptrace ATTACH / TRACEME, strace -p)?
// The owner rule, plus: the target is dumpable, not traced yet, not the
// caller, and (Yama scope 1) the caller is its ANCESTOR - unless it
// holds CAP_PTRACE. 1 = allowed, 0 = refused (logged).
int def_ptrace_attach_ok(const process_t *caller, const process_t *target);

#endif
