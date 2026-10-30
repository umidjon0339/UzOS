/*
 * sys_setns.c - setns(fd, nstype): join the namespace 'fd' refers to.
 *
 * On Linux, fd is /proc/<pid>/ns/<type> (or a pidfd), and the caller
 * moves into that container's view of the system - which is why it
 * needs CAP_SYS_ADMIN in the target. UzOS has no namespaces (see
 * security/container_escape), so there is nothing to join: always -1
 * (EINVAL). It exists so programs get an honest "not supported".
 *
 * Uses: nothing.
 */
#include "modules/process_management/syscalls/setns/sys_setns.h"     // this file

int sys_setns(int fd, uint32_t nstype) {
    (void)fd;
    (void)nstype;
    return -1;
}
