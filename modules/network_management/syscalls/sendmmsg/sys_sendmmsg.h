#ifndef SYS_SENDMMSG_H
#define SYS_SENDMMSG_H
/* sys_sendmmsg.h - sendmmsg(fd, msgs, count, flags): several sendmsg() in one syscall. */
#include <stdint.h>
#include "modules/network_management/syscalls/sendmsg/sys_sendmsg.h"   // msghdr_t
#define SYS_SENDMMSG 150    // EAX=150, EBX=fd, ECX=msgs (mmsghdr_t[]), EDX=count, ESI=flags
#define MMSG_MAX     16     // messages per call

// Linux's struct mmsghdr: a message and how many bytes of it went.
typedef struct {
    msghdr_t hdr;
    uint32_t len;
} mmsghdr_t;

// How many messages were sent, or -errno if not even the first.
int sys_sendmmsg(int fd, mmsghdr_t *msgs, uint32_t count, int flags);
#endif
