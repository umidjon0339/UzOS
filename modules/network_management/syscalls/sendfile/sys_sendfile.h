#ifndef SYS_SENDFILE_H
#define SYS_SENDFILE_H
/* sys_sendfile.h - sendfile(out_fd, in_fd, offset, count): copy a file into a socket inside the kernel. */
#include <stdint.h>
#define SYS_SENDFILE 166    // EAX=166, EBX=out_fd, ECX=in_fd, EDX=offset (uint32_t*, or 0), ESI=count
// Bytes copied, or -errno: -EBADF, -EINVAL (in_fd is not a file).
int sys_sendfile(int out_fd, int in_fd, uint32_t *offset, uint32_t count);
#endif
