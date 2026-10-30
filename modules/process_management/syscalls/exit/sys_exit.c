/*
 * sys_exit.c - exit(code): end the calling process.
 *
 * The exit code (low 8 bits) is kept as exit_status = code << 8, the
 * way wait() reports it (WEXITSTATUS).
 *
 * Uses: process_exit_with() (core/process). The process becomes a zombie
 *       and never runs again; the reaper frees its slot and stack later.
 */

#include "modules/process_management/syscalls/exit/sys_exit.h"  // this file
#include "modules/process_management/core/process/process.h"    // process_exit_with

void sys_exit(int code) {
    process_exit_with((code & 0xFF) << 8);
}
