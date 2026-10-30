/*
 * sys_epoll_create.c - epoll_create(size): epoll_create1(0), if size > 0.
 *
 * Uses: sys_epoll_create1 (syscalls/epoll_create1).
 */
#include "modules/network_management/syscalls/epoll_create/sys_epoll_create.h" // this file
#include "modules/network_management/syscalls/epoll_create1/sys_epoll_create1.h" // sys_epoll_create1
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL

int sys_epoll_create(int size) {
    return size > 0 ? sys_epoll_create1(0) : -NET_EINVAL;
}
