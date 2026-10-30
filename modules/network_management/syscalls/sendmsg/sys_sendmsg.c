/*
 * sys_sendmsg.c - sendmsg(): gather the iovec, then send it.
 *
 * A datagram must go as one, so its pieces are copied together into a
 * kernel buffer first (at most GATHER_MAX bytes). A stream may go piece
 * by piece. Ancillary data (passing descriptors, ...) is not supported.
 *
 * Uses: socket_from_fd / socket_sendto (core/socket), scheduler, memcpy.
 */
#include "modules/network_management/syscalls/sendmsg/sys_sendmsg.h"        // this file
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "lib/string/string.h"                                              // memcpy

#define GATHER_MAX 4096

int sys_sendmsg(int fd, const msghdr_t *msg, int flags) {
    process_t *self = scheduler_get_current_process();
    socket_t *s = socket_from_fd(self, fd);
    if (!s)
        return socket_fd_error(self, fd);   // -EBADF / -ENOTSOCK
    if (msg->iovlen > MSG_IOV_MAX || msg->controllen)
        return -NET_EINVAL;

    if (s->type == SOCK_STREAM) {           // a stream: each piece in turn
        int total = 0;
        for (uint32_t i = 0; i < msg->iovlen; i++) {
            int r = socket_sendto(s, msg->iov[i].base, msg->iov[i].len, flags, 0, 0);
            if (r < 0)
                return total ? total : r;
            total += r;
        }
        return total;
    }
    static uint8_t gather[GATHER_MAX];      // a datagram: all pieces in one
    uint32_t len = 0;
    for (uint32_t i = 0; i < msg->iovlen; i++) {
        if (len + msg->iov[i].len > GATHER_MAX)
            return -NET_EMSGSIZE;
        memcpy(gather + len, msg->iov[i].base, msg->iov[i].len);
        len += msg->iov[i].len;
    }
    return socket_sendto(s, gather, len, flags, msg->name, msg->namelen);
}
