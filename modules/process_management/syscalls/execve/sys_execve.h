#ifndef SYS_EXECVE_H
#define SYS_EXECVE_H
/* sys_execve.h - execve(path, argv, envp): replace my program with another one. */
#include <stdint.h>
#include "arch/x86/isr/isr.h"                                   // registers_t
#define SYS_EXECVE 67       // EAX=67, EBX=path, ECX=argv, EDX=envp

// The user stack each process gets for its new program: the top of its
// own 64 KiB area (EXEC_STACKS_START + slot * 64 KiB, memory_layout.h).
#define EXEC_STACK_PAGES 4

int sys_execve(registers_t *regs, const char *path, char *const argv[], char *const envp[]);
#endif
