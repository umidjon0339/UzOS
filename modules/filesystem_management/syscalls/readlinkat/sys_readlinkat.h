#ifndef SYS_READLINKAT_H
#define SYS_READLINKAT_H
/* sys_readlinkat.h - readlinkat(dirfd, path, buf, size): readlink relative to a directory. */
#include <stdint.h>
#define SYS_READLINKAT 132  // EAX=132, EBX=dirfd, ECX=path, EDX=buf, ESI=size
int sys_readlinkat(int dirfd, const char *path, char *buf, uint32_t size);
#endif
