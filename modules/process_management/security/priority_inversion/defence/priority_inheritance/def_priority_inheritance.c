/*
 * def_priority_inheritance.c - DEFENCE priority_inheritance (concept).
 *
 * Stops:  priority_inversion (security/priority_inversion)
 * Called: nothing in UzOS yet: user programs share no locks (there is
 *         no futex), and the kernel's own critical sections turn
 *         interrupts off instead of sleeping on a lock.
 * Rule:   while a high-priority process waits for a lock, the process
 *         HOLDING it runs at the waiter's priority, until it lets go.
 * Why:    LOW holds a lock, HIGH waits for it, MEDIUM (who needs no lock)
 *         keeps the CPU from LOW: HIGH waits for MEDIUM, the priorities
 *         are upside down. It froze the Mars Pathfinder lander in 1997
 *         until priority inheritance was switched on by radio. Linux:
 *         PI futexes (FUTEX_LOCK_PI), rt_mutex.
 *
 * Uses: registry (security_management).
 */

#include "modules/process_management/security/priority_inversion/defence/priority_inheritance/def_priority_inheritance.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence

int def_pi_priority(int holder, int waiter) {
    return waiter > holder ? waiter : holder;
}

static int selftest(void) {
    if (def_pi_priority(1, 7) != 7) return 0;   // LOW holds, HIGH waits: LOW runs high
    if (def_pi_priority(5, 2) != 5) return 0;   // never lowered
    if (def_pi_priority(4, 4) != 4) return 0;
    return 1;
}

static const defence_t defence = {
    "priority_inheritance",
    "concept: a lock holder runs at its highest waiter's priority",
    "priority_inversion",
    selftest
};

void def_priority_inheritance_register(void) { registry_add_defence(&defence); }
