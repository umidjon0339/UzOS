#ifndef SYS_RECVMMSG_H
#define SYS_RECVMMSG_H
/* sys_recvmmsg.h - recvmmsg(fd, msgs, count, flags, timeout): several recvmsg() in one syscall. */
#include <stdint.h>
#include "modules/network_management/syscalls/sendmmsg/sys_sendmmsg.h"  // mmsghdr_t, MMSG_MAX
#define SYS_RECVMMSG 151    // EAX=151, EBX=fd, ECX=msgs, EDX=count, ESI=flags, EDI=timeout (ignored, 0)
// How many messages were received, or -errno if not even the first.
int sys_recvmmsg(int fd, mmsghdr_t *msgs, uint32_t count, int flags);
#endif
