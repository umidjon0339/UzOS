/*
 * sys_fork.c - fork(): not possible with one address space (see the .h).
 * Uses: nothing.
 */
#include "modules/process_management/syscalls/fork/sys_fork.h"   // this file

int sys_fork(void) {
    return -1;      // needs per-process address spaces: use vfork() + execve()
}
