/*
 * sys_execve.c - execve() STUB: in-place image replacement is not
 * supported; the `exec` command loads ELF programs instead (see .h).
 */
#include "sys_execve.h"

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    (void)path; (void)argv; (void)envp;
    return -1;      // unsupported: see sys_execve.h
}
