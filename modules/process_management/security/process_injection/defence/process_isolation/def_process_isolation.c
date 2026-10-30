/*
 * def_process_isolation.c - DEFENCE process_isolation (partial/concept).
 *
 * Stops:  process_injection (security/process_injection)
 * Called: the CPU, via the PAGE_USER bit, for ring 3 code (core/umem).
 * Rule:   ring 3 sees only its own user pages; it cannot reach kernel
 *         memory or (for a true per-process address space) another
 *         process's memory.
 * Why:    injection writes code or data into another process's memory.
 *         The real barrier is a separate address space per process.
 *         UzOS isolates ring 3 from the kernel, but its kernel-mode
 *         tasks share one address space, so full isolation here is a
 *         concept - the lesson is what a complete barrier requires.
 */

#include "modules/process_management/security/process_injection/defence/process_isolation/def_process_isolation.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                                     // defence_t, registry_add_defence

int def_ring3_isolated(void) { return 1; }   // ring3 cannot reach the kernel

static int selftest(void) { return def_ring3_isolated(); }

static const defence_t defence = {
    "process_isolation",
    "concept: ring3 is walled off; full per-process isolation needs own AS",
    "process_injection",
    selftest
};

void def_process_isolation_register(void) { registry_add_defence(&defence); }
