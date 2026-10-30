/*
 * def_pcb_generation.c - DEFENCE pcb_generation: a saved PCB pointer is
 * checked before it is used again.
 *
 * Stops:  use_after_exit (security/use_after_exit)
 * Called: core/pidfd, on every use of a pidfd (pidfd_send_signal,
 *         pidfd_getfd, waitid P_PIDFD); anyone who keeps a process
 *         across a sleep.
 * Rule:   a reference is {slot, pid, generation}. It is valid only while
 *         the slot is in use AND still holds that pid and generation.
 *         process_create() counts the slot's generation up on every new
 *         life (outside the PCB, so pcb_wipe cannot reset it).
 * Why:    the process table has 64 slots and they are reused. A kernel
 *         path that keeps a process_t * while it sleeps may wake up to
 *         find the process gone and its slot given to someone else; a
 *         signal, an exit status or a permission check would then hit
 *         the newcomer. Linux keeps a reference count on the task (and
 *         struct pid) for the same reason.
 *
 * Uses: audit_log, registry (security_management).
 */

#include "modules/process_management/security/use_after_exit/defence/pcb_generation/def_pcb_generation.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

pcb_ref_t def_pcb_ref(process_t *p) {
    pcb_ref_t ref = { p, p ? p->pid : 0, p ? p->generation : 0 };
    return ref;
}

// 1 = the same life, 0 = freed, -1 = someone else lives there now.
static int check(const process_t *slot, uint32_t pid, uint32_t generation) {
    if (!slot || slot->state == PROCESS_STATE_FREE)
        return 0;
    if (slot->pid != pid || slot->generation != generation)
        return -1;
    return 1;
}

process_t *def_pcb_ref_get(const pcb_ref_t *ref) {
    int r = check(ref->slot, ref->pid, ref->generation);
    if (r < 0)
        audit_log("pcb_generation", "stale process reference refused: the slot has a new owner");
    return r > 0 ? ref->slot : 0;
}

static int selftest(void) {
    process_t p = {0};
    p.state = PROCESS_STATE_SLEEPING;
    p.pid = 7;
    p.generation = 3;
    if (check(&p, 7, 3) != 1) return 0;     // the same life: fine
    p.state = PROCESS_STATE_FREE;
    if (check(&p, 7, 3) != 0) return 0;     // freed: gone
    p.state = PROCESS_STATE_READY;
    p.pid = 9;
    p.generation = 4;
    if (check(&p, 7, 3) != -1) return 0;    // a newcomer: refused
    p.pid = 7;                              // even with the same pid,
    if (check(&p, 7, 3) != -1) return 0;    // a new generation is someone else
    if (check(0, 7, 3) != 0) return 0;
    return 1;
}

static const defence_t defence = {
    "pcb_generation",
    "a saved process reference is checked: same slot, same pid, same life",
    "use_after_exit",
    selftest
};

void def_pcb_generation_register(void) { registry_add_defence(&defence); }
