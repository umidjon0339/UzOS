/*
 * sys_getcaps.c - getcaps(): the capabilities of the calling process.
 *
 * Uses: caps_of (core/caps), process_caller_uid (process_management).
 */

#include "sys_getcaps.h"
#include "caps.h"
#include "process.h"

uint32_t sys_getcaps(void) {
    return caps_of(process_caller_uid());
}
