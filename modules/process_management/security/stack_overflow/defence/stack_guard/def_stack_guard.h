#ifndef DEF_STACK_GUARD_H
#define DEF_STACK_GUARD_H

#include "process.h"

// Is 'esp' still inside p's own stack? 1 = yes, 0 = overflowed (logged).
int def_stack_guard_check(const process_t *p, uint32_t esp);

#endif
