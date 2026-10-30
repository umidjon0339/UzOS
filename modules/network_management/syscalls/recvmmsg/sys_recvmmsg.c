/*
 * sys_recvmmsg.c - recvmmsg(): receive a batch of messages.
 *
 * The first one may wait (unless MSG_DONTWAIT); after it, only what is
 * already there is taken, so the call never waits for a full batch.
 *
 * Uses: sys_recvmsg (syscalls/recvmsg).
 */
#include "modules/network_management/syscalls/recvmmsg/sys_recvmmsg.h"      // this file
#include "modules/network_management/syscalls/recvmsg/sys_recvmsg.h"        // sys_recvmsg
#include "modules/network_management/core/socket/socket.h"                  // MSG_DONTWAIT

int sys_recvmmsg(int fd, mmsghdr_t *msgs, uint32_t count, int flags) {
    if (count > MMSG_MAX)
        count = MMSG_MAX;
    uint32_t got = 0;
    for (; got < count; got++) {
        int r = sys_recvmsg(fd, &msgs[got].hdr, got ? flags | MSG_DONTWAIT : flags);
        if (r < 0)
            return got ? (int)got : r;
        msgs[got].len = (uint32_t)r;
    }
    return (int)got;
}
