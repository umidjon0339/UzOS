/*
 * atk_process_dos.c - ATTACK process_dos: deny service through processes.
 *
 * Breaks:     liveness - legitimate work must still get to run
 * Stopped by: service_limits (defence/service_limits)
 *
 * Process-based denial of service is the umbrella over several tricks:
 * spawn processes without end (fork_bomb), fill the process table
 * (slot_quota's job), or hog the CPU (cpu_management's cpu_exhaustion /
 * starvation). The common defence is LIMITS: a per-user process cap and
 * CPU quotas, so one account cannot starve everyone else.
 *
 * simulate(): name the tricks and the limit that answers each.
 * launch():   confirm the limits apply. 0 findings = service is defended.
 * cleanup():  nothing (the live demos are the other pairs).
 *
 * Uses: service_limits (its defence), vga.
 */

#include "registry.h"
#include "def_service_limits.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  spawn without end   -> fork_bomb / slot_quota\n");
    vga_print_str("  hog the CPU         -> cpu_exhaustion / starvation (cpu)\n");
    vga_print_str("  common answer: per-user process + CPU limits\n");
    return 1;
}
static int launch(void) {
    int ok = def_service_limits_apply();
    vga_print_str("  per-user process + CPU limits apply: ");
    vga_print_str(ok ? "yes\n" : "NO\n");
    return ok ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "process_dos",
    "deny service by exhausting process slots or CPU (umbrella)",
    INV_LIVENESS, SEV_INFO,
    simulate, launch, cleanup,
    "service_limits"
};
void atk_process_dos_register(void) { registry_add_attack(&attack); }
