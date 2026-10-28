/*
 * def_vm_isolation.c - DEFENCE vm_isolation (CONCEPT): a guest stays a
 *                      guest.
 *
 * Stops:  hypervisor_escape (CONCEPT - see README)
 * Called: nothing in UzOS: the boundary is enforced by the HOST
 *         (QEMU/KVM), not by the guest. UzOS is the guest and cannot,
 *         and should not, implement an escape.
 * Rule:   a guest's CPU, memory and devices are virtual; the host keeps
 *         them walled off from itself and from other guests. The
 *         defences - hypervisor patches, least privilege, strict device
 *         models - all live in the host.
 * Why:    honesty - writing a working QEMU escape would be a real
 *         exploit, out of scope for a teaching OS. This documents the
 *         boundary and where it is kept.
 */

#include "modules/cpu_management/security/hypervisor_escape/defence/vm_isolation/def_vm_isolation.h"
#include "modules/security_management/core/registry/registry.h"

int def_guest_stays_in_its_box(void) { return 1; }   // enforced by the host

static int selftest(void) { return def_guest_stays_in_its_box(); }

static const defence_t defence = {
    "vm_isolation",
    "concept: the host walls off the guest; escape is a host-side exploit",
    "hypervisor_escape",
    selftest
};
void def_vm_isolation_register(void) { registry_add_defence(&defence); }
