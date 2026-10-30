#ifndef PIDFD_H
#define PIDFD_H

/*
 * pidfd.h - A file descriptor that names one process.
 *
 * A PID is a number: once that process ends, the number could name
 * another process (on Linux, where PIDs wrap around). A pidfd names the
 * process itself:
 *
 *   fd = pidfd_open(pid)            the handle is made while it lives
 *   ...the process ends...
 *   pidfd_send_signal(fd, SIGTERM)  -> -1 (ESRCH), never someone else
 *
 * Each pidfd is a vfs node (so open/close/fd_cleanup handle it like a
 * file) that holds a pcb_ref_t: slot + pid + generation.
 */

#include "modules/process_management/core/process/process.h"   // process_t
#include "modules/filesystem_management/core/vfs/vfs.h"         // vfs_node_t

#define PIDFD_MAX 32            // pidfds open at once, all processes together

// A new pidfd node for 'target' (not open yet: fd_alloc opens it).
// NULL if all PIDFD_MAX are in use.
vfs_node_t *pidfd_create(process_t *target);

// Is 'node' a pidfd?
int pidfd_is(const vfs_node_t *node);

// The process that descriptor 'fd' of 'owner' names. NULL if 'fd' is
// not a pidfd, or that process has ended (pcb_generation).
process_t *pidfd_target(const process_t *owner, int fd);

#endif
