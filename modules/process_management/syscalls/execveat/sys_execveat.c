/*
 * sys_execveat.c - execveat(dirfd, path, argv, envp, flags).
 *
 * execve() with the path looked up from a directory descriptor. UzOS
 * descriptors name files, never directories, so the only dirfd it can
 * use is AT_FDCWD (the current directory) - or an absolute path, which
 * ignores dirfd anyway, as in Linux. Then it IS execve(). flags must be 0.
 *
 * Uses: sys_execve (syscalls/execve).
 */
#include "modules/process_management/syscalls/execveat/sys_execveat.h"   // this file
#include "modules/process_management/syscalls/execve/sys_execve.h"       // sys_execve

int sys_execveat(registers_t *regs, int dirfd, const char *path, char *const argv[],
                 char *const envp[], int flags) {
    if (flags || !path || (path[0] != '/' && dirfd != AT_FDCWD))
        return -1;                      // EINVAL / ENOTDIR: no directory descriptors
    return sys_execve(regs, path, argv, envp);
}
