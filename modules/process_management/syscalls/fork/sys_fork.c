/*
 * sys_fork.c - fork() STUB: needs per-process address spaces (see .h).
 * Uses: nothing (returns -1 until UzOS has separate address spaces).
 */
#include "sys_fork.h"

int sys_fork(void) {
    return -1;      // unsupported: see sys_fork.h
}
