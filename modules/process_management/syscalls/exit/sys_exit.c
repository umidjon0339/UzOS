/*
 * sys_exit.c - exit(): end the calling process.
 *
 * Uses: process_exit() (core/process). The process becomes a zombie and
 *       never runs again; the reaper frees its slot and stack later.
 */

#include "sys_exit.h"
#include "process.h"

void sys_exit(void) {
    process_exit();
}
