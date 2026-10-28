/*
 * atk_spectre.c - ATTACK spectre (ILLUSTRATION): leak across a boundary
 *                 the CPU crosses speculatively.
 *
 * Breaks:     disclosure - a value behind a check must not leak
 * Stopped by: user_kernel_isolation (defence/user_kernel_isolation)
 *
 * Spectre makes the CPU run code it will later undo - past an array
 * bound, or down a mispredicted branch - and reads a forbidden value in
 * that window, leaking it through the cache. It needs a branch
 * predictor and out-of-order execution to train and time; UzOS has
 * neither to offer, so it CANNOT run a real Spectre. We show the
 * boundary it targets: the user/kernel line in the page tables.
 *
 * simulate(): explain speculative execution and the cache leak.
 * launch():   confirm the user/kernel boundary is in place.
 * cleanup():  nothing.
 *
 * Uses: user_kernel_isolation (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/spectre/defence/user_kernel_isolation/def_user_kernel_isolation.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  Spectre runs code speculatively past a check, reads a\n");
    ui_printf("  forbidden value, and leaks it by cache timing\n");
    ui_printf("  it needs branch prediction + out-of-order execution\n");
    ui_printf("  UzOS has none to train; the boundary is user vs kernel\n");
    return 1;
}

static int launch(void) {
    int ok = def_isolation_boundary_ok();
    ui_printf("  user/kernel boundary intact: %s\n", ok ? "yes" : "NO");
    return ok ? 0 : 1;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "spectre",
    "leak a value across a speculatively-crossed boundary (illustration)",
    INV_DISCLOSURE, SEV_INFO,
    simulate, launch, cleanup,
    "user_kernel_isolation"
};

void atk_spectre_register(void) { registry_add_attack(&attack); }
