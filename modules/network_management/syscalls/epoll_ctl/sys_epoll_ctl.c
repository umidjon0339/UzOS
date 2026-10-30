/*
 * sys_epoll_ctl.c - epoll_ctl(): change the watch list.
 *
 * Uses: epoll_control (core/epoll), scheduler.
 */
#include "modules/network_management/syscalls/epoll_ctl/sys_epoll_ctl.h"    // this file
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_epoll_ctl(int epfd, int op, int fd, const epoll_event_t *event) {
    return epoll_control(scheduler_get_current_process(), epfd, op, fd, event);
}
