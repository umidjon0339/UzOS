#ifndef SYSCALL_HANDLER_H
#define SYSCALL_HANDLER_H

/*
 * syscall_handler.h - The door from user programs into the kernel.
 *
 * A program asks the kernel for something with:
 *
 *     mov eax, <syscall number>   ; which service (SYS_* in syscalls/<name>/)
 *     mov ebx, <argument>         ; its argument, if any
 *     int 0x80                    ; enter the kernel
 *                                 ; the answer is now in eax
 */

// Hook int 0x80. Called once from kernel_main.
void init_syscalls(void);

#endif
