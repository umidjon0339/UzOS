#ifndef SYS_FCHMOD_H
#define SYS_FCHMOD_H
/* sys_fchmod.h - fchmod(fd, mode): chmod the file behind a descriptor. */
#include <stdint.h>
#define SYS_FCHMOD 106       // EAX=106, EBX=fd, ECX=mode (Linux bits: 0644)
int sys_fchmod(int fd, uint32_t mode);
#endif
