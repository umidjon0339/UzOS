/*
 * atk_container_escape.c - ATTACK container_escape (concept).
 *
 * Breaks:     isolation - a container must not reach the host
 * Stopped by: namespace_isolation (defence/namespace_isolation)
 *
 * A container is a process boxed in by namespaces (its own view of PIDs,
 * mounts, users, network) plus cgroups and dropped capabilities. An
 * escape breaks out of that box to the host - usually through a leaked
 * capability, a shared mount, or a kernel bug. UzOS has no namespaces or
 * containers, so there is nothing to escape; this documents the defence.
 *
 * Uses: namespace_isolation (its defence), vga.
 */

#include "modules/security_management/core/registry/registry.h"                                                        // attack_t, registry_add_attack
#include "modules/process_management/security/container_escape/defence/namespace_isolation/def_namespace_isolation.h"  // def_namespaces_present
#include "modules/device_management/vga/vga.h"                                                                         // vga_print_str

static int simulate(void) {
    vga_print_str("  break a container's namespace box to reach the host\n");
    vga_print_str("  via a leaked capability, shared mount, or kernel bug\n");
    vga_print_str("  UzOS has no namespaces/containers: nothing to escape\n");
    return 1;
}
static int launch(void) {
    vga_print_str("  not run: UzOS has no containers to break out of\n");
    return def_namespaces_present() ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "container_escape",
    "break out of a container to the host (concept: UzOS has none)",
    INV_ISOLATION, SEV_INFO,
    simulate, launch, cleanup,
    "namespace_isolation"
};
void atk_container_escape_register(void) { registry_add_attack(&attack); }
