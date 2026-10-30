/*
 * pidfd.c - The pidfd nodes: one per open handle.
 *
 * Like perf_event_open's counters, a pidfd is a vfs_node_t embedded in
 * a small struct. fd_alloc() opens it, close() / fd_cleanup() close it;
 * when its open_count is 0 the entry may be used again. Reading or
 * writing a pidfd does nothing (no read/write functions), as in Linux.
 *
 * Shared state (core rule 2): pidfd_open makes them; pidfd_send_signal,
 * pidfd_getfd and waitid(P_PIDFD) look through them.
 *
 * Uses: def_pcb_ref / def_pcb_ref_get (security/use_after_exit/defence/
 *       pcb_generation), strncpy / memset (lib/string), cpu_irq_save.
 */

#include "modules/process_management/core/pidfd/pidfd.h"                                         // this part
#include "modules/process_management/security/use_after_exit/defence/pcb_generation/def_pcb_generation.h"  // def_pcb_ref, def_pcb_ref_get
#include "lib/string/string.h"                                                                   // memset, strncpy
#include "arch/x86/cpu/cpu.h"                                                                    // cpu_irq_save, cpu_irq_restore

typedef struct {
    vfs_node_t node;            // FIRST: a pointer to the node is a pointer to the pidfd
    int        used;
    pcb_ref_t  ref;             // the one process it names
} pidfd_t;

static pidfd_t pidfds[PIDFD_MAX];

vfs_node_t *pidfd_create(process_t *target) {
    uint32_t irq = cpu_irq_save();
    for (int i = 0; i < PIDFD_MAX; i++) {
        pidfd_t *f = &pidfds[i];
        if (f->used && f->node.open_count > 0)
            continue;                           // still open somewhere
        memset(f, 0, sizeof(*f));
        f->used = 1;
        f->ref  = def_pcb_ref(target);
        strncpy(f->node.name, "[pidfd]", FS_NAME_MAX);
        f->node.flags  = FS_FILE;
        f->node.mode   = 0;                     // nothing to read or write
        f->node.owner  = target->uid;
        f->node.parent = &f->node;              // in no directory
        cpu_irq_restore(irq);
        return &f->node;
    }
    cpu_irq_restore(irq);
    return 0;
}

int pidfd_is(const vfs_node_t *node) {
    return node >= &pidfds[0].node && node <= &pidfds[PIDFD_MAX - 1].node;
}

process_t *pidfd_target(const process_t *owner, int fd) {
    if (!owner || fd < 0 || fd >= MAX_FDS || !pidfd_is(owner->fd_table[fd]))
        return 0;
    pidfd_t *f = (pidfd_t *)owner->fd_table[fd];
    // DEFENCE pcb_generation: the handle names one life of one slot.
    return def_pcb_ref_get(&f->ref);
}
