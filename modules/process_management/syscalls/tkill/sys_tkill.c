/*
 * sys_tkill.c - tkill(tid, sig): signal a single thread by its id.
 *
 * In UzOS a thread IS a process (threads share a tgid, see clone), so a
 * tid is a pid. Same permission rule as kill. tkill is the old, less
 * safe interface; tgkill adds a thread-group check.
 *
 * Uses: scheduler_get_current_process, process_get_by_pid, process_kill
 *       (core/process); signal_send, SIGKILL (core/signal);
 *       def_signal_allowed (signal_permission).
 */
#include "modules/process_management/syscalls/tkill/sys_tkill.h"                                               // this file
#include "modules/process_management/core/process/process.h"                                                   // process_t, process_get_by_pid, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"                                               // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                                                     // SIGKILL, signal_send
#include "modules/process_management/security/signal_abuse/defence/signal_permission/def_signal_permission.h"  // def_signal_allowed

int sys_tkill(uint32_t tid, int sig) {
    process_t *self = scheduler_get_current_process();
    process_t *t = process_get_by_pid(tid);
    if (!t) return -1;
    if (!def_signal_allowed(self ? self->uid : 0, t->uid, t->is_system))
        return -1;
    if (sig == SIGKILL) { process_kill(tid); return 0; }
    signal_send(t, sig);
    return 0;
}
