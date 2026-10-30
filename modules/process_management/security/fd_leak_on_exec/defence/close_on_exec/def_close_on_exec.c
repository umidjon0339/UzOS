/*
 * def_close_on_exec.c - DEFENCE close_on_exec: what was opened for the old
 * program does not reach the new one.
 *
 * Stops:  fd_leak_on_exec (security/fd_leak_on_exec)
 * Called: sys_execve() (syscalls/execve), right before the new program
 *         starts.
 * Rules:  1. every descriptor opened with O_CLOEXEC is closed
 *         2. every pidfd is closed too (UzOS: always close-on-exec, as
 *            Linux makes them)
 *         the others (0, 1, 2 and plain files) stay: that is how a
 *         shell passes a redirected file to a program
 * Why:    execve keeps the descriptor table. A privileged program that
 *         opened /etc/shadow and then runs a helper hands the helper
 *         that open file - it never needs permission to read it.
 *         O_CLOEXEC is how a program says "this one is mine only".
 *
 * Uses: fd_release (io_management core/fd), pidfd_is (core/pidfd),
 *       O_CLOEXEC (io_management syscalls/open), audit_log, registry.
 */

#include "modules/process_management/security/fd_leak_on_exec/defence/close_on_exec/def_close_on_exec.h"  // this file
#include "modules/process_management/core/pidfd/pidfd.h"          // pidfd_is
#include "modules/io_management/core/fd/fd.h"                     // fd_release, FIRST_FILE_FD
#include "modules/io_management/syscalls/open/sys_open.h"         // O_CLOEXEC
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static int must_close(uint32_t flags, int is_pidfd) {
    return (flags & O_CLOEXEC) || is_pidfd;
}

int def_close_on_exec(process_t *p) {
    int closed = 0;
    for (int fd = FIRST_FILE_FD; fd < MAX_FDS; fd++) {
        if (!p->fd_table[fd] || !must_close(p->fd_flags[fd], pidfd_is(p->fd_table[fd])))
            continue;
        fd_release(p, fd);
        closed++;
    }
    if (closed)
        audit_log("close_on_exec", "execve closed descriptors the old program kept for itself");
    return closed;
}

static int selftest(void) {
    if (!must_close(O_CLOEXEC | 1, 0)) return 0;    // O_READ | O_CLOEXEC: closed
    if (must_close(1 | 2, 0)) return 0;             // a plain file: stays
    if (!must_close(1, 1)) return 0;                // a pidfd: closed
    return 1;
}

static const defence_t defence = {
    "close_on_exec",
    "execve closes O_CLOEXEC descriptors and pidfds",
    "fd_leak_on_exec",
    selftest
};

void def_close_on_exec_register(void) { registry_add_defence(&defence); }
