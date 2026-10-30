/*
 * sys_epoll_create1.c - epoll_create1(): make the watch list and give it a descriptor.
 *
 * Uses: epoll_create (core/epoll), fd_alloc (io_management), scheduler, O_*.
 */
#include "modules/network_management/syscalls/epoll_create1/sys_epoll_create1.h" // this file
#include "modules/network_management/core/epoll/epoll.h"                    // epoll_create, EPOLL_CLOEXEC
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/io_management/core/fd/fd.h"                               // fd_alloc
#include "modules/io_management/syscalls/open/sys_open.h"                   // O_READ, O_CLOEXEC
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_epoll_create1(int flags) {
    if (flags & ~EPOLL_CLOEXEC)
        return -NET_EINVAL;
    vfs_node_t *node = epoll_create();
    if (!node)
        return -NET_ENOMEM;
    int fd = fd_alloc(scheduler_get_current_process(), node,
                      O_READ | ((flags & EPOLL_CLOEXEC) ? O_CLOEXEC : 0));
    return fd < 0 ? -NET_EMFILE : fd;
}
