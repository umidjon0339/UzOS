#ifndef DEF_FAULT_ISOLATION_H
#define DEF_FAULT_ISOLATION_H

#include "isr.h"

// Can the OS survive this exception by killing only the current
// process? 1 = yes (logged), 0 = no: the caller must halt.
int def_fault_isolation_survivable(const registers_t *regs);

#endif
