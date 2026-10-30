#ifndef SYS_VFORK_H
#define SYS_VFORK_H
/*
 * sys_vfork.h - vfork(): a child that borrows my memory and my stack
 * until it calls execve() or ends; I wait until then.
 */
#include "arch/x86/isr/isr.h"   // registers_t
#define SYS_VFORK 66        // EAX=66
int sys_vfork(const registers_t *regs);
#endif
