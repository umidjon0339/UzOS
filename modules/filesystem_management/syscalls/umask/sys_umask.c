/*
 * sys_umask.c - umask(mask): set the bits new files leave out; returns
 * the old mask.
 *
 * Linux bits (022 = group and others lose write). UzOS keeps two digits
 * (owner, others) in process_t.umask: the owner's r/w bits and the
 * others' r/w bits. Children inherit it. Whatever it says, a new file
 * is never writable by others (secure_umask, in ramdisk_create).
 *
 * Uses: scheduler_get_current_process (process_management).
 */
#include "modules/filesystem_management/syscalls/umask/sys_umask.h"     // this file
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process

uint32_t sys_umask(uint32_t mask) {
    process_t *self = scheduler_get_current_process();
    if (!self)
        return 0;
    uint32_t old = self->umask;
    self->umask = ((mask >> 6) & 6) << 3 | (mask & 6);
    uint32_t others = old & 7;                      // back to Linux bits:
    return ((old >> 3) & 7) << 6 | others << 3 | others;   // group = others
}
