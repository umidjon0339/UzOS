#ifndef SYS_EXECVE_H
#define SYS_EXECVE_H
/*
 * sys_execve.h - execve(path, argv, envp): replace the calling process's
 * program with the one at 'path'.
 *
 * STUB as a SYSCALL. A real execve() throws away the caller's memory and
 * loads a new program in its place, keeping the same pid. UzOS loads ELF
 * programs through the `exec` COMMAND (elf_load_and_run, core/elf), which
 * runs them as a fresh process rather than replacing the caller in place
 * (in-place replacement needs per-process address spaces). Returns -1;
 * use the `exec` command to run a program.
 */
#define SYS_EXECVE 67       // EAX=67, EBX=path, ECX=argv, EDX=envp
int sys_execve(const char *path, char *const argv[], char *const envp[]);
#endif
