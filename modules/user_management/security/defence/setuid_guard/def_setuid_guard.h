#ifndef DEF_SETUID_GUARD_H
#define DEF_SETUID_GUARD_H

#include <stdint.h>

// May a process running as 'current' become 'wanted'?
// 1 = yes, 0 = refused (logged).
int def_setuid_allowed(uint32_t current, uint32_t wanted);

#endif
