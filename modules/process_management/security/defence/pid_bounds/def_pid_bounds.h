#ifndef DEF_PID_BOUNDS_H
#define DEF_PID_BOUNDS_H

#include <stdint.h>

#define PID_LIMIT 0xFFFFFF00u   // PIDs stop here, well before 32 bits wrap

// Is 'pid' a usable PID? 1 = yes, 0 = refused (logged).
int def_pid_bounds_check(uint32_t pid);

#endif
