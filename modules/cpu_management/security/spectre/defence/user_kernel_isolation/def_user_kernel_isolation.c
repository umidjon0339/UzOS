/*
 * def_user_kernel_isolation.c - DEFENCE user_kernel_isolation: the ring
 *                               3 / ring 0 line bounds what leaks.
 *
 * Stops:  spectre (security/spectre) - an ILLUSTRATION (see README)
 * Called: the CPU, on every access, via the PAGE_USER bit (core/vmm,
 *         core/umem). Real Spectre mitigation also needs microcode and
 *         speculation barriers the hardware provides.
 * Rule:   kernel memory has no user bit; a ring 3 access faults. Spectre
 *         tries to read across a boundary speculatively (e.g. past an
 *         array bound, or a mispredicted branch) and leak the result by
 *         cache timing. What the page tables forbid bounds the damage.
 * Why:    UzOS cannot train a branch predictor or run a real Spectre;
 *         the lesson is that isolation is the boundary being attacked.
 */

#include "modules/cpu_management/security/spectre/defence/user_kernel_isolation/def_user_kernel_isolation.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/memory_management/core/vmm/vmm.h"

int def_isolation_boundary_ok(void) {
    vmm_walk_t w;
    if (!vmm_walk(0x00100000, &w))          // the kernel base at 1 MB
        return 1;                           // not mapped here: nothing to leak
    return (w.pte & PAGE_PRESENT) && !(w.pte & PAGE_USER);
}

static int selftest(void) {
    if (!def_isolation_boundary_ok()) return 0;    // kernel hidden from ring 3
    return 1;
}

static const defence_t defence = {
    "user_kernel_isolation",
    "kernel memory has no user bit; speculation cannot cross the line",
    "spectre",
    selftest
};

void def_user_kernel_isolation_register(void) { registry_add_defence(&defence); }
