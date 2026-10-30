#ifndef DEF_SIGFRAME_CHECK_H
#define DEF_SIGFRAME_CHECK_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"  // process_t
#include "modules/process_management/core/signal/signal.h"    // sigcontext_t

// The EFLAGS bits a program may set for itself: the arithmetic flags,
// direction, single step, alignment check, resume. Never IOPL, NT or VM.
#define SIGFRAME_USER_EFLAGS 0x00050DD5u
#define SIGFRAME_FIXED_EFLAGS 0x00000202u      // IF on, and bit 1 (always 1)

// rt_sigreturn asks this before it restores 'ctx', which the program
// says is in the frame at 'frame'. 1 = allowed: 'frame' is the newest
// frame the kernel itself pushed for 'p' (taken off p's list now), EIP
// and ESP point into user space, and ctx->eflags has been cut down to
// the bits a program may set. 0 = refused (logged): the caller ends the
// program with SIGSEGV.
int def_sigframe_check(process_t *p, uint32_t frame, sigcontext_t *ctx);

#endif
