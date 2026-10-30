/*
 * def_sigframe_check.c - DEFENCE sigframe_check: only a real frame comes back.
 *
 * Stops:  sigreturn_forgery (security/sigreturn_forgery)
 * Called: sys_rt_sigreturn() (syscalls/rt_sigreturn), before it loads
 *         the saved registers into the CPU.
 * Rules:  1. there is a handler frame the kernel pushed, and 'frame' is
 *            the newest one (each frame comes back once)
 *         2. the saved EIP and ESP are user-space addresses
 *         3. EFLAGS keeps only the bits a program may set itself:
 *            IOPL, NT and VM are cleared, IF is set
 * Why:    rt_sigreturn loads EVERY register from memory the program
 *         owns. That is "Sigreturn Oriented Programming" (SROP): forge a
 *         frame, call rt_sigreturn without any signal, and the kernel
 *         sets whatever registers you wrote. EFLAGS with IOPL 3 lets a
 *         ring 3 program run cli and in/out: it owns the hardware.
 *         Rule 1 refuses a frame no signal made; rule 3 still strips
 *         IOPL from a real frame the handler changed.
 *
 * Uses: USER_SPACE_START/END (memory_management layout), audit_log,
 *       registry (security_management).
 */

#include "modules/process_management/security/sigreturn_forgery/defence/sigframe_check/def_sigframe_check.h"  // this file
#include "modules/memory_management/core/layout/memory_layout.h"                                              // USER_SPACE_START, USER_SPACE_END
#include "modules/security_management/core/registry/registry.h"                                               // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                                     // audit_log

static int user_address(uint32_t a) {
    return a >= USER_SPACE_START && a < USER_SPACE_END;
}

// The whole rule. 0 = refused, with 'why' set.
static int check(process_t *p, uint32_t frame, sigcontext_t *ctx, const char **why) {
    if (p->sig_frames <= 0 || p->sig_frame[p->sig_frames - 1] != frame) {
        *why = "rt_sigreturn without a signal frame (forged)";
        return 0;
    }
    if (!user_address(ctx->eip) || !user_address(ctx->esp)) {
        *why = "rt_sigreturn to a kernel address";
        return 0;
    }
    p->sig_frames--;                                    // this frame is used up
    ctx->eflags = (ctx->eflags & SIGFRAME_USER_EFLAGS) | SIGFRAME_FIXED_EFLAGS;
    return 1;
}

int def_sigframe_check(process_t *p, uint32_t frame, sigcontext_t *ctx) {
    const char *why = 0;
    if (check(p, frame, ctx, &why))
        return 1;
    audit_log("sigframe_check", why);
    return 0;
}

static int selftest(void) {
    process_t p = {0};
    sigcontext_t c = {0};
    const char *why;

    c.eip = USER_SPACE_START + 0x100;
    c.esp = USER_SPACE_START + 0x2000;
    c.eflags = 0x3202;                                  // IOPL 3: the attack

    if (check(&p, 0x40001F00, &c, &why)) return 0;      // no frame pushed: refused

    p.sig_frame[0] = 0x40001F00;
    p.sig_frames = 1;
    if (check(&p, 0x40001E00, &c, &why)) return 0;      // not the newest frame: refused

    c.eip = 0x00100000;                                 // into the kernel
    if (check(&p, 0x40001F00, &c, &why)) return 0;

    c.eip = USER_SPACE_START + 0x100;
    if (!check(&p, 0x40001F00, &c, &why)) return 0;     // the real one: fine
    if (c.eflags & 0x3000) return 0;                    // ...but IOPL is gone
    if (!(c.eflags & 0x200)) return 0;                  // ...and IF is on
    if (p.sig_frames != 0) return 0;                    // ...and it is used up
    if (check(&p, 0x40001F00, &c, &why)) return 0;      // the same frame twice: refused
    return 1;
}

static const defence_t defence = {
    "sigframe_check",
    "rt_sigreturn restores only a frame the kernel pushed; never IOPL",
    "sigreturn_forgery",
    selftest
};

void def_sigframe_check_register(void) { registry_add_defence(&defence); }
