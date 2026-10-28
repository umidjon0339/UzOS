#ifndef DEF_PRIORITY_BOUNDS_H
#define DEF_PRIORITY_BOUNDS_H

#include "modules/process_management/core/process/process.h"   // process_t

// May 'caller' give 'target' the nice value 'nice'? caller NULL = the
// kernel (root). 1 = yes, 0 = refused (logged).
int def_nice_change_allowed(const process_t *caller, const process_t *target, int nice);

// May 'caller' put 'target' under 'policy' with real-time priority
// 'rt_priority'? 1 = yes, 0 = refused (logged).
int def_policy_change_allowed(const process_t *caller, const process_t *target,
                              int policy, int rt_priority);

#endif
