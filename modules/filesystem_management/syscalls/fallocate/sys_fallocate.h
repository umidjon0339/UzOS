#ifndef SYS_FALLOCATE_H
#define SYS_FALLOCATE_H
/* sys_fallocate.h - fallocate(fd, mode, offset, length): reserve room in a file. */
#include <stdint.h>
#define SYS_FALLOCATE 115   // EAX=115, EBX=fd, ECX=mode (0), EDX=offset, ESI=length
int sys_fallocate(int fd, int mode, uint32_t offset, uint32_t length);
#endif
