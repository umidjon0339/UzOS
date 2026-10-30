#ifndef DEF_CLOSE_ON_EXEC_H
#define DEF_CLOSE_ON_EXEC_H

#include "modules/process_management/core/process/process.h"   // process_t

// execve: close every descriptor of 'p' opened with O_CLOEXEC, and every
// pidfd (a handle on a process must not reach a program that never asked
// for it). Returns how many were closed.
int def_close_on_exec(process_t *p);

#endif
