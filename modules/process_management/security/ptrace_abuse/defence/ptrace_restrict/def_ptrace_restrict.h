#ifndef DEF_PTRACE_RESTRICT_H
#define DEF_PTRACE_RESTRICT_H

#include <stdint.h>

// May 'caller_uid' inspect or control (read registers/memory, single-
// step) a process owned by 'target_uid'? Debugging another process
// exposes all of its memory, so it needs ownership or CAP_SYS_PTRACE-
// like privilege, and a system process is off limits. UzOS has no
// ptrace() yet; this is the gate such a call would have to pass.
int def_ptrace_allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system);

#endif
