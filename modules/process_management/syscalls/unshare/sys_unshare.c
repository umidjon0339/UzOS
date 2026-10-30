/*
 * sys_unshare.c - unshare(flags): stop sharing something.
 *
 * UzOS processes never share a descriptor table or a current directory
 * (clone refuses CLONE_FILES / CLONE_FS), so asking to unshare those is
 * already true: 0, as Linux answers when nothing is shared.
 * The NAMESPACE flags (CLONE_NEWPID, CLONE_NEWNS, ...) would give the
 * caller its own view of PIDs, mounts, hostname, users: containers. UzOS
 * has no namespaces (see security/container_escape), so they fail (-1,
 * EINVAL) instead of pretending to isolate anything.
 *
 * Uses: nothing (it only checks the flags).
 */
#include "modules/process_management/syscalls/unshare/sys_unshare.h"     // this file

#define NOTHING_TO_DO (UNSHARE_FS | UNSHARE_FILES | UNSHARE_SYSVSEM)

int sys_unshare(uint32_t flags) {
    if (flags & ~NOTHING_TO_DO)
        return -1;              // a namespace, or an unknown flag: not in UzOS
    return 0;                   // already private
}
