/*
 * atk_toctou_race.c - ATTACK toctou_race: strike in the check-to-use gap.
 *
 * Breaks:     trust - a condition checked must still hold when it is used
 * Stopped by: atomic_check (defence/atomic_check)
 *
 * TOCTOU = Time Of Check to Time Of Use. A program asks "is this file
 * mine?" and, a moment later, opens it. In the gap an attacker swaps the
 * file for a symlink to /etc/secret, and the program opens THAT with its
 * own rights. The fix is to never split the check from the use: do both
 * in one step (open with O_CREAT|O_EXCL, use a file descriptor, ...).
 *
 * simulate(): walk through the classic check-then-open race.
 * launch():   compare the two-step form (unsafe) with the one-step form
 *             (safe) through the rule. 0 findings = atomic form is safe.
 * cleanup():  nothing.
 *
 * Uses: atomic_check (its defence), vga.
 */

#include "registry.h"
#include "def_atomic_check.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  1. check: is /tmp/x mine?        (yes)\n");
    vga_print_str("  2. attacker swaps /tmp/x -> /etc/secret\n");
    vga_print_str("  3. use: open /tmp/x              (opens the secret)\n");
    vga_print_str("  fix: one atomic step, no gap between check and use\n");
    return 1;
}

static int launch(void) {
    int findings = 0;
    if (def_check_is_atomic(0)) findings++;     // two-step must be unsafe
    if (!def_check_is_atomic(1)) findings++;    // one-step must be safe
    vga_print_str("  two-step check-then-use is a race: ");
    vga_print_str(def_check_is_atomic(0) ? "NO (called safe!)\n" : "yes\n");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "toctou_race",
    "swap a resource between the check and the use of it",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "atomic_check"
};

void atk_toctou_race_register(void) { registry_add_attack(&attack); }
