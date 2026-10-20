#ifndef DEF_PRIORITY_BOUNDS_H
#define DEF_PRIORITY_BOUNDS_H

#include "process.h"

// May 'caller' set 'target' to 'level'? caller NULL = the kernel (root).
// 1 = yes, 0 = refused (logged).
int def_priority_change_allowed(const process_t *caller, const process_t *target,
                                int level);

#endif
