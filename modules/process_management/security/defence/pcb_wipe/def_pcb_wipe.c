#include "proc_security.h"
#include "string.h"

/* ===================================================================
 * DEFENCE: PCB wipe
 * ===================================================================
 * Restores INV_DISCLOSURE - a new process must not inherit stale data.
 *
 * A process slot is reused many times during one boot. If the new
 * owner only writes *some* of the fields, the rest still hold whatever
 * the previous owner left there: its cwd pointer, its open file
 * descriptors, its offsets. That is a real information leak, and a
 * garbage pointer in cwd is also a crash waiting to happen.
 *
 * The fix is one line, applied at the single place where a slot is
 * handed over. Clearing first and writing afterwards means "every
 * field is either mine or zero", which is easy to reason about.
 * =================================================================== */

void def_pcb_wipe(process_t *p) {
    if (!p) return;
    memset(p, 0, sizeof(process_t));
    p->state = PROCESS_STATE_FREE;
}

static int def_pcb_wipe_selftest(void) {
    process_t probe;

    /* pretend a previous process used this slot */
    probe.pid   = 7777;
    probe.uid   = 42;
    probe.cwd   = (struct vfs_node *)0xDEADBEEF;
    probe.state = PROCESS_STATE_RUNNING;
    for (int i = 0; i < MAX_FDS; i++) {
        probe.fd_table[i]  = (struct vfs_node *)0x1000;
        probe.fd_offset[i] = 512;
    }

    def_pcb_wipe(&probe);

    if (probe.pid   != 0) return 0;
    if (probe.uid   != 0) return 0;
    if (probe.cwd   != 0) return 0;
    if (probe.state != PROCESS_STATE_FREE) return 0;
    for (int i = 0; i < MAX_FDS; i++) {
        if (probe.fd_table[i]  != 0) return 0;
        if (probe.fd_offset[i] != 0) return 0;
    }

    return 1;
}

static const defence_t def = {
    "pcb_wipe",
    "zero a process slot before handing it to a new process",
    "uninit_pcb",
    def_pcb_wipe_selftest
};

void def_pcb_wipe_register(void) {
    proc_security_register_defence(&def);
}
