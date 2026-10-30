#ifndef DEF_CHILD_ONLY_H
#define DEF_CHILD_ONLY_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// May 'caller' wait for the process 'pid', whose parent is 'parent_pid'?
// Only its parent may. wait4 / waitpid / waitid ask this when they are
// given a pid (live, or in the exit log). 1 = allowed, 0 = refused (logged).
int def_wait_allowed(const process_t *caller, uint32_t pid, uint32_t parent_pid);

#endif
