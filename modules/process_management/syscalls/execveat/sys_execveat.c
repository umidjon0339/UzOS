/*
 * sys_execveat.c - execveat() STUB: same as execve (see .h).
 */
#include "sys_execveat.h"

int sys_execveat(int dirfd, const char *path, char *const argv[],
                 char *const envp[], int flags) {
    (void)dirfd; (void)path; (void)argv; (void)envp; (void)flags;
    return -1;      // unsupported: see sys_execveat.h
}
