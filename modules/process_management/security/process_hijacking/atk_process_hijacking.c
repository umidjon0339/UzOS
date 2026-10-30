/*
 * atk_process_hijacking.c - ATTACK process_hijacking (concept).
 *
 * Breaks:     integrity - a process must run its own code
 * Stopped by: control_flow_integrity (defence/control_flow_integrity)
 *
 * Hijacking redirects a running program to attacker-chosen code: a
 * corrupted return address, an overwritten function pointer, or a tampered
 * saved frame. The structural fix is Control-Flow Integrity - every
 * indirect call/return must land on a known-good target - which is a
 * compiler and hardware feature. UzOS's contribution is isolation: the
 * kernel owns each process's entry and return frames, out of ring 3's reach.
 *
 * Uses: control_flow_integrity (its defence), vga.
 */

#include "modules/security_management/core/registry/registry.h"                                                               // attack_t, registry_add_attack
#include "modules/process_management/security/process_hijacking/defence/control_flow_integrity/def_control_flow_integrity.h"  // def_entry_is_kernel_set
#include "modules/device_management/vga/vga.h"                                                                                // vga_print_str

static int simulate(void) {
    vga_print_str("  overwrite a return address or function pointer\n");
    vga_print_str("  the program then runs attacker-chosen code\n");
    vga_print_str("  fix: CFI (compiler+hardware); UzOS owns entry frames\n");
    return 1;
}
static int launch(void) {
    vga_print_str("  not run: concept (CFI is a compiler/hardware feature)\n");
    return def_entry_is_kernel_set() ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "process_hijacking",
    "redirect a running program to attacker-chosen code (concept)",
    INV_INTEGRITY, SEV_INFO,
    simulate, launch, cleanup,
    "control_flow_integrity"
};
void atk_process_hijacking_register(void) { registry_add_attack(&attack); }
