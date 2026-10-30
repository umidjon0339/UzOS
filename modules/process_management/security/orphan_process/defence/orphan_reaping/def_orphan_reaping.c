/*
 * def_orphan_reaping.c - DEFENCE orphan_reaping: no orphan is left behind.
 *
 * Stops:  orphan_process (security/orphan_process)
 * Called: the reaper (core/scheduler, defence reaper), which runs over
 *         the whole table and collects any ZOMBIE, parent alive or not.
 * Rule:   a zombie is collected because it is a zombie - the reaper does
 *         not wait for a parent to call wait(). So a child whose parent
 *         has died is still freed.
 * Why:    on Linux an orphan is re-parented to init (pid 1) so it is
 *         reaped. UzOS has a global reaper instead, which reaches every
 *         zombie directly; the result is the same - no orphan leaks a
 *         process slot. (zombie_leak/reaper is the same reaper seen from
 *         the "parent never reaped me" angle.)
 */

#include "modules/process_management/security/orphan_process/defence/orphan_reaping/def_orphan_reaping.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                            // defence_t, registry_add_defence

static int collected(int is_zombie, int parent_alive) {
    (void)parent_alive;             // the reaper does not care
    return is_zombie != 0;
}

int def_orphan_is_collected(int is_zombie, int parent_alive) {
    return collected(is_zombie, parent_alive);
}

static int selftest(void) {
    if (!collected(1, 0)) return 0;     // zombie, parent dead: still collected
    if (!collected(1, 1)) return 0;     // zombie, parent alive: collected
    if (collected(0, 0)) return 0;      // not a zombie: nothing to collect
    return 1;
}

static const defence_t defence = {
    "orphan_reaping",
    "a global reaper frees every zombie, so an orphan never leaks a slot",
    "orphan_process",
    selftest
};

void def_orphan_reaping_register(void) { registry_add_defence(&defence); }
