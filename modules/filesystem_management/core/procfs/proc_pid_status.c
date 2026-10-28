/*
 * proc_pid_status.c - /proc/<pid>/status: one process, in readable lines.
 *
 *     Name:   worker
 *     State:  S (sleeping)
 *     Tgid:   7                    the process (thread group)
 *     Pid:    7                    this thread
 *     PPid:   2                    who made it
 *     Uid:    0  0  0  0           real, effective, saved, filesystem
 *     Threads:        1
 *     Cpus_allowed:   1            the affinity mask, in hex
 *     Cpus_allowed_list:      0    ...and as a list
 *     Mems_allowed_list:      0    NUMA nodes it may use
 *     voluntary_ctxt_switches:        12    it slept / gave up the CPU
 *     nonvoluntary_ctxt_switches:     3     it was preempted
 *
 * Linux's file has more lines (memory, signals, capabilities); these
 * are the ones about the CPU, plus who the process is.
 *
 * Uses: affinity_get / affinity_list (cpu_management/core/affinity), numa_online_nodes
 *       (cpu_management/core/numa), thread_count (in this file), FS_* fd table size.
 */

#include "modules/filesystem_management/core/procfs/proc_pid_status.h"
#include "modules/cpu_management/core/affinity/affinity.h"      // affinity_get, affinity_list
#include "modules/cpu_management/core/numa/numa.h"              // numa_online_nodes

// Threads alive in thread group 'tgid' (every clone(CLONE_THREAD) child
// has its parent's tgid). Only this file needs it, so it lives here.
static int thread_count(uint32_t tgid) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].tgid == tgid && table[i].state != PROCESS_STATE_FREE &&
            table[i].state != PROCESS_STATE_ZOMBIE)
            n++;
    return n;
}

static const char *state_text(process_state_t s) {
    switch (s) {
        case PROCESS_STATE_RUNNING:  return "R (running)";
        case PROCESS_STATE_READY:    return "R (running)";   // Linux: R = running or ready
        case PROCESS_STATE_SLEEPING: return "S (sleeping)";
        case PROCESS_STATE_ZOMBIE:   return "Z (zombie)";
        default:                     return "X (dead)";
    }
}

void proc_pid_status(proc_buf_t *buf, const process_t *p) {
    char cpus[32], nodes[32];
    affinity_list(affinity_get(p), cpus, sizeof(cpus));
    affinity_list(numa_online_nodes(), nodes, sizeof(nodes));

    proc_printf(buf, "Name:\t%s\n", p->name);
    proc_printf(buf, "State:\t%s\n", state_text(p->state));
    proc_printf(buf, "Tgid:\t%u\n", p->tgid ? p->tgid : p->pid);
    proc_printf(buf, "Pid:\t%u\n", p->pid);
    proc_printf(buf, "PPid:\t%u\n", p->ppid);
    proc_printf(buf, "Uid:\t%u\t%u\t%u\t%u\n", p->uid, p->uid, p->uid, p->uid);
    proc_printf(buf, "FDSize:\t%d\n", MAX_FDS);
    proc_printf(buf, "Threads:\t%d\n", thread_count(p->tgid ? p->tgid : p->pid));
    proc_printf(buf, "VmStk:\t%u kB\n", PROCESS_STACK_SIZE / 1024);
    proc_printf(buf, "Cpus_allowed:\t%x\n", affinity_get(p));
    proc_printf(buf, "Cpus_allowed_list:\t%s\n", cpus);
    proc_printf(buf, "Mems_allowed:\t%x\n", numa_online_nodes());
    proc_printf(buf, "Mems_allowed_list:\t%s\n", nodes);
    proc_printf(buf, "voluntary_ctxt_switches:\t%u\n", p->nvcsw);
    proc_printf(buf, "nonvoluntary_ctxt_switches:\t%u\n", p->nivcsw);
}
