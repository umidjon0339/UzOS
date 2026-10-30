/*
 * def_no_new_privs.c - DEFENCE no_new_privs: a promise that cannot be undone.
 *
 * Stops:  no_new_privs_bypass (security/no_new_privs_bypass)
 * Called: sys_seccomp() (installing a filter); process_create() and
 *         sys_prctl() keep the flag's two other rules.
 * Rules:  1. a normal user may install a seccomp filter only with
 *            no_new_privs set (root: always)
 *         2. no_new_privs can be turned on, never off (prctl takes 1 only)
 *         3. every child inherits it (process_create), and the seccomp
 *            filter with it
 * Why:    a seccomp filter can make chosen syscalls FAIL. Aimed at a
 *         more privileged program (Linux: a setuid one), "setuid()
 *         failed, but I carry on as root" is a classic hole. With
 *         no_new_privs, nothing this process starts can ever be more
 *         privileged than it, so its filter cannot be turned against a
 *         stronger program. Linux has exactly this rule.
 *
 * Uses: caps_has / CAP_SETUID (security_management core/caps), audit_log, registry.
 */

#include "modules/process_management/security/no_new_privs_bypass/defence/no_new_privs/def_no_new_privs.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_SETUID

static int seccomp_ok(int no_new_privs, int privileged) {
    return no_new_privs || privileged;
}

int def_nnp_seccomp_ok(const process_t *p) {
    if (p && seccomp_ok(p->no_new_privs, caps_has(p->uid, CAP_SETUID)))
        return 1;
    audit_log("no_new_privs", "seccomp filter refused: set no_new_privs first");
    return 0;
}

static int selftest(void) {
    if (seccomp_ok(0, 0)) return 0;         // a user without the promise: no
    if (!seccomp_ok(1, 0)) return 0;        // with no_new_privs: yes
    if (!seccomp_ok(0, 1)) return 0;        // root: yes
    return 1;
}

static const defence_t defence = {
    "no_new_privs",
    "seccomp needs no_new_privs; it is sticky and inherited",
    "no_new_privs_bypass",
    selftest
};

void def_no_new_privs_register(void) { registry_add_defence(&defence); }
