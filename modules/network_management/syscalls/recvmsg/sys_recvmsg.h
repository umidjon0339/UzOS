#ifndef SYS_RECVMSG_H
#define SYS_RECVMSG_H
/* sys_recvmsg.h - recvmsg(fd, msg, flags): receive into several buffers at once. */
#include <stdint.h>
#include "modules/network_management/syscalls/sendmsg/sys_sendmsg.h"   // msghdr_t, iovec_t
#define SYS_RECVMSG 145     // EAX=145, EBX=fd, ECX=msg (msghdr_t), EDX=flags
#define MSG_TRUNC   0x20    // msg.flags: the datagram was longer than the buffers
// Bytes received, or -errno (as recvfrom). msg->name gets the sender.
int sys_recvmsg(int fd, msghdr_t *msg, int flags);
#endif
