/*
 * sys_kcmp.c - kcmp(pid1, pid2, type, idx1, idx2).
 *
 * Do two processes share the same kernel object? Checkpoint/restore
 * tools ask this to rebuild "these two shared a file". The answer is an
 * ORDER, not the address (that would leak kernel pointers):
 *   0 the same     1 / 2 different (first < / > second)     -1 error
 * In UzOS every process has one address space (KCMP_VM: always 0) and
 * its own descriptor table, directory, handlers (the same object only
 * for the same process); KCMP_FILE compares two open files.
 * Both processes' details must be visible to the caller (hidepid).
 *
 * Uses: process_get_by_pid / process_caller_uid (core/process),
 *       def_hidepid_visible (security/proc_info_leak/defence/hidepid).
 */
#include "modules/process_management/syscalls/kcmp/sys_kcmp.h"            // this file
#include "modules/process_management/core/process/process.h"              // process_get_by_pid, process_caller_uid
#include "modules/process_management/security/proc_info_leak/defence/hidepid/def_hidepid.h"  // def_hidepid_visible

static int order(uint32_t a, uint32_t b) {
    return a == b ? 0 : a < b ? 1 : 2;
}

int sys_kcmp(uint32_t pid1, uint32_t pid2, int type, uint32_t idx1, uint32_t idx2) {
    process_t *a = process_get_by_pid(pid1), *b = process_get_by_pid(pid2);
    uint32_t viewer = process_caller_uid();
    // DEFENCE hidepid: comparing reveals details of both.
    if (!a || !b || !def_hidepid_visible(viewer, a) || !def_hidepid_visible(viewer, b))
        return -1;

    switch (type) {
        case KCMP_FILE:
            if (idx1 >= MAX_FDS || idx2 >= MAX_FDS || !a->fd_table[idx1] || !b->fd_table[idx2])
                return -1;                              // EBADF
            return order((uint32_t)a->fd_table[idx1], (uint32_t)b->fd_table[idx2]);
        case KCMP_VM:
            return 0;                                   // one address space for all
        case KCMP_FILES: case KCMP_FS: case KCMP_SIGHAND: case KCMP_IO: case KCMP_SYSVSEM:
            return order((uint32_t)a, (uint32_t)b);     // each process has its own
        default:
            return -1;
    }
}
