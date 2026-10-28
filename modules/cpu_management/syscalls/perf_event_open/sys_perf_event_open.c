/*
 * sys_perf_event_open.c - perf_event_open(): a counter behind a file descriptor.
 *
 *     perf_event_attr_t a = { PERF_TYPE_SOFTWARE, sizeof(a),
 *                             PERF_COUNT_SW_CONTEXT_SWITCHES };
 *     int fd = perf_event_open(&a, 0, -1, -1, 0);
 *     ...
 *     uint64_t n; read(fd, &n, 8);     // switches since the open
 *     close(fd);
 *
 * Who may watch whom:
 *   pid 0 / your own process   anyone
 *   another user's process     CAP_PRIORITY (root)
 *   pid -1 (the whole system)  CAP_PRIORITY (root)
 * (Linux uses perf_event_paranoid and CAP_PERFMON for this.)
 *
 * Uses: perf_counter_create (perf_counter.c, this folder), fd_alloc /
 *       fd_release (io_management),
 *       rlimit_nofile (core/rlimit),
 *       caps_has (security_management), the current process (process_management).
 */

#include "modules/cpu_management/syscalls/perf_event_open/sys_perf_event_open.h"
#include "modules/io_management/core/fd/fd.h"                       // fd_alloc, fd_release
#include "modules/cpu_management/core/rlimit/rlimit.h"              // rlimit_nofile
#include "modules/io_management/syscalls/open/sys_open.h"           // O_READ
#include "modules/security_management/core/caps/caps.h"             // caps_has
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process

int sys_perf_event_open(const perf_event_attr_t *attr, int pid, int cpu,
                        int group_fd, uint32_t flags) {
    process_t *self = scheduler_get_current_process();
    if (!attr || !self || attr->size < sizeof(perf_event_attr_t))
        return -1;
    if (group_fd != -1 || flags != 0 || (cpu != -1 && cpu != 0))
        return -1;                          // no groups, one CPU
    if (pid < -1)
        return -1;

    // Whom to watch, and may we?
    uint32_t target = 0;                    // 0 = the whole system
    if (pid == -1) {
        if (!caps_has(self->uid, CAP_PRIORITY))
            return -1;
    } else {
        process_t *p = pid ? process_get_by_pid(pid) : self;
        if (!p || (p->uid != self->uid && !caps_has(self->uid, CAP_PRIORITY)))
            return -1;
        target = p->pid;
    }

    vfs_node_t *node = perf_counter_create(target, attr->type, attr->config);
    if (!node)
        return -1;                          // unsupported, or all counters busy
    int fd = fd_alloc(self, node, O_READ);
    if (fd >= rlimit_nofile(self)) {        // RLIMIT_NOFILE, as in open()
        fd_release(self, fd);
        return -1;
    }
    return fd;
}
