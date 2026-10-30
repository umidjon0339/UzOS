/*
 * epoll.c - The instances and their interest lists.
 *
 * Each instance is a vfs node inside a small struct (like a pidfd), so
 * it is a descriptor: close() frees it when the last descriptor goes.
 * epoll_wait turns the interest list into pollfd_t's and lets
 * core/poll do the waiting.
 *
 * Uses: poll_wait (core/poll), NET_E* (core/inet), cpu_irq_save, memset,
 *       strncpy (lib/string).
 */

#include "modules/network_management/core/epoll/epoll.h"         // this file
#include "modules/network_management/core/poll/poll.h"           // poll_wait, pollfd_t
#include "modules/network_management/core/inet/inet.h"          // NET_E*
#include "arch/x86/cpu/cpu.h"                                    // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                   // memset, strncpy

typedef struct {
    int fd;                         // -1 = unused
    uint32_t events;
    uint64_t data;
} interest_t;

typedef struct {
    vfs_node_t node;                // FIRST: a pointer to the node is a pointer to the instance
    int used;
    interest_t list[EPOLL_INTERESTS];
} epoll_t;

static epoll_t instances[EPOLL_MAX];

// The last descriptor closed: the instance is free again.
static void release(vfs_node_t *node) {
    ((epoll_t *)node)->used = 0;
}

int epoll_is(const vfs_node_t *node) {
    return node >= &instances[0].node && node <= &instances[EPOLL_MAX - 1].node;
}

vfs_node_t *epoll_create(void) {
    uint32_t irq = cpu_irq_save();
    for (int i = 0; i < EPOLL_MAX; i++) {
        epoll_t *e = &instances[i];
        if (e->used || e->node.open_count)
            continue;
        memset(e, 0, sizeof(*e));
        e->used = 1;
        for (int j = 0; j < EPOLL_INTERESTS; j++)
            e->list[j].fd = -1;
        strncpy(e->node.name, "anon_inode:[eventpoll]", FS_NAME_MAX);
        e->node.flags = FS_FILE;
        e->node.release = release;          // parent NULL, nlink 0: freed with its last fd
        cpu_irq_restore(irq);
        return &e->node;
    }
    cpu_irq_restore(irq);
    return 0;
}

static epoll_t *instance(process_t *p, int epfd) {
    if (epfd < 0 || epfd >= MAX_FDS || !epoll_is(p->fd_table[epfd]))
        return 0;
    return (epoll_t *)p->fd_table[epfd];
}

int epoll_control(process_t *p, int epfd, int op, int fd, const epoll_event_t *event) {
    epoll_t *e = instance(p, epfd);
    if (!e)
        return -NET_EINVAL;
    if (fd == epfd || fd < 0 || fd >= MAX_FDS || (fd > 2 && !p->fd_table[fd]))
        return -NET_EBADF;
    interest_t *slot = 0, *free_slot = 0;
    for (int i = 0; i < EPOLL_INTERESTS; i++) {
        if (e->list[i].fd == fd)
            slot = &e->list[i];
        else if (e->list[i].fd < 0 && !free_slot)
            free_slot = &e->list[i];
    }
    if (op == EPOLL_CTL_DEL) {
        if (!slot)
            return -NET_EINVAL;             // Linux: ENOENT
        slot->fd = -1;
        return 0;
    }
    if (op != EPOLL_CTL_ADD && op != EPOLL_CTL_MOD)
        return -NET_EINVAL;
    if (!event)
        return -NET_EFAULT;
    if (op == EPOLL_CTL_ADD) {
        if (slot)
            return -NET_EADDRINUSE;         // Linux: EEXIST
        if (!free_slot)
            return -NET_ENOMEM;
        slot = free_slot;
        slot->fd = fd;
    } else if (!slot) {
        return -NET_EINVAL;                 // MOD of something not watched
    }
    slot->events = event->events;
    slot->data = event->data;
    return 0;
}

int epoll_wait_events(process_t *p, int epfd, epoll_event_t *out, int max, int timeout_ms) {
    epoll_t *e = instance(p, epfd);
    if (!e || max <= 0)
        return -NET_EINVAL;
    pollfd_t fds[EPOLL_INTERESTS];
    interest_t *which[EPOLL_INTERESTS];
    int count = 0;
    for (int i = 0; i < EPOLL_INTERESTS; i++) {
        if (e->list[i].fd < 0)
            continue;
        fds[count].fd = e->list[i].fd;
        fds[count].events = (int16_t)(e->list[i].events & 0xFFFF);
        which[count++] = &e->list[i];
    }
    int ready = poll_wait(p, fds, count, timeout_ms);
    if (ready <= 0)
        return ready;
    int n = 0;
    for (int i = 0; i < count && n < max; i++) {
        if (!fds[i].revents)
            continue;
        out[n].events = (uint16_t)fds[i].revents;
        out[n].data = which[i]->data;
        n++;
    }
    return n;
}
