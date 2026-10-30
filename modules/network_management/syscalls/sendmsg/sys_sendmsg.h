#ifndef SYS_SENDMSG_H
#define SYS_SENDMSG_H
/*
 * sys_sendmsg.h - sendmsg(fd, msg, flags): send from several buffers at once.
 *
 * The message's bytes may lie in several places (an "iovec": a list of
 * base + length), e.g. a header here and the data there; sendmsg sends
 * them as ONE message, without copying them together first in the
 * program.
 */
#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"     // sockaddr_t
#define SYS_SENDMSG 144     // EAX=144, EBX=fd, ECX=msg (msghdr_t), EDX=flags
#define MSG_IOV_MAX 16      // buffers per message

typedef struct {
    void *base;
    uint32_t len;
} iovec_t;

// Linux's struct msghdr (32-bit).
typedef struct {
    sockaddr_t *name;       // the address (sendto's) or NULL
    uint32_t namelen;
    iovec_t *iov;           // the buffers
    uint32_t iovlen;
    void *control;          // ancillary data: not supported, must be 0 long
    uint32_t controllen;
    int flags;              // recvmsg: MSG_TRUNC ...
} msghdr_t;

// Bytes sent, or -errno (as sendto).
int sys_sendmsg(int fd, const msghdr_t *msg, int flags);
#endif
