/*
 * atk_hypervisor_escape.c - ATTACK hypervisor_escape (CONCEPT).
 *
 * Breaks:     isolation - a guest must not reach the host
 * Stopped by: vm_isolation (defence/vm_isolation) - CONCEPT
 *
 * A hypervisor escape breaks out of a virtual machine into the host that
 * runs it, usually through a bug in an emulated device or instruction.
 * UzOS runs AS the guest under QEMU, so an escape would mean attacking
 * QEMU itself - a real exploit, and out of scope for a teaching OS. It
 * is shown as a CONCEPT and nothing is launched. The defences live in
 * the host: hypervisor patches, least privilege, strict device models.
 *
 * Uses: vm_isolation (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/hypervisor_escape/defence/vm_isolation/def_vm_isolation.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  break out of the VM into the host, via an emulated-\n");
    ui_printf("  device or instruction bug in the hypervisor\n");
    ui_printf("  UzOS is the guest; an escape attacks QEMU itself\n");
    ui_printf("  defences live in the host, not in this guest\n");
    return 1;
}
static int launch(void) {
    ui_printf("  not run: UzOS is the guest; escape is a host-side exploit\n");
    return def_guest_stays_in_its_box() ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "hypervisor_escape",
    "break out of the VM into the host (concept: UzOS is the guest)",
    INV_ISOLATION, SEV_INFO,
    simulate, launch, cleanup,
    "vm_isolation"
};
void atk_hypervisor_escape_register(void) { registry_add_attack(&attack); }
