#ifndef SYS_MKDIRAT_H
#define SYS_MKDIRAT_H
/* sys_mkdirat.h - mkdirat(dirfd, path, mode): mkdir relative to a directory, with a mode. */
#include <stdint.h>
#define SYS_MKDIRAT 120     // EAX=120, EBX=dirfd, ECX=path, EDX=mode (Linux bits: 0755)
int sys_mkdirat(int dirfd, const char *path, uint32_t mode);
#endif
