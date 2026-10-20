#ifndef USERMODE_H
#define USERMODE_H

/*
 * usermode.h - Drop from ring 0 (kernel) to ring 3 (user).
 *
 * The CPU has no "go to ring 3" instruction. Instead we build the stack
 * frame an interrupt would leave behind and run 'iret' on it.
 * The way back is an interrupt: the CPU then loads the kernel stack
 * from the TSS (see arch/x86/tss/).
 */

#include <stdint.h>

// Start running at 'eip' in ring 3 with stack 'esp'. Never returns.
void switch_to_user_mode(uint32_t eip, uint32_t esp);

#endif
