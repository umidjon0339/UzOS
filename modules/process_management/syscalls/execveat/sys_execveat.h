#ifndef SYS_EXECVEAT_H
#define SYS_EXECVEAT_H
/* sys_execveat.h - execveat(dirfd, path, argv, envp, flags): execve, path relative to a directory. */
#include <stdint.h>
#include "arch/x86/isr/isr.h"                                   // registers_t
#define SYS_EXECVEAT 68     // EAX=68, EBX=dirfd, ECX=path, EDX=argv, ESI=envp, EDI=flags
#define AT_FDCWD (-100)     // "relative to my current directory"
int sys_execveat(registers_t *regs, int dirfd, const char *path, char *const argv[],
                 char *const envp[], int flags);
#endif
