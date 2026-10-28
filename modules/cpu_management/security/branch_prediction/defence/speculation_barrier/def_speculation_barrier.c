/*
 * def_speculation_barrier.c - DEFENCE speculation_barrier: bound what a
 *                             predictor attack could reach.
 *
 * Stops:  branch_prediction (security/branch_prediction) - ILLUSTRATION
 * Called: nothing new at runtime: the real fixes (IBRS/IBPB microcode,
 *         retpoline compilation) live in the CPU and the compiler. What
 *         UzOS provides is the isolation that bounds the leak (core/vmm).
 * Rule:   with one CPU running one task at a time and kernel pages
 *         hidden from ring 3, a mistrained branch cannot reach another
 *         process's secret. The structural fix belongs to hardware.
 * Why:    honesty - a teaching kernel cannot add microcode, so the
 *         lesson is WHERE the fix lives and what isolation still buys.
 */

#include "modules/cpu_management/security/branch_prediction/defence/speculation_barrier/def_speculation_barrier.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/memory_management/core/vmm/vmm.h"

int def_speculation_bounded(void) {
    vmm_walk_t w;
    if (!vmm_walk(0x00100000, &w))
        return 1;
    return (w.pte & PAGE_PRESENT) && !(w.pte & PAGE_USER);   // boundary holds
}

static int selftest(void) {
    if (!def_speculation_bounded()) return 0;
    return 1;
}

static const defence_t defence = {
    "speculation_barrier",
    "isolation bounds the leak; the real fix is microcode + retpolines",
    "branch_prediction",
    selftest
};

void def_speculation_barrier_register(void) { registry_add_defence(&defence); }
