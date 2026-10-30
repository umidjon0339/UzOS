/*
 * def_seccomp_filter.c - DEFENCE seccomp_filter: a sandboxed program gets
 * only the syscalls it asked to keep.
 *
 * Stops:  syscall_abuse (security/syscall_abuse)
 * Called: the syscall handler (syscalls/handler), before EVERY int 0x80
 *         of a process that has a seccomp mode (set by syscalls/seccomp).
 * Rules:  strict  read, write, exit, rt_sigreturn - anything else ends
 *                 the program with SIGSYS
 *         filter  the syscalls whose bit is set; anything else gets the
 *                 filter's action: SIGSYS, or "fails with -1"
 *         a filter only ever gets STRICTER: a new one is ANDed with the
 *         old, strict stays strict, and children inherit it
 * Why:    a program that reads untrusted input (a parser, a renderer)
 *         may be taken over by it. If it gave up every syscall it does
 *         not need before it starts, the attacker's code can only use
 *         those few: no open(), no kill(), no setuid(). Chrome, OpenSSH
 *         and systemd services all do this on Linux.
 *
 * Uses: audit_log, registry (security_management).
 */

#include "modules/process_management/security/syscall_abuse/defence/seccomp_filter/def_seccomp_filter.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

// The syscalls strict mode keeps (UzOS numbers).
#define NR_EXIT         1
#define NR_READ         15
#define NR_WRITE        16
#define NR_RT_SIGRETURN 76

static int allowed(int mode, const uint32_t allow[4], uint32_t nr) {
    if (mode == SECCOMP_MODE_STRICT)
        return nr == NR_READ || nr == NR_WRITE || nr == NR_EXIT || nr == NR_RT_SIGRETURN;
    if (mode == SECCOMP_MODE_FILTER)
        return nr < 128 && (allow[nr / 32] >> (nr % 32)) & 1;
    return 1;                                   // no sandbox
}

int def_seccomp_allowed(const process_t *p, uint32_t nr) {
    if (allowed(p->seccomp_mode, p->seccomp_allow, nr))
        return 1;
    audit_log("seccomp_filter", "syscall outside the sandbox refused");
    return 0;
}

uint32_t def_seccomp_action(const process_t *p) {
    return p->seccomp_mode == SECCOMP_MODE_STRICT ? SECCOMP_RET_KILL_PROCESS : p->seccomp_action;
}

static int selftest(void) {
    uint32_t allow[4] = { (1u << 2) | (1u << 16), 0, 0, 0 };    // getpid, write
    if (!allowed(0, allow, 14)) return 0;               // no sandbox: all
    if (!allowed(SECCOMP_MODE_STRICT, allow, 16)) return 0;
    if (allowed(SECCOMP_MODE_STRICT, allow, 2)) return 0;       // getpid: not in strict
    if (!allowed(SECCOMP_MODE_FILTER, allow, 2)) return 0;
    if (allowed(SECCOMP_MODE_FILTER, allow, 14)) return 0;      // open: not allowed
    if (allowed(SECCOMP_MODE_FILTER, allow, 500)) return 0;     // beyond the table
    return 1;
}

static const defence_t defence = {
    "seccomp_filter",
    "a sandboxed program gets only the syscalls its filter kept",
    "syscall_abuse",
    selftest
};

void def_seccomp_filter_register(void) { registry_add_defence(&defence); }
