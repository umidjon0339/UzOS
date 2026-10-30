#ifndef SYS_READLINK_H
#define SYS_READLINK_H
/* sys_readlink.h - readlink(path, buf, size): where a symbolic link points. */
#include <stdint.h>
#define SYS_READLINK 131    // EAX=131, EBX=path, ECX=buf, EDX=size
int sys_readlink(const char *path, char *buf, uint32_t size);
#endif
