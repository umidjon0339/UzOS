/*
 * sys_recvmsg.c - recvmsg(): receive one message, then scatter it.
 *
 * The message lands in a kernel buffer first, then its bytes are
 * spread over the iovec's buffers in order. A datagram longer than all
 * of them together is cut, and msg->flags says MSG_TRUNC.
 *
 * Uses: socket_from_fd / socket_recvfrom (core/socket), scheduler, memcpy.
 */
#include "modules/network_management/syscalls/recvmsg/sys_recvmsg.h"        // this file
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "lib/string/string.h"                                              // memcpy

#define SCATTER_MAX 4096

int sys_recvmsg(int fd, msghdr_t *msg, int flags) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    if (msg->iovlen > MSG_IOV_MAX)
        return -NET_EINVAL;
    uint32_t room = 0;
    for (uint32_t i = 0; i < msg->iovlen; i++)
        room += msg->iov[i].len;

    static uint8_t buf[SCATTER_MAX];
    uint32_t want = s->type == SOCK_STREAM ? (room < SCATTER_MAX ? room : SCATTER_MAX) : SCATTER_MAX;
    int n = socket_recvfrom(s, buf, want, flags, msg->name, msg->name ? &msg->namelen : 0);
    if (n < 0)
        return n;
    msg->flags = (uint32_t)n > room ? MSG_TRUNC : 0;
    msg->controllen = 0;                    // no ancillary data
    uint32_t done = 0;
    for (uint32_t i = 0; i < msg->iovlen && done < (uint32_t)n; i++) {
        uint32_t part = (uint32_t)n - done < msg->iov[i].len ? (uint32_t)n - done : msg->iov[i].len;
        memcpy(msg->iov[i].base, buf + done, part);
        done += part;
    }
    return (int)done;
}
