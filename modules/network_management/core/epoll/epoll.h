#ifndef EPOLL_H
#define EPOLL_H

/*
 * epoll.h - epoll: a kernel-kept list of descriptors to watch.
 *
 * poll() hands the kernel the whole list on EVERY call. epoll keeps the
 * list in the kernel instead, so a server with thousands of clients
 * says once "watch this one" and then only asks "who is ready?":
 *
 *   int ep = epoll_create1(0);                     the list, as a descriptor
 *   epoll_ctl(ep, EPOLL_CTL_ADD, fd, &event);      watch fd (events, your data)
 *   n = epoll_wait(ep, events, 16, timeout);       who is ready
 *
 * UzOS's epoll is "level-triggered" only: a descriptor is reported on
 * every call while it is ready (EPOLLET is accepted but acts the same).
 * Interests name descriptors of the calling process (Linux names the
 * open file itself).
 */

#include <stdint.h>
#include "modules/filesystem_management/core/vfs/vfs.h"         // vfs_node_t
#include "modules/process_management/core/process/process.h"    // process_t

#define EPOLL_MAX        8          // epoll instances at once
#define EPOLL_INTERESTS  16         // descriptors one instance watches

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

#define EPOLLIN      0x001          // the same bits as POLLIN, POLLOUT, ...
#define EPOLLOUT     0x004
#define EPOLLERR     0x008
#define EPOLLHUP     0x010
#define EPOLLET      (1u << 31)     // edge-triggered (treated as level here)
#define EPOLL_CLOEXEC 0x80000

// Linux's struct epoll_event (packed on x86: 12 bytes).
typedef struct {
    uint32_t events;
    uint64_t data;                  // anything the program wants back
} __attribute__((packed)) epoll_event_t;

// A new instance (not open yet: fd_alloc opens it), or NULL.
vfs_node_t *epoll_create(void);
int epoll_is(const vfs_node_t *node);

// EPOLL_CTL_ADD / MOD / DEL on the instance at epfd of p. 0 or -NET_E*.
int epoll_control(process_t *p, int epfd, int op, int fd, const epoll_event_t *event);

// Up to max ready events; wait up to timeout_ms. Count, 0, or -NET_E*.
int epoll_wait_events(process_t *p, int epfd, epoll_event_t *out, int max, int timeout_ms);

#endif
