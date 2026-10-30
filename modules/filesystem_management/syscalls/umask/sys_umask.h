#ifndef SYS_UMASK_H
#define SYS_UMASK_H
/* sys_umask.h - umask(mask): the mode bits my new files will NOT get. */
#include <stdint.h>
#define SYS_UMASK 112       // EAX=112, EBX=mask (Linux bits: 022)
uint32_t sys_umask(uint32_t mask);
#endif
