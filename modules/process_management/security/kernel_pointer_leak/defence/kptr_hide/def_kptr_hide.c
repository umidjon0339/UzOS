/*
 * def_kptr_hide.c - DEFENCE kptr_hide: kernel addresses print as 0.
 *
 * Stops:  kernel_pointer_leak (security/kernel_pointer_leak)
 * Called: procinfo (commands/procinfo), before it prints a process's
 *         kernel stack, saved stack pointer or entry function.
 * Rule:   a kernel address is shown only to CAP_PTRACE (root); anyone
 *         else sees 0x00000000.
 * Why:    an exploit needs to know WHERE things are: the address of a
 *         kernel stack tells an attacker where to aim an overflow, the
 *         address of a function where to jump. Hiding them is cheap and
 *         turns a precise attack into guessing. Linux: kptr_restrict
 *         (the %pK format prints zeros).
 *
 * Uses: caps_has / CAP_PTRACE (security_management core/caps), audit_log,
 *       registry.
 */

#include "modules/process_management/security/kernel_pointer_leak/defence/kptr_hide/def_kptr_hide.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log
#include "modules/security_management/core/caps/caps.h"           // caps_has, CAP_PTRACE

static uint32_t shown(int viewer_has_cap, uint32_t addr) {
    return viewer_has_cap ? addr : 0;
}

uint32_t def_kptr_show(uint32_t viewer_uid, uint32_t addr) {
    uint32_t out = shown(caps_has(viewer_uid, CAP_PTRACE), addr);
    if (addr && !out)
        audit_log("kptr_hide", "kernel address hidden from a normal user");
    return out;
}

static int selftest(void) {
    if (shown(0, 0x00105000) != 0) return 0;            // user: zero
    if (shown(1, 0x00105000) != 0x00105000) return 0;   // root: the address
    if (shown(0, 0) != 0) return 0;
    return 1;
}

static const defence_t defence = {
    "kptr_hide",
    "kernel addresses print as 0 for normal users (kptr_restrict)",
    "kernel_pointer_leak",
    selftest
};

void def_kptr_hide_register(void) { registry_add_defence(&defence); }
