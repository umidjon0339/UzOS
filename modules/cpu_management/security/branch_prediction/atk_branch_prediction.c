/*
 * atk_branch_prediction.c - ATTACK branch_prediction (ILLUSTRATION):
 *                           train the predictor to steer speculation.
 *
 * Breaks:     disclosure - mispredicted paths must not leak secrets
 * Stopped by: speculation_barrier (defence/speculation_barrier)
 *
 * The branch predictor guesses which way a branch goes so the CPU can
 * run ahead. An attacker trains it to guess WRONG on purpose, so the
 * victim speculatively runs a path that touches a secret, then reads
 * the trace from the cache (this is how Spectre v2 steers). It needs a
 * shared, trainable predictor and out-of-order execution. UzOS runs one
 * task at a time on one CPU and cannot reproduce it; the honest fix is
 * microcode and retpolines, with isolation bounding the rest.
 *
 * simulate(): explain training the predictor and where the real fix is.
 * launch():   confirm isolation still bounds any leak.
 * cleanup():  nothing.
 *
 * Uses: speculation_barrier (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/branch_prediction/defence/speculation_barrier/def_speculation_barrier.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  train the predictor so the victim mis-speculates a path\n");
    ui_printf("  that path touches a secret; read it back from the cache\n");
    ui_printf("  the real fixes are microcode (IBRS/IBPB) and retpolines\n");
    ui_printf("  UzOS: one task at a time, kernel pages hidden - bounded\n");
    return 1;
}

static int launch(void) {
    int ok = def_speculation_bounded();
    ui_printf("  isolation bounds a mis-speculated leak: %s\n",
              ok ? "yes" : "NO");
    return ok ? 0 : 1;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "branch_prediction",
    "mis-train the predictor to steer speculation (illustration)",
    INV_DISCLOSURE, SEV_INFO,
    simulate, launch, cleanup,
    "speculation_barrier"
};

void atk_branch_prediction_register(void) { registry_add_attack(&attack); }
