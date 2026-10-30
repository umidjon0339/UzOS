#ifndef SYS_FCHMODAT_H
#define SYS_FCHMODAT_H
/* sys_fchmodat.h - fchmodat(dirfd, path, mode): chmod relative to a directory. */
#include <stdint.h>
#define SYS_FCHMODAT 107       // EAX=107, EBX=dirfd, ECX=path, EDX=mode (Linux bits: 0644)
int sys_fchmodat(int dirfd, const char *path, uint32_t mode);
#endif
