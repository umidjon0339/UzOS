/*
 * exitlog.c - The ring of exit records (see exitlog.h).
 *
 * Kernel caller (core rule 1): the reaper. Shared state (rule 2): every
 * wait syscall takes from the same records, so a child is reported once.
 *
 * Uses: cpu_irq_save (arch/x86/cpu).
 */

#include "modules/process_management/core/exitlog/exitlog.h"  // this part
#include "arch/x86/cpu/cpu.h"                                 // cpu_irq_save, cpu_irq_restore

static struct {
    int           used;
    uint32_t      order;        // smaller = ended earlier
    exit_record_t r;
} records[EXITLOG_SIZE];
static uint32_t next_order;

void exitlog_add(const process_t *p) {
    if (p->tgid && p->tgid != p->pid)
        return;                 // a thread (clone CLONE_THREAD): nobody waits for it
    uint32_t irq = cpu_irq_save();
    int slot = 0;
    for (int i = 0; i < EXITLOG_SIZE; i++) {        // a free one, else the oldest
        if (!records[i].used) { slot = i; break; }
        if (records[i].order < records[slot].order) slot = i;
    }
    records[slot].used  = 1;
    records[slot].order = next_order++;
    exit_record_t *r = &records[slot].r;
    r->pid  = p->pid;
    r->ppid = p->ppid;
    r->pgid = p->pgid;
    r->uid  = p->uid;
    r->status       = p->exit_status;
    r->user_ticks   = p->user_ticks + p->child_user_ticks;
    r->kernel_ticks = p->kernel_ticks + p->child_kernel_ticks;
    r->nvcsw  = p->nvcsw + p->child_nvcsw;
    r->nivcsw = p->nivcsw + p->child_nivcsw;
    cpu_irq_restore(irq);
}

static int matches(const exit_record_t *r, int32_t pid, uint32_t my_pgid) {
    if (pid > 0)   return r->pid == (uint32_t)pid;
    if (pid == -1) return 1;
    if (pid == 0)  return r->pgid == my_pgid;
    return r->pgid == (uint32_t)-pid;
}

int exitlog_take(uint32_t parent, int32_t pid, uint32_t my_pgid, exit_record_t *out) {
    uint32_t irq = cpu_irq_save();
    int best = -1;
    for (int i = 0; i < EXITLOG_SIZE; i++) {
        if (!records[i].used || records[i].r.ppid != parent || !matches(&records[i].r, pid, my_pgid))
            continue;
        if (best < 0 || records[i].order < records[best].order)
            best = i;
    }
    if (best >= 0) {
        if (out)
            *out = records[best].r;
        records[best].used = 0;
    }
    cpu_irq_restore(irq);
    return best >= 0;
}

int exitlog_find(uint32_t pid, exit_record_t *out) {
    for (int i = 0; i < EXITLOG_SIZE; i++) {
        if (records[i].used && records[i].r.pid == pid) {
            if (out)
                *out = records[i].r;
            return 1;
        }
    }
    return 0;
}
