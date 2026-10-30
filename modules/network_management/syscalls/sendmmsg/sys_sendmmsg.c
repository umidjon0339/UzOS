/*
 * sys_sendmmsg.c - sendmmsg(): send a batch of messages.
 *
 * One trip into the kernel instead of one per message: that is the
 * whole point (a DNS server answering many queries saves syscalls).
 * It stops at the first message that fails.
 *
 * Uses: sys_sendmsg (syscalls/sendmsg).
 */
#include "modules/network_management/syscalls/sendmmsg/sys_sendmmsg.h"      // this file
#include "modules/network_management/core/inet/inet.h"                      // NET_EINVAL

int sys_sendmmsg(int fd, mmsghdr_t *msgs, uint32_t count, int flags) {
    if (count > MMSG_MAX)
        count = MMSG_MAX;
    uint32_t sent = 0;
    for (; sent < count; sent++) {
        int r = sys_sendmsg(fd, &msgs[sent].hdr, flags);
        if (r < 0)
            return sent ? (int)sent : r;
        msgs[sent].len = (uint32_t)r;
    }
    return (int)sent;
}
