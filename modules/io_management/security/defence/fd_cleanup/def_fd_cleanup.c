/*
 * def_fd_cleanup.c - DEFENCE fd_cleanup: a finished process closes its files.
 *
 * Stops:  fd_leak (security/attack/fd_leak)
 * Called: the reaper (process_management), just before a dead process's
 *         slot is freed.
 * Rule:   every descriptor still open is closed.
 * Why:    each open descriptor holds the file's open count up. A process
 *         that ends (or is killed) without close() would leave the count
 *         up forever, so busy_check would never let anyone delete that
 *         file. Programs forget, crash, or get killed: the kernel must
 *         clean up for them.
 */

#include "def_fd_cleanup.h"
#include "registry.h"
#include "audit.h"
#include "fd.h"

// Close everything; return how many were still open. No logging, so the
// selftest can use it too.
static int close_all(process_t *p) {
    int closed = 0;
    for (int fd = FIRST_FILE_FD; fd < MAX_FDS; fd++) {
        if (p->fd_table[fd]) {
            fd_release(p, fd);
            closed++;
        }
    }
    return closed;
}

void def_fd_close_all(process_t *p) {
    if (close_all(p))
        audit_log("fd_cleanup", "process ended with open files: closed them");
}

static int selftest(void) {
    // A pretend process with two open files.
    process_t probe = {0};
    vfs_node_t a = {0}, b = {0};
    fd_alloc(&probe, &a, 1);
    fd_alloc(&probe, &b, 1);

    return close_all(&probe) == 2 &&
           fd_count(&probe) == 0 && a.open_count == 0 && b.open_count == 0;
}

static const defence_t defence = {
    "fd_cleanup",
    "close every file a process left open when it ends",
    "fd_leak",
    selftest
};

void def_fd_cleanup_register(void) {
    registry_add_defence(&defence);
}
