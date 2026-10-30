#ifndef DEF_KILL_GUARD_H
#define DEF_KILL_GUARD_H

#include "modules/process_management/core/process/process.h"  // process_t

// May 'caller' kill 'target'? 1 = yes, 0 = refused (logged).
// caller NULL = the kernel itself (treated as root).
int def_kill_allowed(const process_t *caller, const process_t *target);

#endif
