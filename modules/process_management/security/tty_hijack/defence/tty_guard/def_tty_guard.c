/*
 * def_tty_guard.c - DEFENCE tty_guard (concept).
 *
 * Stops:  tty_hijack (security/tty_hijack)
 * Called: nothing in UzOS: the console has no input-injection call,
 *         and the keyboard driver feeds only the shell.
 * Rule:   pushing characters into a terminal's input as if typed is
 *         only for a process of the session that owns that terminal -
 *         never from another session, never from a sandboxed child.
 * Why:    `su guest -c prog`: prog runs as guest but still on root's
 *         terminal. With TIOCSTI it types "chmod 777 /etc/shadow\n"
 *         into root's shell, which runs it as root once prog exits.
 *         Linux restricted TIOCSTI (and it can be switched off:
 *         dev.tty.legacy_tiocsti = 0); a new session per su (setsid)
 *         also takes the terminal away.
 *
 * Uses: registry (security_management).
 */

#include "modules/process_management/security/tty_hijack/defence/tty_guard/def_tty_guard.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence

int def_tty_inject_ok(uint32_t caller_sid, uint32_t tty_sid, int admin) {
    return admin || caller_sid == tty_sid;
}

static int selftest(void) {
    if (!def_tty_inject_ok(2, 2, 0)) return 0;      // its own terminal
    if (def_tty_inject_ok(9, 2, 0)) return 0;       // another session's: no
    if (!def_tty_inject_ok(9, 2, 1)) return 0;      // an administrator
    return 1;
}

static const defence_t defence = {
    "tty_guard",
    "concept: only a terminal's own session may inject input into it",
    "tty_hijack",
    selftest
};

void def_tty_guard_register(void) { registry_add_defence(&defence); }
