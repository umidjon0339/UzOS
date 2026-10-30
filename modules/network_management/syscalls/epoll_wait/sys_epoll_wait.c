/*
 * sys_epoll_wait.c - epoll_wait(): sleep until something on the list is ready.
 *
 * Uses: epoll_wait_events (core/epoll), scheduler.
 */
#include "modules/network_management/syscalls/epoll_wait/sys_epoll_wait.h"  // this file
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

int sys_epoll_wait(int epfd, epoll_event_t *events, int max, int timeout_ms) {
    return epoll_wait_events(scheduler_get_current_process(), epfd, events, max, timeout_ms);
}
