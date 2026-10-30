/*
 * def_handler_bounds.c - DEFENCE handler_bounds: what a handler may be.
 *
 * Stops:  handler_hijack (security/handler_hijack)
 * Called: sys_rt_sigaction() (syscalls/rt_sigaction), before it stores
 *         a new action.
 * Rules:  1. SIGKILL and SIGSTOP cannot be caught or ignored
 *         2. a handler is SIG_DFL, SIG_IGN, or a user-space address
 *         3. with SA_RESTORER, the restorer is a user-space address
 * Why:    SIGKILL and SIGSTOP are how root and the shell always regain
 *         control: a program that could catch them could never be
 *         stopped. A handler or restorer address is where the kernel
 *         sends the CPU later; it must never point into the kernel.
 *
 * Uses: USER_SPACE_START/END (memory_management layout), audit_log,
 *       registry (security_management).
 */

#include "modules/process_management/security/handler_hijack/defence/handler_bounds/def_handler_bounds.h"  // this file
#include "modules/memory_management/core/layout/memory_layout.h"                                           // USER_SPACE_START, USER_SPACE_END
#include "modules/security_management/core/registry/registry.h"                                            // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                                  // audit_log

static int user_address(uint32_t a) {
    return a >= USER_SPACE_START && a < USER_SPACE_END;
}

static const char *refusal(int sig, const k_sigaction_t *act) {
    if (sig == SIGKILL || sig == SIGSTOP)
        return "SIGKILL and SIGSTOP cannot be caught or ignored";
    if (act->handler != SIG_DFL && act->handler != SIG_IGN && !user_address(act->handler))
        return "signal handler outside user space";
    if ((act->flags & SA_RESTORER) && !user_address(act->restorer))
        return "signal restorer outside user space";
    return 0;
}

int def_handler_ok(int sig, const k_sigaction_t *act) {
    const char *why = refusal(sig, act);
    if (!why)
        return 1;
    audit_log("handler_bounds", why);
    return 0;
}

static int selftest(void) {
    k_sigaction_t a = { USER_SPACE_START + 0x100, 0, 0, 0 };
    if (refusal(SIGUSR1, &a)) return 0;             // a normal handler
    if (!refusal(SIGKILL, &a)) return 0;            // catch SIGKILL: no
    a.handler = SIG_IGN;
    if (!refusal(SIGSTOP, &a)) return 0;            // ignore SIGSTOP: no
    if (refusal(SIGTERM, &a)) return 0;             // ignore SIGTERM: fine
    a.handler = 0x00100000;                         // the kernel's code
    if (!refusal(SIGUSR1, &a)) return 0;
    a.handler = USER_SPACE_START + 0x100;
    a.flags = SA_RESTORER;
    a.restorer = 0xC0001000;                        // kernel heap
    if (!refusal(SIGUSR1, &a)) return 0;
    return 1;
}

static const defence_t defence = {
    "handler_bounds",
    "SIGKILL/SIGSTOP keep their default; handlers point into user space",
    "handler_hijack",
    selftest
};

void def_handler_bounds_register(void) { registry_add_defence(&defence); }
