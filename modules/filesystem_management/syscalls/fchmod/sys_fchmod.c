/*
 * sys_fchmod.c - fchmod(fd, mode): chmod the file behind a descriptor.
 *
 * The mode is Linux's (0644 = owner rw, group r, others r). UzOS keeps
 * two digits - owner and everybody else - so it takes the owner's read
 * and write bits and the OTHERS' read and write bits; group and execute
 * bits have no meaning here. Only the owner (or root) may change it
 * (perm_may_chmod). ctime becomes now. Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_cwd / vfs_start_dir / vfs_now (core/vfs),
 *       perm_may_chmod (user_management), the current process
 *       (process_management) for its descriptors.
 */
#include "modules/filesystem_management/syscalls/fchmod/sys_fchmod.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_lookup, vfs_now
#include "modules/user_management/core/perm/perm.h"                   // perm_may_chmod
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process

// Linux mode bits (0644) -> UzOS's two digits (owner, others): read and
// write of the owner, and of the others (UzOS has no group, no x bit).
static uint32_t uzos_mode(uint32_t linux_mode) {
    uint32_t owner  = (linux_mode >> 6) & 6;
    uint32_t others = linux_mode & 6;
    return owner << 3 | others;
}

static int change_mode(vfs_node_t *node, uint32_t mode) {
    if (!node || !perm_may_chmod(node))       // its owner, or root
        return -1;
    node->mode  = uzos_mode(mode);
    node->ctime = vfs_now();
    return 0;
}

int sys_fchmod(int fd, uint32_t mode) {
    process_t *self = scheduler_get_current_process();
    if (!self || fd < 0 || fd >= MAX_FDS)
        return -1;
    return change_mode(self->fd_table[fd], mode);
}
