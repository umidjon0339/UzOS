/*
 * atk_tty_hijack.c - ATTACK tty_hijack (concept): type into someone
 * else's shell.
 *
 * Breaks:     integrity - only the person at the keyboard types commands
 * Stopped by: tty_guard (defence/tty_guard)
 *
 * A program running on root's terminal as another user pushes
 * characters into the terminal's input (Linux: ioctl TIOCSTI) - a whole
 * command and an Enter. When it exits, root's shell reads them and runs
 * the command as root.
 *
 * simulate(): the story.
 * launch():   CONCEPT: UzOS has no input-injection call, so nothing can
 *             be pushed. It shows the rule's answers. findings = 0.
 * cleanup():  nothing.
 *
 * Uses: def_tty_inject_ok (its defence), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/tty_hijack/defence/tty_guard/def_tty_guard.h"  // def_tty_inject_ok
#include "modules/ui/core/style/ui.h"                             // ui_printf

static int simulate(void) {
    ui_printf("  `su guest -c prog` runs prog on root's terminal\n");
    ui_printf("  prog pushes \"chmod 777 /etc/shadow\\n\" into the input (TIOCSTI)\n");
    ui_printf("  prog exits; root's shell reads it and runs it as root\n");
    return 1;
}

static int launch(void) {
    ui_printf("  concept: UzOS has no input-injection call (only the keyboard types)\n");
    ui_printf("  the rule: same session -> %s, other session -> %s\n",
              def_tty_inject_ok(2, 2, 0) ? "allowed" : "refused",
              def_tty_inject_ok(9, 2, 0) ? "allowed" : "refused");
    return 0;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "tty_hijack",
    "push keystrokes into another session's terminal (concept)",
    INV_INTEGRITY, SEV_INFO,
    simulate, launch, cleanup,
    "tty_guard"
};

void atk_tty_hijack_register(void) { registry_add_attack(&attack); }
