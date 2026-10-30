#ifndef SYS_FTRUNCATE_H
#define SYS_FTRUNCATE_H
/* sys_ftruncate.h - ftruncate(fd, length): truncate an open file. */
#include <stdint.h>
#define SYS_FTRUNCATE 114   // EAX=114, EBX=fd, ECX=length
int sys_ftruncate(int fd, uint32_t length);
#endif
