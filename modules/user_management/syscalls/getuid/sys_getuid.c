/*
 * sys_getuid.c - getuid(): the uid of the calling process.
 *
 * Uses: process_caller_uid (process_management).
 */

#include "sys_getuid.h"
#include "process.h"

uint32_t sys_getuid(void) {
    return process_caller_uid();
}
